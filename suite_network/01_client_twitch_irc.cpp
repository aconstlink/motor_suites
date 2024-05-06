


#include <motor/network/typedefs.h>
#include <motor/platform/network/network_module_creator.hpp>
#include <motor/social/twitch/irc_parser.hpp>

#include "tokens.hpp"

using namespace motor::core::types ;

bool_t done = false ;


namespace this_file
{
    class my_client : public motor::network::iclient_handler
    {
        motor::string_t data ;

        size_t _pass_send = 0 ;

        bool_t _require_pong = false ;
        motor::string_t _pong = "" ;

        motor::social::twitch::irc_parser_t parser ;

    public:

        my_client( void_t ) noexcept {}

        virtual void_t on_connect( motor::network::connect_result const res ) noexcept
        {
            motor::log::global_t::status( "Connection : " + motor::network::to_string( res ) ) ;
        }

        virtual void_t on_close( void_t ) noexcept
        {
            motor::log::global_t::status( "Connection closed" ) ;
        }

        virtual motor::network::receive_result on_receive(
            byte_cptr_t buffer, size_t const sib ) noexcept
        {
            motor::string_t message( (char_cptr_t)buffer, sib ) ;
            
            parser.parse( message ) ;
            
            parser.for_each( [&]( motor::social::twitch::irc_command const c, motor::string_in_t param )
            {
                if( c == motor::social::twitch::irc_command::ping )
                {
                    _pong = "PONG " + motor::string_t(":") + param ;
                    _require_pong = true ;
                    return false ;
                }
                return true ;
            } ) ;
            
            parser.clear() ;
            
            motor::log::global_t::status( message ) ;

            
            return motor::network::receive_result::ok ;
        }

        virtual motor::network::transmit_result on_send(
            byte_cptr_t & buffer, size_t & num_sib ) noexcept
        {
            if ( _pass_send == 0 )
            {
                data = "PASS oauth:"+ tokens::twitch() +"\r\n" ;
                buffer = byte_ptr_t( data.c_str() ) ;
                num_sib = data.size() ;
                ++_pass_send ;
                return motor::network::transmit_result::ok ;
            }
            else if ( _pass_send == 1 )
            {
                data = "NICK acl_bot\r\n" ;
                buffer = byte_ptr_t( data.c_str() ) ;
                num_sib = data.size() ;
                ++_pass_send ;
                return motor::network::transmit_result::ok ;
            }
            else if ( _pass_send == 2 )
            {
                data = "JOIN #aconstlink\r\n" ;
                buffer = byte_ptr_t( data.c_str() ) ;
                num_sib = data.size() ;
                ++_pass_send ;
                return motor::network::transmit_result::ok ;
            }
            #if 1
            else if ( _pass_send == 3 )
            {
                data = "CAP REQ :twitch.tv/membership twitch.tv/tags twitch.tv/commands\r\n" ;
                buffer = byte_ptr_t( data.c_str() ) ;
                num_sib = data.size() ;
                ++_pass_send ;
                return motor::network::transmit_result::ok ;
            }
            #else
            else if ( _pass_send == 3 ) ++_pass_send ;
            #endif

            if( _pass_send > 3 )
            {
                if( _require_pong )
                {
                    buffer = byte_ptr_t( _pong.c_str() ) ;
                    num_sib = _pong.size() ;
                    _require_pong = false ;
                }
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
            return motor::network::transmit_result::have_nothing ;
        }
    };
}

int main( int argc, char ** argv )
{
    auto * mod = motor::platform::network_module_creator::create() ;

    // platform not supported
    if ( mod == nullptr ) return 1 ;

    mod->create_tcp_client( motor::network::create_tcp_client_info { 
        "my_client", 
        motor::network::ipv4::binding_point_host { "6667", "irc.chat.twitch.tv"},
        //motor::network::ipv4::binding_point_host { "80", "irc-ws.chat.twitch.tv" },
        motor::shared( this_file::my_client() ) } ) ;

    while ( !done )
    {
        //motor::log::global_t::status( "waiting in main loop" ) ;
        std::this_thread::sleep_for( std::chrono::seconds( 2 ) ) ;
    }

    return 0 ;
}