


#include <motor/network/typedefs.h>
#include <motor/platform/network/network_module_creator.hpp>
#include <motor/social/twitch/irc_parser.hpp>
#include <motor/io/database.h>

#include <nlohmann/json.hpp>

#include <fstream>
#include <string>

using namespace motor::core::types ;

bool_t done = false ;

namespace this_file
{
    // the client code flow can not be used because it only grants a app access token,
    // but the irc bot requires a user access token. A user access token requires the
    // user to authorize the bot to access the chat and maybe other scopes.
    enum class code_grant_flow
    {
        none,
        device_code_flow, // no server, limited input(setopbox, gameconsode, engine) [user access token]
        authorization_code_flow, // server/server [user access token]
        implicit_code_flow, // no server, client-side apps(java script, modile app) [user access token]
        client_code_flow // server/server like acf [app access token]
    } ;

    // The response when requesting a device code
    struct device_code_response
    {
        motor::string_t device_code ;
        int_t expires_in = 0 ;
        int_t interval = 0  ;
        motor::string_t user_code ;
        motor::string_t verification_uri ;

        device_code_response( void_t ) noexcept{}

        device_code_response( device_code_response const & rhv ) noexcept
        {
            device_code = ( rhv.device_code ) ;
            expires_in = rhv.expires_in ;
            interval = rhv.interval ;
            user_code = ( rhv.user_code ) ;
            verification_uri = ( rhv.verification_uri ) ;
        }

        device_code_response & operator = ( device_code_response  const & rhv ) noexcept
        {
            device_code = ( rhv.device_code ) ;
            expires_in = rhv.expires_in ;
            interval = rhv.interval ;
            user_code = ( rhv.user_code ) ;
            verification_uri = ( rhv.verification_uri ) ;

            return *this ;
        }

        device_code_response( device_code_response && rhv ) noexcept
        {
            device_code = std::move( rhv.device_code ) ;
            expires_in = rhv.expires_in ;
            interval = rhv.interval ;
            user_code = std::move( rhv.user_code ) ;
            verification_uri = std::move( rhv.verification_uri ) ;
        }

        device_code_response & operator = ( device_code_response && rhv ) noexcept
        {
            device_code = std::move( rhv.device_code ) ;
            expires_in = rhv.expires_in ;
            interval = rhv.interval ;
            user_code = std::move( rhv.user_code ) ;
            verification_uri = std::move( rhv.verification_uri ) ;

            return *this ;
        }
    } ;
    motor_typedef( device_code_response ) ;

    enum class program_state
    {
        load_credentials,
        need_device_code,
        need_token,
        need_refresh,
        need_token_validation,
        pending,
        need_login,
        login_pending,
        bot_is_online
    };

    enum class initial_process_result
    {
        invalid,
        ok, 
        curl_failed,
        // there are no credentials or no client_id
        credentials_failed,
        // credentials has no token
        no_token,
        // twitch responded the token to be invalid
        invalid_token
    };

    enum class refresh_process_result
    {
        invalid,
        invalid_client_id,
        invalid_token,
        ok,
        curl_failed,
        
    };

    enum class request_user_token_result
    {
        invalid,
        curl_failed,
        invalid_device_code,
        request_failed,
        pending,
        ok
    };

    enum class validation_process_result
    {
        invalid,
        ok,
        invalid_access_token,
        curl_failed,
    };

    enum class device_code_process_result
    {
        curl_failed,
        device_code_request_failed,
        ok
    };

    class twitch_irc_bot : public motor::network::iclient_handler
    {
        motor_this_typedefs( twitch_irc_bot ) ;

    private:

        motor::string_t data_out ;
        motor::string_t data_in ;

        bool_t _require_pong = false ;
        motor::string_t _pong = "" ;

        motor::social::twitch::irc_parser_t parser ;

        
        this_file::program_state _ps = 
            this_file::program_state::load_credentials ;

        using clk_t = std::chrono::high_resolution_clock ;
        clk_t::time_point _login_tp ;

