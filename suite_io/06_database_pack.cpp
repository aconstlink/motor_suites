
#include <motor/io/database.h>
#include <motor/log/global.h>

using namespace motor::core::types ;

//
// this test packs a motor database file from a file system working directory
// and for a few seconds, the data monitor can be tested by saving files in the working dir.
//
int main( int argc, char ** argv )
{
    // 1. start database
    // 2. pack it
    {
        motor::io::database db( motor::io::path_t( DATAPATH ), "./working", "data" ) ;

        motor::string_t txt("Store before pack") ;
        {
            db.store( motor::io::location_t( "some_info2.txt" ), txt.c_str(), txt.size() ).wait_for_operation( [=]( motor::io::result const res )
            {
                motor::log::global_t::status( "before pack store result: " + motor::io::to_string(res) ) ;
            } ) ;
        }

        db.pack() ;

        {
            motor::string_t txt("Store after pack") ;
            {
                db.store( motor::io::location_t( "some_info2.txt" ), txt.c_str(), txt.size() ).wait_for_operation( [=]( motor::io::result const res )
                {
                    motor::log::global_t::status( "after pack store result: " + motor::io::to_string(res) ) ;
                } ) ;
            }
        }

        db.dump_to_std() ;
    }

    #if 0
    motor::io::global_t::deinit() ;
    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;
    #endif

    // read from packed database
    {
        motor::io::database db( motor::io::path_t( DATAPATH ), "./working", "data" ) ;
        {
            db.load( motor::io::location_t( "some_info2.txt" ) ).wait_for_operation( [&] ( char_cptr_t data, size_t const sib )
            {
                motor::log::global_t::status( "********************************" ) ;
                motor::log::global_t::status( "Loaded: " + motor::string_t( data, sib ) ) ;
            } ) ;
        }
    }

    #if 1
    motor::io::global_t::deinit() ;
    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;
    #endif

    return 0 ;
}
