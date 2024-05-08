


#include <motor/network/typedefs.h>
#include <motor/platform/network/network_module_creator.hpp>
#include <motor/social/twitch/irc_parser.hpp>

#include "tokens.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <string>

using namespace motor::core::types ;

bool_t done = false ;


namespace this_file
{
    class twitch_irc_bot : public motor::network::iclient_handler
    {
        motor_this_typedefs( twitch_irc_bot ) ;

    private:

        motor::string_t data_out ;
        motor::string_t data_in ;

        bool_t _require_pong = false ;
        motor::string_t _pong = "" ;

        motor::social::twitch::irc_parser_t parser ;

        bool_t _need_login = true ;
        bool_t _need_refresh = false ;

        struct login_data
        {
            motor::string_t user_token ;
            motor::string_t refresh_token ;
            motor::string_t client_id ;
            motor::string_t client_secret ;
        };
        motor_typedef( login_data  ) ;


        login_data_t _login_data ;

    private:

        bool_t validate_token( login_data_cref_t ld ) const noexcept
        {
            auto const curl_validate_com =
                "curl -X GET https://id.twitch.tv/oauth2/validate "
                "-H \"Authorization: OAuth " + ld.user_token + "\" "
                "-s -o validate" ;

            std::system( curl_validate_com.c_str() ) ;

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

                        motor::log::global_t::status( "Twitch IRC Bot token validation: " ) ;
                        motor::log::global_t::status( "["+motor::to_string(code)+"] : " + msg ) ;
                        return false ;
                    }
                }
            }

            // clear content
            {
                std::ofstream ofs ;
                ofs.open( "validate", std::ofstream::out | std::ofstream::trunc );
                ofs.close();
            }

            return true ;
        }

        bool_t refresh_token( login_data_ref_t ld ) const noexcept
        {
            auto const curl_refresh_com =
                "curl -X POST https://id.twitch.tv/oauth2/token "
                "-H \"Content-Type: application/x-www-form-urlencoded\" "
                "-d \"grant_type=refresh_token&refresh_token=" + ld.refresh_token +
                "&client_id=" + ld.client_id +
                "&client_secret=" + ld.client_secret + "\" "
                "-s -o refresh_token";

            std::system( curl_refresh_com.c_str() ) ;

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
                //auto const data_string = data.dump() ;
                //motor::log::global_t::status( data[ "message" ] ) ;
                //motor::log::global_t::status(  ) ;

                if( data.contains("status") )
                {
                    size_t const code = data["status"] ;
                    motor::string_t msg ;
                    if( data.contains( "message" ) )
                    {
                        msg = data["message"] ;
                    }
                    motor::log::global_t::status( "Twitch IRC Bot token validation: " ) ;
                    motor::log::global_t::status( "[" + motor::to_string( code ) + "] : " + msg ) ;

                    return false ;
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

            return true ;
        }

        void_t validate_and_or_refresh_token( void_t ) noexcept
        {
            if ( !this_t::validate_token( _login_data ) )
            {
                if ( this_t::refresh_token( _login_data ) )
                {
                    motor::log::global_t::status( "[twitch_irc_bot] : Twitch Token refreshed." ) ;
                    _need_refresh = false ;
                }
            }
        }

    public:

        twitch_irc_bot( login_data_rref_t ld ) noexcept : _login_data(std::move( ld ) )
        {
            this_t::validate_and_or_refresh_token() ;
        }

        virtual motor::network::user_decision on_connect( motor::network::connect_result const res, size_t const tried ) noexcept
        {
            motor::log::global_t::status( "Connection : " + motor::network::to_string( res ) ) ;
            if( tried > 2 ) std::this_thread::sleep_for( std::chrono::seconds(2) ) ;

            if( res == motor::network::connect_result::established )
            {
                this_t::validate_and_or_refresh_token() ;
                _need_login = true ;
            }

            return motor::network::user_decision::keep_going ;
        }

        virtual motor::network::user_decision on_sync( void_t ) noexcept
        {
            std::this_thread::sleep_for( std::chrono::milliseconds(10) ) ;
            return motor::network::user_decision::keep_going ;
        }

        virtual motor::network::user_decision on_update( void_t ) noexcept
        {
            if( _need_refresh )
            {
                _need_login = this_t::refresh_token( _login_data ) ;
                _need_refresh = false ;
            }

            return motor::network::user_decision::keep_going ;
        }

        virtual void_t on_receive( byte_cptr_t buffer, size_t const sib ) noexcept
        {
            data_in += motor::string_t( (char_cptr_t)buffer, sib ) ;
        }

        virtual void_t on_received( void_t ) noexcept
        {
            parser.parse( data_in ) ;

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
                        _need_refresh = true ;
                        _need_login = true ;
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
                        else if ( com == "whisper" )
                        {
                            auto const iter = tags.find( "display-name" ) ;
                            if ( iter != tags.end() )
                            {
                                motor::string_t user = iter->second ;
                                data_out = ":" + user + "!" + user + "@" + user + ".tmi.twitch.tv WHISPER aconstlink :HeyGuys \r\n" ;
                            }
                                
                        }
                    }
                }
                else if( c == motor::social::twitch::irc_command::part )
                {

                }

                return true ;
            } ) ;

            parser.clear() ;

            motor::log::global_t::status( data_in ) ;
            data_in.clear() ;
        }

        virtual void_t on_send( byte_cptr_t & buffer, size_t & num_sib ) noexcept
        {
            if( _need_login )
            {
                data_out =
                    "PASS oauth:" + _login_data.user_token + "\r\n"
                    "NICK acl_bot\r\n"
                    "JOIN #aconstlink\r\n"
                    "CAP REQ :twitch.tv/membership twitch.tv/tags twitch.tv/commands\r\n" ;

                buffer = byte_ptr_t( data_out.c_str()  );
                num_sib = data_out.size() ;

                _need_login = false ;

                return ;
            }

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


            #if 0 // test send section
            
            else if ( _pass_send == 4 )
            {
                data = "PRIVMSG #aconstlink : HeyGuys <3 PartyTime\r\n" ;
                buffer = byte_ptr_t( data.c_str() ) ;
                num_sib = data.size() ;
                ++_pass_send ;
                return motor::network::transmit_result::ok ;
            }
            else if ( _pass_send == 5 )
            {
                data = "PRIVMSG #aconstlink :/color blue\r\n" ;
                buffer = byte_ptr_t( data.c_str() ) ;
                num_sib = data.size() ;
                ++_pass_send ;
                return motor::network::transmit_result::ok ;
            }
            else if ( _pass_send == 6 )
            {
                data = "PRIVMSG #aconstlink : HeyGuys <3 PartyTime test\r\n" ;
                buffer = byte_ptr_t( data.c_str() ) ;
                num_sib = data.size() ;
                ++_pass_send ;
                return motor::network::transmit_result::ok ;
            }
            #endif
        }

        virtual void_t on_sent( motor::network::transmit_result const ) noexcept
        {
            data_out.clear() ;
        }
    };
}

