
#include <motor/io/database.h>
#include <motor/log/global.h>

using namespace motor::core::types ;

//
// test init/deinit db for memory leak detection
//
int main( int argc, char ** argv )
{
    {
        motor::io::database db( motor::io::path_t( DATAPATH ), "./working", "data" ) ;
        db.dump_to_std() ;
    }

    motor::io::global_t::deinit() ;
    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;

    return 0 ;
}