        // this is when update needs to wait for something
        std::chrono::seconds _timeout = std::chrono::seconds(0) ;

        motor::io::database_mtr_t _db ;

        struct login_data
        {
            motor::string_t user_token ;
            motor::string_t refresh_token ;
            motor::string_t client_id ;
            motor::string_t client_secret ;
            motor::string_t scopes ;
            motor::string_t device_code ;
        };
        motor_typedef( login_data  ) ;


        login_data_t _login_data ;

        device_code_response _dcr ;

        code_grant_flow _mode = this_file::code_grant_flow::device_code_flow ;

    private:

        //**********************************************************************************
        validation_process_result validate_token( login_data_cref_t ld ) const noexcept
        {
            motor::log::global_t::status( "Validating User Tocken" ) ;

            auto const curl_validate_com =
                "curl -X GET https://id.twitch.tv/oauth2/validate "
                "-H \"Authorization: OAuth " + ld.user_token + "\" "
                "-s -o validate" ;

            auto const sys_res = std::system( curl_validate_com.c_str() ) ;

            if( sys_res != 0 ) 
            {
                return this_file::validation_process_result::curl_failed ;
            }

            // investigate response
            {
                std::string response ;

                // read content
                {
                    std::ifstream myfile( "validate" );
                    std::string line;
                    if ( myfile.is_open() )
                    {
                        while ( std::getline ( myfile, line ) )
                        {
                            response += line ;
                        }
                        myfile.close();
                    }
                }

                // parse json
                {
                    nlohmann::json data = nlohmann::json::parse( response ) ;

                    if ( data.contains("status") && data.contains( "message" ) )
                    {
                        size_t const code = data[ "status" ] ;
                        motor::string_t const msg = data["message"] ;

                        motor::log::global_t::status( "Twitch IRC Bot token validation failed: " ) ;
                        motor::log::global_t::status( "["+motor::to_string(code)+"] : " + msg ) ;
                        return this_file::validation_process_result::invalid_access_token ;
                    }
                }
            }

            // clear content
            {
                std::ofstream ofs ;
                ofs.open( "validate", std::ofstream::out | std::ofstream::trunc );
                ofs.close();
            }

            return this_file::validation_process_result::ok ;
        }

        //**********************************************************************************
        // Refresh the user token using the refresh token.
        this_file::refresh_process_result refresh_token( login_data_ref_t ld, this_file::code_grant_flow const cgf ) const noexcept
        {
            motor::log::global_t::status( "Refreshing User Token" ) ;

            if( cgf == this_file::code_grant_flow::authorization_code_flow )
            {
                auto const curl_refresh_com =
                    "curl -X POST https://id.twitch.tv/oauth2/token "
                    "-H \"Content-Type: application/x-www-form-urlencoded\" "
                    "-d \"grant_type=refresh_token&refresh_token=" + ld.refresh_token +
                    "&client_id=" + ld.client_id +
                    "&client_secret=" + ld.client_secret + "\" "
                    "-s -o refresh_token";

                auto const sys_res = std::system( curl_refresh_com.c_str() ) ;

                if ( sys_res != 0 ) 
                {
                    return this_file::refresh_process_result::curl_failed ;
                }
            }
            else if ( cgf == this_file::code_grant_flow::device_code_flow )
            {
                auto const curl_refresh_com =
                    "curl -X POST https://id.twitch.tv/oauth2/token "
                    "-H \"Content-Type: application/x-www-form-urlencoded\" "
                    "-d \"grant_type=refresh_token&refresh_token=" + ld.refresh_token +
                    "&client_id=" + ld.client_id +
                    #if 0
                    "&client_secret=" + ld.client_secret + "\" "
                    #else
                    "\" "
                    #endif
                    "-s -o refresh_token";

                auto const sys_res = std::system( curl_refresh_com.c_str() ) ;

                if ( sys_res != 0 )
                {
                    return this_file::refresh_process_result::curl_failed ;
                }
            }

            std::string response ;

            // read content
            {
                std::ifstream myfile( "refresh_token" ) ;
                std::string line;
                if ( myfile.is_open() )
                {
                    while ( std::getline ( myfile, line ) )
                    {
                        response += line ;
                    }
                    myfile.close();
                }
            }

            // clear content
            {
                std::ofstream ofs;
                ofs.open( "refresh_token", std::ofstream::out | std::ofstream::trunc );
                ofs.close();
            }

            // json validate
            {
                nlohmann::json data = nlohmann::json::parse( response ) ;
                if( data.contains("status") )
                {
                    size_t const code = data["status"] ;
                    motor::string_t msg ;
                    if( data.contains( "message" ) )
                    {
                        msg = data["message"] ;
                    }
                    motor::log::global_t::status( "Twitch IRC Bot token refresh failed: " ) ;
                    motor::log::global_t::status( "[" + motor::to_string( code ) + "] : " + msg ) ;

                    //return this_file::refresh_process_result::invalid_client_id ;
                    return this_file::refresh_process_result::invalid_token ;
                }
                
                if( data.contains("access_token") )
                {
                    ld.user_token = data["access_token"] ;
                }

                if ( data.contains( "refresh_token" ) )
                {
                    ld.refresh_token = data[ "refresh_token" ] ;
                }
            }

            return this_file::refresh_process_result::ok ;
        }

