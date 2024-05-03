
#include <motor/network/typedefs.h>
#include <motor/platform/network/network_module_creator.hpp>

using namespace motor::core::types ;

bool_t done = false ;

namespace this_file
{
    class my_client : public motor::network::iclient_handler
    {
        motor::string_t data ;

        bool_t _pass_send = true ;

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
            return motor::network::receive_result::ok ;
        }

        virtual motor::network::transmit_result on_send(
            byte_cptr_t & buffer, size_t & num_sib ) noexcept
        {
            if ( !_pass_send )
            {
                return motor::network::transmit_result::ok ;
            }
            return motor::network::transmit_result::have_nothing ;
        }
    };
}

// need to transfer to client handler.
#if 0
auto const recv_funk = [&] ( byte_cptr_t data, size_t const sib ) -> motor::network::receive_result
{
    if ( !handshake_recv )
    {
        motor::string_t received( char_cptr_t( data ), sib ) ;
        if ( received != motor::string_t( "Hello Client" ) )
        {
            motor::log::global_t::status( "client handshake : failed" ) ;
            return motor::network::receive_result::close ;
        }

        motor::log::global_t::status( "client handshake : " + received ) ;

        handshake_recv = true ;
    }
    
    return motor::network::receive_result::ok ;
} ;
#endif

int main( int argc, char ** argv )
{
    auto * mod = motor::platform::network_module_creator::create() ;

    // platform not supported
    if ( mod == nullptr ) return 1 ;

    mod->create_tcp_client( motor::network::create_tcp_client_info { 
        "my_client", 
        motor::network::ipv4::binding_point_host { "3456", "localhost" },
        motor::shared( this_file::my_client() ) } ) ;

    while ( !done )
    {
        //motor::log::global_t::status( "waiting in main loop" ) ;
        std::this_thread::sleep_for( std::chrono::seconds( 2 ) ) ;
    }

    return 0 ;
}