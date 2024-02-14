
#include <motor/io/database.h>
#include <motor/log/global.h>

using namespace motor::core::types ;

//
// this test packs a motor database file from a file system working directory
// and for a few seconds, the data monitor can be tested by saving files in the working dir.
//
int main( int argc, char ** argv )
{
    {
        motor::io::database db( motor::io::path_t( DATAPATH ), "./working", "data" ) ;

        {
            db.load( motor::io::location_t( "images.checker.png" ) ).wait_for_operation( [&] ( char_cptr_t data, size_t const sib, motor::io::result const ) 
            { 

                motor::log::global_t::status( "********************************" ) ;
                motor::log::global_t::status( "loaded images.checker with " + motor::from_std( std::to_string(sib) ) + " bytes" ) ;
            } ) ;
        }

        {
            db.load( motor::io::location_t( "some_info.txt" ) ).wait_for_operation( [&] ( char_cptr_t data, size_t const sib, motor::io::result const )
            {
                motor::log::global_t::status( "********************************" ) ;
                    motor::log::global_t::status( "loaded some_info.txt with " + motor::from_std( std::to_string(sib) ) + " bytes" ) ;
            } ) ;
        }

        {
            db.load( motor::io::location_t( "meshes.text.obj" ) ).wait_for_operation( [&] ( char_cptr_t data, size_t const sib, motor::io::result const )
            {
                motor::log::global_t::status( "********************************" ) ;
                    motor::log::global_t::status( "loaded meshes.text.obj with " + motor::from_std( std::to_string(sib) ) + " bytes" ) ;
            } ) ;
        }
    }

    motor::io::global_t::deinit() ;
    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;

    // 1. attach monitor before changes
    // 2. make a store operation
    // 3. a monitor should trigger
    {
        motor::io::monitor_mtr_t mon = motor::memory::create_ptr( motor::io::monitor_t() ) ;
        motor::io::database db( motor::io::path_t( DATAPATH ), "./working", "data" ) ;

        db.attach( motor::share( mon ) ) ;
        //db.attach( "data.some_info2", mon ) ;
        //db.attach( "subfolder.data", mon ) ;

        //db.pack() ;

        motor::string_t txt("Write through database") ;
        {
            db.store( motor::io::location_t( "some_info2.txt" ), txt.c_str(), txt.size() ).wait_for_operation( [=]( motor::io::result const res )
            {
                motor::log::global_t::status( "store result: " + motor::io::to_string(res) ) ;
            } ) ;
        }

        // test file changes
        size_t const checks = 4 ;
        for( size_t i=0; i<checks; ++i )
        {
            mon->for_each_and_swap( [&]( motor::io::location_cref_t loc, motor::io::monitor_t::notify const n )
            {
                motor::log::global_t::status( "[monitor] : Got " + motor::io::monitor_t::to_string(n) + " for " + loc.as_string() ) ;
            }) ;

            std::this_thread::sleep_for( std::chrono::milliseconds(1000) ) ;
        }

        db.detach( mon ) ;

        db.dump_to_std() ;
        motor::memory::release_ptr( mon ) ;
    }

    motor::io::global_t::deinit() ;
    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;

    return 0 ;
}