        this_file::device_code_process_result request_device_code( login_data_inout_t ld, device_code_response & rt ) const noexcept
        {
            motor::log::global_t::status( "Requesting Device Code" ) ;

            auto const curl_request_com =
                "curl --location https://id.twitch.tv/oauth2/device "
                "--form \"client_id=" + ld.client_id + "\" "
                "--form \"scopes="+ld.scopes+"\" "
                "-s -o data_code_flow_request" ;

            auto const sys_res = std::system( curl_request_com.c_str() ) ;
            if( sys_res != 0 ) 
            {
                return this_file::device_code_process_result::curl_failed ;
            }

            std::string response ;

            // read content
            {
                std::ifstream myfile( "data_code_flow_request" ) ;
                std::string line;
                if ( myfile.is_open() )
                {
                    while ( std::getline ( myfile, line ) )
                    {
                        response += line ;
                    }
                    myfile.close();
                }
            }

            // clear content
            {
                std::ofstream ofs;
                ofs.open( "data_code_flow_request", std::ofstream::out | std::ofstream::trunc );
                ofs.close();
            }

            // json 
            {
                nlohmann::json data = nlohmann::json::parse( response ) ;

                if( data.contains("status") )
                {
                    size_t const code = data["status"] ;
                    motor::string_t msg ;
                    if( data.contains( "message" ) )
                    {
                        msg = data["message"] ;
                    }
                    motor::log::global_t::error( "Twitch IRC Bot device code request failed: " ) ;
                    motor::log::global_t::error( "[" + motor::to_string( code ) + "] : " + msg ) ;

                    return this_file::device_code_process_result::device_code_request_failed ;
                }
                
                if( data.contains("device_code") )
                {
                    rt.device_code = data["device_code"] ;
                    ld.device_code = data[ "device_code" ] ;
                }
                if( data.contains("expires_in") )
                {
                    rt.expires_in = data["expires_in"] ;
                }
                if( data.contains("interval") )
                {
                    rt.interval = data["interval"] ;
                }
                if( data.contains("user_code"))
                {
                    rt.user_code = data["user_code"] ;
                }
                if( data.contains("verification_uri"))
                {
                    rt.verification_uri = data["verification_uri"] ;
                }
            }
            return this_file::device_code_process_result::ok ;
        }


