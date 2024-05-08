
#include <motor/network/typedefs.h>
#include <motor/platform/network/network_module_creator.hpp>

using namespace motor::core::types ;

bool_t done = false ;

namespace this_file
{
    class my_client : public motor::network::iclient_handler
    {
        motor::string_t data_in ;
        motor::string_t data_out ;

    public:

        my_client( void_t ) noexcept {}

        virtual ~my_client( void_t ) noexcept {}

        virtual motor::network::user_decision on_connect( motor::network::connect_result const res, size_t const ) noexcept
        {
            motor::log::global_t::status( "Connection : " + motor::network::to_string( res ) ) ;
            return motor::network::user_decision::keep_going ;
        }

        virtual motor::network::user_decision on_sync( void_t ) noexcept 
        {
            return motor::network::user_decision::keep_going ;
        }

        virtual motor::network::user_decision on_update( void_t ) noexcept
        {
            return motor::network::user_decision::keep_going ;
        }

        virtual void_t on_receive( byte_cptr_t buffer, size_t const sib ) noexcept
        {
            data_in += motor::string_t( (char_cptr_t) buffer, sib ) ;
        }

        virtual void_t on_received( void_t ) noexcept 
        {
            // do something with data_in ...
            // ... and then clear it
            data_in.clear() ;
        }

        virtual void_t on_send( byte_cptr_t & buffer, size_t & num_sib ) noexcept 
        {
        }

        virtual void_t on_sent( motor::network::transmit_result const ) noexcept 
        {
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
        motor::network::ipv4::binding_point_host { "3456", "localhost" },
        motor::shared( this_file::my_client() ) } ) ;

    while ( !done )
    {
        //motor::log::global_t::status( "waiting in main loop" ) ;
        std::this_thread::sleep_for( std::chrono::seconds( 2 ) ) ;
    }

    return 0 ;
}