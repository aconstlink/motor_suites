
#include <motor/network/typedefs.h>
#include <motor/platform/network/network_module_creator.hpp>

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
            bool_t do_handshake_sent ;
            bool_t do_handshake_recv ;

            motor::string_t send_buffer ;
            motor::string_t recv_buffer ;
        };

        motor::vector< client_data > clients ;

        virtual motor::network::accept_result on_accept( motor::network::client_id_t const cid,
            motor::network::ipv4::address_cref_t addr ) noexcept
        {
            motor::log::global_t::status( "[my_server_handler] : on_accept" ) ;

            if ( clients.size() <= cid ) clients.resize( cid + 1 ) ;

            clients[cid] = client_data { addr, false, false } ;

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

            // handshake section
            if ( !cd.do_handshake_recv )
            {
                if ( cd.recv_buffer == motor::string_t( "ok" ) )
                {
                    cd.do_handshake_recv = true ;
                }
            }

            return motor::network::receive_result::ok ;
        }

        virtual motor::network::transmit_result on_send(
            motor::network::client_id_t const cid, byte_cptr_t & buffer, size_t & num_sib ) noexcept
        {
            auto & cd = clients[ cid ] ;

            // handshake section
            if ( !cd.do_handshake_sent )
            {
                motor::log::global_t::status( "[my_server_handler] : sending initial handshake" ) ;

                cd.send_buffer = motor::string_t( "Hello Client" ) ;
                buffer = (byte_cptr_t) cd.send_buffer.c_str() ;
                num_sib = cd.send_buffer.size() ;

                cd.do_handshake_sent = true ;
            }

            return motor::network::transmit_result::ok ;
        }
    };
}

int main( int argc, char ** argv )
{
    auto * mod = motor::platform::network_module_creator::create() ;
    
    // platform not supported
    if ( mod == nullptr ) return 1 ;
    
    
    mod->create_tcp_server( { "my server", motor::network::ipv4::binding_point { 3456 },
        motor::shared( this_file::my_server_handler() ) } ) ;

    while ( !done )
    {
        //motor::log::global_t::status("waiting in main loop") ;
        std::this_thread::sleep_for( std::chrono::seconds( 2 ) ) ;
    }

    return 0 ;
}