        this_file::request_user_token_result request_user_token(
            this_t::login_data_inout_t ld ) const noexcept
        {
            motor::log::global_t::status( "Requesting User Token" ) ;

            auto const curl_request_com =
                "curl --location https://id.twitch.tv/oauth2/token "
                "--form \"client_id=" + ld.client_id + "\" "
                "--form \"scopes=" + ld.scopes + "\" "
                "--form \"device_code=" + ld.device_code + "\" "
                "--form \"grant_type=urn:ietf:params:oauth:grant-type:device_code\" "
                "-s -o user_access_token" ;

            auto const sys_res = std::system( curl_request_com.c_str() ) ;
            if ( sys_res != 0 ) 
            {
                return this_file::request_user_token_result::curl_failed ;
            }

            std::string response ;

            // read content
            {
                std::ifstream myfile( "user_access_token" ) ;
                std::string line;
                if ( myfile.is_open() )
                {
                    while ( std::getline ( myfile, line ) )
                    {
                        response += line ;
                    }
                    myfile.close();
                }
            }

            // clear content
            {
                std::ofstream ofs;
                ofs.open( "user_access_token", std::ofstream::out | std::ofstream::trunc );
                ofs.close();
            }

            // json 
            if( !response.empty() )
            {
                nlohmann::json data = nlohmann::json::parse( response ) ;

                if ( data.contains( "status" ) )
                {
                    size_t const code = data[ "status" ] ;
                    motor::string_t msg ;

                    if ( data.contains( "message" ) )
                    {
                        msg = data[ "message" ] ;
                    }

                    motor::log::global_t::error( "Twitch IRC Bot device code request failed: " ) ;
                    motor::log::global_t::error( "[" + motor::to_string( code ) + "] : " + msg ) ;

                    if( msg == "authorization_pending" )
                    {
                        return this_file::request_user_token_result::pending ;
                    }
                    else if( msg == "invalid device code" )
                    {
                        return this_file::request_user_token_result::invalid_device_code ;
                    }
                    else if( msg == "missing device_code" )
                    {
                        return this_file::request_user_token_result::invalid_device_code ;
                    }
                    
                    return this_file::request_user_token_result::request_failed ;
                }

                if ( data.contains( "access_token" ) )
                {
                    ld.user_token = data[ "access_token" ] ;
                }
                if ( data.contains( "refresh_token" ) )
                {
                    ld.refresh_token = data[ "refresh_token" ] ;
                }
            }
            else
            {
                motor::log::global_t::error("response was empty. Please check curl request.") ;
                return this_file::request_user_token_result::request_failed ;
            }
            return this_file::request_user_token_result::ok ;
        }

        bool_t load_credentials( this_t::login_data_out_t ld ) const noexcept
        {
            bool_t ret = false ;

            _db->load( motor::io::location( "twitch.credentials" ) ).wait_for_operation(
                [&] ( char_cptr_t buf, size_t const sib, motor::io::result const res )
            {
                if ( res == motor::io::result::ok )
                {
                    auto const content = std::string( buf, sib ) ;
                    auto const json = nlohmann::json::parse( content ) ;
                    if ( json.contains( "access_token" ) )
                    {
                        ld.user_token = json[ "access_token" ] ;
                    }
                    if ( json.contains( "refresh_token" ) )
                    {
                        ld.refresh_token = json[ "refresh_token" ] ;
                    }
                    if ( json.contains( "client_id" ) )
                    {
                        ld.client_id = json[ "client_id" ] ;
                    }
                    if ( json.contains( "client_secret" ) )
                    {
                        ld.client_secret = json[ "client_secret" ] ;
                    }
                    if ( json.contains( "scopes" ) )
                    {
                        ld.scopes = json[ "scopes" ] ;
                    }
                    if ( json.contains( "device_code" ) )
                    {
                        ld.device_code = json[ "device_code" ] ;
                    }
                    ret = true ;
                }
            } ) ;
            return ret ;
        }

        //**********************************************************************************
        bool_t validate_credentials( this_t::login_data_out_t ld  ) const noexcept
        {
            return !ld.client_id.empty() ;
        }

