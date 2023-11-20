
#include <motor/io/global.h>
#include <motor/log/global.h>

using namespace motor::core::types ;

//
int main( int argc, char ** argv )
{

    // #1 load non-existing
    {
        motor::io::global_t::load( "" ).wait_for_operation( [&]( char_cptr_t d, size_t const sib, motor::io::result const res )
        {
            if( res != motor::io::result::ok ) return ;
            motor::log::global_t::status( motor::string_t( d, sib ) ) ;
        } ) ;
    }
    
    // #2 load some data
    {
        motor::io::path_t const p = motor::string_t( DATAPATH ) + "/working/some_info.txt"  ;
        motor::io::global_t::load( p ).wait_for_operation( [&]( char_cptr_t d, size_t const sib, motor::io::result const res )
        {
            if( res != motor::io::result::ok ) return ;
            motor::log::global_t::status( motor::string_t( d, sib ) ) ;
        } ) ;
    }

    // #3 store some data
    {
        motor::string_t store_data("stored from program says HELLO2" ) ;
        motor::io::path_t const p = motor::string_t( DATAPATH ) + "/working/some_info2.txt"  ;
        motor::io::global_t::store( p, store_data.c_str(), store_data.size() ).wait_for_operation( [&]( motor::io::result const res )
        {
            motor::log::global_t::status( "store operation result : " + motor::io::to_string( res ) ) ;
        } ) ;
    }

    // #4 load some data
    {
        motor::io::path_t const p = motor::string_t( DATAPATH ) + "/working/some_info2.txt"  ;
        motor::io::global_t::load( p ).wait_for_operation( [&]( char_cptr_t d, size_t const sib, motor::io::result const res )
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
