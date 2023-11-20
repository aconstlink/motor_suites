
#include <motor/io/database.h>
#include <motor/log/global.h>

using namespace motor::core::types ;

//
// test loading from db
//
int main( int argc, char ** argv )
{
    {
        motor::io::database db( motor::io::path_t( DATAPATH ), "./working", "data" ) ;
        {
            db.load( motor::io::location_t( "images.checker.png" ) ).wait_for_operation( [&] ( char_cptr_t data, size_t const sib ) 
            { 

                motor::log::global_t::status( "********************************" ) ;
                motor::log::global_t::status( "loaded images.checker with " + motor::from_std( std::to_string(sib) ) + " bytes" ) ;
            } ) ;
        }

        {
            db.load( motor::io::location_t( "some_info.txt" ) ).wait_for_operation( [&] ( char_cptr_t data, size_t const sib )
            {
                motor::log::global_t::status( "********************************" ) ;
                motor::log::global_t::status( "loaded some_info.txt with " + motor::from_std( std::to_string(sib) ) + " bytes" ) ;
            } ) ;
        }

        {
            db.load( motor::io::location_t( "meshes.text.obj" ) ).wait_for_operation( [&] ( char_cptr_t data, size_t const sib )
            {
                motor::log::global_t::status( "********************************" ) ;
                motor::log::global_t::status( "loaded meshes.text.obj with " + motor::from_std( std::to_string(sib) ) + " bytes" ) ;
            } ) ;
        }
    }

    motor::io::global_t::deinit() ;
    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;

    return 0 ;
}