        //**********************************************************************************
        void_t write_credentials( this_t::login_data_in_t ld ) const noexcept
        {
            //
            std::string const content = nlohmann::json (
            {
                { "access_token", _login_data.user_token },
                { "refresh_token", _login_data.refresh_token },
                { "client_id", _login_data.client_id },
                { "client_secret", _login_data.client_secret },
                { "scopes", _login_data.scopes },
                { "device_code", _login_data.device_code }
            } ).dump() ;

            _db->store( motor::io::location_t( "twitch.credentials" ), content.c_str(), content.size() ).
                wait_for_operation( [&] ( motor::io::result const res )
            {
                if ( res == motor::io::result::ok )
                {
                    motor::log::global_t::error( "New Twitch Tokens written to : twitch.credentials" ) ;
                }
                else
                {
                    motor::log::global_t::error( "failed to write new tokens" ) ;
                }
            } ) ;
        }

        //**********************************************************************************
        void_t update( void_t ) noexcept
        {
            std::this_thread::sleep_for( _timeout ) ;
            _timeout = std::chrono::seconds( 0 ) ;

            switch( _ps ) 
            {
            case this_file::program_state::load_credentials:
            {
                auto const res = this_t::load_credentials_from_file() ;
                switch( res )
                {
                case this_file::initial_process_result::curl_failed: 
                    exit( 1 ) ; 
                    break ;
                case this_file::initial_process_result::invalid_token: 
                    _ps = this_file::program_state::need_refresh ;
                    break ;
                case this_file::initial_process_result::no_token: 
                    _ps = this_file::program_state::need_device_code ;
                    break ;
                case this_file::initial_process_result::ok : 
                    _ps = this_file::program_state::need_token_validation ;
                    break ;
                }

                break ;
            }
            case this_file::program_state::need_token_validation:
            {
                auto const res = this_t::validate_token( _login_data ) ;
                switch( res ) 
                {
                case this_file::validation_process_result::curl_failed:
                    exit( 1 ) ;
                    break ;
                case this_file::validation_process_result::invalid_access_token:
                    _ps = this_file::program_state::need_refresh ;
                    break ;
                case this_file::validation_process_result::ok:
                    _ps = this_file::program_state::need_login ;
                    break ;
                }
                break ;
            }
            case this_file::program_state::need_device_code:
            {
                auto const res = this_t::request_device_code( _login_data, _dcr ) ;
                switch( res ) 
                {
                case this_file::device_code_process_result::curl_failed: 
                    exit( 1 ) ;
                    break ;

                case this_file::device_code_process_result::device_code_request_failed: 
                {
                    motor::log::global_t::error("Please check the client_id in the credentials.") ;
                    motor::log::global_t::error( "Will reinitialize in 10 seconds." ) ;
                    _timeout = std::chrono::seconds( 10 ) ;
                    _ps = this_file::program_state::load_credentials ;
                    break ;
                }
                case this_file::device_code_process_result::ok: 
                    _ps = this_file::program_state::need_token ;
                    break ;
                }
                break ;
            }
            case this_file::program_state::need_token:
            {
                auto const res = this_t::request_user_token( _login_data ) ;
                switch ( res )
                {
                case this_file::request_user_token_result::curl_failed: 
                    exit(1) ;
                    break ;
                case this_file::request_user_token_result::invalid_device_code:
                    _ps = this_file::program_state::need_device_code ;
                    break ;
                    // lets give the user some time to authorize.
                    // then re-try user token request.
                case this_file::request_user_token_result::pending:
                    motor::log::global_t::status( "Please visit for authorization:") ;
                    motor::log::global_t::status( _dcr.verification_uri ) ;
                    _timeout = std::chrono::seconds( 10 ) ;
                    break ;
                    // unknown failure. Just redo everything and hope for the best.
                    // Twitch error status codes are not documented or are changing 
                    // based on error, so if the message can not be parsed correctly...
                case this_file::request_user_token_result::request_failed:
                    motor::log::global::error("Some error occurred when requesting a user token.") ;
                    motor::log::global::error( "Reinitializing process." ) ;
                    _ps = this_file::program_state::load_credentials ;
                    break ;
                    // Yay. Worked.
                case this_file::request_user_token_result::ok:
                    this_t::write_credentials( _login_data ) ;
                    _ps = this_file::program_state::need_login ;
                    break ;
                }
                break ;
            }
            case this_file::program_state::need_refresh: 
            {
                auto const res = this_t::refresh_token( _login_data, _mode ) ;
                switch ( res )
                {
                case this_file::refresh_process_result::ok:
                    _ps = this_file::program_state::need_login ;
                    break ;
                case this_file::refresh_process_result::curl_failed: break ;
                case this_file::refresh_process_result::invalid_client_id: break ;
                case this_file::refresh_process_result::invalid_token:
                    _ps = this_file::program_state::need_token ;
                    break ;
                case this_file::refresh_process_result::invalid: break ;

                }
                break ;
            }
            case this_file::program_state::need_login:
            {
                data_out =
                    "PASS oauth:" + _login_data.user_token + "\r\n"
                    "NICK aconstlink\r\n"
                    "JOIN #aconstlink\r\n"
                    "CAP REQ :twitch.tv/membership twitch.tv/tags twitch.tv/commands\r\n" ;

                _login_tp = clk_t::now() ;
                _ps = this_file::program_state::login_pending ;
                break ;
            }
            case this_file::program_state::login_pending:
            {
                if( std::chrono::duration_cast<std::chrono::seconds>( clk_t::now() - _login_tp ) >= 
                    std::chrono::seconds(5) ) 
                {
                    //_ps = this_file::program_state::need_login ;
                }
                break ;
            }}
        }

