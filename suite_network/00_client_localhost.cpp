
#include <motor/network/typedefs.h>
#include <motor/platform/network/network_module_creator.hpp>

using namespace motor::core::types ;

bool_t done = false ;

bool_t handshake_recv = false ;
bool_t handshake_sent = false ;

auto const send_funk = [] ( byte_ptr_t ptr, size_t & send_sib, size_t const max_sib ) -> motor::network::transmit_result
{

    ptr[ 0 ] = 1 ;
    ptr[ 1 ] = 2 ;
    ptr[ 2 ] = 3 ;
    send_sib = 3 ;

    if ( handshake_recv && !handshake_sent )
    {

    }

    return motor::network::transmit_result::proceed ;
} ;

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

int main( int argc, char ** argv )
{
    auto * mod = motor::platform::network_module_creator::create() ;

    // platform not supported
    if ( mod == nullptr ) return 1 ;

    mod->create_tcp_client( motor::network::create_tcp_client_info { 
        "", 
        motor::network::ipv4::binding_point::localhost_at( 3456 ),
        send_funk, recv_funk } ) ;

    while ( !done )
    {
        //motor::log::global_t::status( "waiting in main loop" ) ;
        std::this_thread::sleep_for( std::chrono::seconds( 2 ) ) ;
    }

    return 0 ;
}