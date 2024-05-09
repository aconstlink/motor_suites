
#include <motor/network/typedefs.h>
#include <motor/platform/network/network_module_creator.hpp>

#include <motor/std/vector>
#include <motor/log/global.h>

#include <thread>

using namespace motor::core::types ;

bool_t done = false ;

namespace this_file
{
    class my_server_handler : public motor::network::iserver_handler
    {
        struct client_data
        {
            motor::network::ipv4::address_t addr ;

            motor::string_t send_buffer ;
            motor::string_t recv_buffer ;
        };

        motor::vector< client_data > clients ;

        virtual motor::network::accept_result on_accept( motor::network::client_id_t const cid,
            motor::network::ipv4::address_cref_t addr ) noexcept
        {
            motor::log::global_t::status( "[my_server_handler] : on_accept" ) ;

            if ( clients.size() <= cid ) clients.resize( cid + 1 ) ;

            clients[cid] = client_data { addr } ;

            return motor::network::accept_result::ok ;
        }

        virtual void_t on_close( motor::network::client_id_t const ) noexcept
        {
            motor::log::global_t::status( "[my_server_handler] : client closed connection" ) ;
        }

        virtual motor::network::receive_result on_receive(
            motor::network::client_id_t const cid, byte_cptr_t byte, size_t const sib ) noexcept
        {
            auto & cd = clients[ cid ] ;

            cd.recv_buffer = motor::string_t( char_cptr_t(byte), sib ) ;

            motor::log::global_t::status(cd.recv_buffer ) ;

            return motor::network::receive_result::ok ;
        }

        virtual motor::network::transmit_result on_send(
            motor::network::client_id_t const cid, byte_cptr_t & buffer, size_t & num_sib ) noexcept
        {
            return motor::network::transmit_result::ok;
        }
    };
}

int main( int argc, char ** argv )
{
    auto * mod = motor::platform::network_module_creator::create() ;
    
    // platform not supported
    if ( mod == nullptr ) return 1 ;
    
    
    mod->create_tcp_server( { "my server", motor::network::ipv4::binding_point { 3000 }, 
        motor::shared( this_file::my_server_handler() ) } ) ;

    while ( !done )
    {
        //motor::log::global_t::status("waiting in main loop") ;
        std::this_thread::sleep_for( std::chrono::seconds( 2 ) ) ;
    }

    return 0 ;
}