        //**********************************************************************************
        this_file::initial_process_result load_credentials_from_file( void_t ) noexcept
        {
            if ( !this_t::load_credentials( _login_data ) ||
                !this_t::validate_credentials( _login_data ) )
            {
                motor::log::global_t::status( "[twitch_irc_bot] : Twitch Client ID missing." ) ;
                return this_file::initial_process_result::credentials_failed ;
            }

            if ( _login_data.user_token.empty() ) 
            {
                return this_file::initial_process_result::no_token ;
            }

            return this_file::initial_process_result::ok ;
        }

    public:

        //**********************************************************************************
        twitch_irc_bot( this_file::code_grant_flow const mode, motor::io::database_mtr_safe_t db ) noexcept : 
            _db( motor::move(db) ), _mode( mode )
        {
            assert( _db != nullptr ) ;

            // test if curl is present in the system
            {
                auto const sys_res = std::system( "curl --version -s " ) ;
                if ( sys_res != 0 )
                {
                    motor::log::global::error( "curl required. Please install curl." ) ;
                    exit(1) ;
                }
            }

            this_t::update() ;
        }

        //**********************************************************************************
        virtual ~twitch_irc_bot( void_t ) noexcept{}

        //**********************************************************************************
        virtual motor::network::user_decision on_connect( motor::network::connect_result const res, size_t const tried ) noexcept
        {
            motor::log::global_t::status( "Connection : " + motor::network::to_string( res ) ) ;
            if( tried > 2 ) std::this_thread::sleep_for( std::chrono::seconds(2) ) ;

            if( res == motor::network::connect_result::established )
            {
                this_t::update() ;
            }
            else if( res == motor::network::connect_result::closed )
            {
                _ps = this_file::program_state::need_login ;
            }

            return motor::network::user_decision::keep_going ;
        }

        //**********************************************************************************
        virtual motor::network::user_decision on_sync( void_t ) noexcept
        {
            std::this_thread::sleep_for( std::chrono::milliseconds(10) ) ;
            return motor::network::user_decision::keep_going ;
        }

        //**********************************************************************************
        virtual motor::network::user_decision on_update( void_t ) noexcept
        {
            this_t::update() ;

            return motor::network::user_decision::keep_going ;
        }

        //**********************************************************************************
        virtual void_t on_receive( byte_cptr_t buffer, size_t const sib ) noexcept
        {
            data_in += motor::string_t( (char_cptr_t)buffer, sib ) ;
        }

