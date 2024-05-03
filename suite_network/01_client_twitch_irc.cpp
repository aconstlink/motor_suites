
#include <motor/network/typedefs.h>
#include <motor/platform/network/network_module_creator.hpp>

using namespace motor::core::types ;

bool_t done = false ;


namespace this_file
{
    class my_client : public motor::network::iclient_handler
    {
        motor::string_t data ;

        size_t _pass_send = 0 ;

    public:

        my_client( void_t ) noexcept {

        }

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
            motor::log::global_t::status( message ) ;

            return motor::network::receive_result::ok ;
        }

        virtual motor::network::transmit_result on_send(
            byte_cptr_t & buffer, size_t & num_sib ) noexcept
        {
            #if 0
            //if ( !_pass_send )
            {
                data = "PASS oauth:3016fimc2m9nz0oqftji6f5wtuqod8\r\nNICK aconstlink_chatbot\r\n" ;
                buffer = byte_ptr_t( data.c_str() ) ;
                num_sib = data.size() ;
                _pass_send = true ;
                return motor::network::transmit_result::ok ;
            }
            #else
            if ( _pass_send == 100 )
            {
                data = "CAP REQ :twitch.tv/membership twitch.tv/tags twitch.tv/commands\r\n" ;
                buffer = byte_ptr_t( data.c_str() ) ;
                num_sib = data.size() ;
                ++_pass_send ;
                return motor::network::transmit_result::ok ;
            }
            else if ( _pass_send == 0 )
            {
                data = "PASS oauth:8qj8b15ogxlutt4994ingutukotmxm\r\n" ;
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