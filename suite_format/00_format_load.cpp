
#include <motor/format/global.h>
#include <motor/io/database.h>
#include <motor/log/global.h>

using namespace motor::core::types ;

//
int main( int argc, char ** argv )
{
    // create a database for the project
    motor::io::database_mtr_t db = motor::shared( motor::io::database_t( motor::io::path_t( DATAPATH ), "./working", "data" ) ) ;

    motor::io::monitor_mtr_t mon = motor::shared( motor::io::monitor_t() ) ;

    // track file changes for re-import
    {
        db->attach( mon ) ;
    }

    // the ctor will register some default factories...
    motor::format::module_registry_mtr_t mod_reg = motor::format::global::register_default_modules( 
        motor::shared( motor::format::module_registry_t() ) ) ;

    // ... so this import will work by using the default module for png which might be the stb module.
    auto fitem = mod_reg->import_from( motor::io::location_t( "images.checker.png" ), motor::share( db ) ) ;

    // wait for the import to be finished which is async.
    if( auto * iir = dynamic_cast<motor::format::image_item_mtr_t>( fitem.get() ); iir != nullptr )
    {
        motor::log::global_t::status( "loaded image" ) ;
    }

    for( size_t i = 0; i < 10; ++i )
    {
        mon->for_each_and_swap( [&] ( motor::io::location_cref_t loc, motor::io::monitor_t::notify const n )
        {
            
        } ) ;

        std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) ) ;
    }

    motor::memory::release_ptr( mod_reg ) ;

    motor::io::global_t::deinit() ;
    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;

    return 0 ;
}