        //**********************************************************************************
        virtual void_t on_received( void_t ) noexcept
        {
            parser.parse( data_in ) ;

            if( _ps == this_file::program_state::login_pending )
            {
                _ps = this_file::program_state::bot_is_online ;
            }

            parser.for_each( [&] ( motor::social::twitch::irc_command const c, 
                motor::social::twitch::tags_cref_t tags, motor::string_in_t param )
            {
                if ( c == motor::social::twitch::irc_command::ping )
                {
                    _pong = "PONG " + motor::string_t( ":" ) + param + "\r\n" ;
                    _require_pong = true ;
                    return false ;
                }
                else if ( c == motor::social::twitch::irc_command::notice )
                {
                    auto const r0 = param.find( "Login" ) ;
                    auto const r1 = param.find( "authentication") ;
                    auto const r2 = param.find( "failed" ) ;
                    
                    if( r1 != std::string::npos && r2 != std::string::npos )
                    {
                        _ps = this_file::program_state::need_refresh ;
                    }
                    
                    return false ;
                }
                else if( c == motor::social::twitch::irc_command::privmsg )
                {
                    size_t const pos = param.find_first_of( '!' ) ;
                    if( pos != std::string::npos && pos == 0 )
                    {
                        auto const com = param.substr( 1, param.size() - 1 ) ;
                        if( com == "echo" )
                        {
                            auto const user = tags.find( "display-name" ) ;
                            if( user != tags.end() )
                                data_out = "PRIVMSG #aconstlink : HeyGuys " + user->second + "\r\n" ;
                        }
                        else if ( com == "commands" )
                        {
                            data_out = "PRIVMSG #aconstlink : !commands !echo !discord \n" ;
                        }
                        else if( com == "discord" )
                        {
                            data_out = "PRIVMSG #aconstlink : https://discord.gg/z7yfXYBY\r\n" ;
                        }
                        else 
                        {
                            data_out = "PRIVMSG #aconstlink : NotLikeThis unrecognized command. Try !commands\r\n" ;
                        }
                    }
                }
                else if( c == motor::social::twitch::irc_command::part )
                {
                }
                else
                {
                    int bp = 0 ;
                }

                return true ;
            } ) ;

            parser.clear() ;

            motor::log::global_t::status( data_in ) ;
            data_in.clear() ;
        }

        //**********************************************************************************
        virtual void_t on_send( byte_cptr_t & buffer, size_t & num_sib ) noexcept
        {
            if ( _require_pong )
            {
                buffer = byte_ptr_t( _pong.c_str() ) ;
                num_sib = _pong.size() ;
                _require_pong = false ;
            }

            if( data_out.size() != 0 )
            {
                buffer = byte_ptr_t( data_out.c_str() );
                num_sib = data_out.size() ;

                return ;
            }
        }

        //**********************************************************************************
        virtual void_t on_sent( motor::network::transmit_result const ) noexcept
        {
            data_out.clear() ;
        }
    };
}

int main( int argc, char ** argv )
{
    auto mod = motor::platform::network_module_creator::create() ;

    motor::io::database_mtr_t db = motor::shared( 
        motor::io::database( motor::io::path_t( DATAPATH ), "./working", "data" ) ) ;

    // platform not supported
    if ( mod == nullptr ) return 1 ;

    auto mtr = motor::shared( this_file::twitch_irc_bot( this_file::code_grant_flow::device_code_flow, motor::move( db ) ) ) ;

    mod->create_tcp_client( motor::network::create_tcp_client_info { 
        "my_client", 
        motor::network::ipv4::binding_point_host { "6667", "irc.chat.twitch.tv"},
        //motor::network::ipv4::binding_point_host { "80", "irc-ws.chat.twitch.tv" },
        motor::move( mtr ) } ) ;
    
    while ( !done )
    {
        //motor::log::global_t::status( "waiting in main loop" ) ;
        std::this_thread::sleep_for( std::chrono::seconds( 2 ) ) ;
    }

    return 0 ;
}