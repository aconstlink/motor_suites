
#include <motor/io/global.h>
#include <motor/log/global.h>

#include <chrono>

using namespace motor::core::types ;

// start operations async and wait for it.
int main( int argc, char ** argv )
{
    motor::io::system_t::load_handle_t h1 ;
    motor::io::system_t::store_handle_t h2 ;
    motor::io::system_t::load_handle_t h3 ;

    // #1 load some data
    {
        motor::io::path_t const p = motor::string_t( DATAPATH ) + "/working/some_info.txt"  ;
        h1 = motor::io::global_t::load( p, std::launch::async ) ;
    }

    motor::string_t store_data("stored from program says HELLO3" ) ;

    // #2 store some data while #1
    {
        motor::io::path_t const p = motor::string_t( DATAPATH ) + "/working/some_info2.txt"  ;
        h2 = motor::io::global_t::store( p, store_data.c_str(), store_data.size(), std::launch::async ) ;
    }

    std::this_thread::sleep_for( std::chrono::milliseconds(1000) ) ;

    {
        h1.wait_for_operation( [&]( char_cptr_t d, size_t const sib, motor::io::result const res )
        {
            if( res != motor::io::result::ok ) return ;
            motor::log::global_t::status( motor::string_t( d, sib ) ) ;
        } ) ;
    }

    {
        h2.wait_for_operation( [&]( motor::io::result const res )
        {
            motor::log::global_t::status( "store operation result : " + motor::io::to_string( res ) ) ;
        } ) ;
    }

    // #3 load some data from store. Must wait until store is done
    {
        motor::io::path_t const p = motor::string_t( DATAPATH ) + "/working/some_info2.txt"  ;
        h3 = motor::io::global_t::load( p, std::launch::deferred ) ;
    }

    {
        h3.wait_for_operation( [&]( char_cptr_t d, size_t const sib, motor::io::result const res )
        {
            if( res != motor::io::result::ok ) return ;
            motor::log::global_t::status( motor::string_t( d, sib ) ) ;
        } ) ;
    }

    motor::io::global_t::deinit() ;
    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;

    return 0 ;
}