int main( int argc, char ** argv )
{
    auto * mod = motor::platform::network_module_creator::create() ;

    // platform not supported
    if ( mod == nullptr ) return 1 ;

    auto mtr = motor::shared( this_file::twitch_irc_bot(
    {
        tokens::user_access_token(),
        tokens::user_refres_token(),
        tokens::client_id(),
        tokens::client_secret()
    } ) ) ;

    mod->create_tcp_client( motor::network::create_tcp_client_info { 
        "my_client", 
        motor::network::ipv4::binding_point_host { "6667", "irc.chat.twitch.tv"},
        //motor::network::ipv4::binding_point_host { "80", "irc-ws.chat.twitch.tv" },
        motor::move( mtr ) } ) ;

    #if 0
    {
        auto const curl_refresh_com =
            "curl -X POST https://id.twitch.tv/oauth2/token "
            "-H \"Content-Type: application/x-www-form-urlencoded\" "
            "-d \"grant_type=refresh_token&refresh_token=" + tokens::user_refres_token() +
            "&client_id=" + tokens::client_id() +
            "&client_secret=" + tokens::client_secret() + "\" "
            "-s -o test.txt ";

        std::system( curl_refresh_com.c_str() ) ;

        std::ifstream myfile( "test.txt" );
        std::string response ;
        std::string line;
        if ( myfile.is_open() )
        {
            while ( std::getline ( myfile, line ) )
            {
                response += line ;
            }
            myfile.close();
            nlohmann::json data = nlohmann::json::parse( response ) ;
            auto const data_string = data.dump() ;
            motor::log::global_t::status( data[ "message" ] ) ;
            //motor::log::global_t::status(  ) ;

            if ( data[ "status" ].is_string() )
            {
            }
            int const bp = 0 ;
        }
    }
    
    #endif
    while ( !done )
    {
        //motor::log::global_t::status( "waiting in main loop" ) ;
        std::this_thread::sleep_for( std::chrono::seconds( 2 ) ) ;
    }

    return 0 ;
}