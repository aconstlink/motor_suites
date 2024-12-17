


#include <motor/format/global.h>
#include <motor/format/aseprite/aseprite_module.h>

#include <motor/io/database.h>
#include <motor/log/global.h>

#include <motor/math/vector/vector4.hpp>

//
int main( int argc, char ** argv )
{
    motor::format::module_registry_mtr_t mod_reg = motor::format::global::register_default_modules( 
        motor::shared( motor::format::module_registry_t(), "mod registry"  ) ) ;
    
    {
        motor::io::database_t db = motor::io::database_t( motor::io::path_t( DATAPATH ), "./working", "data" ) ;

        // import
        {
            auto item = mod_reg->import_from( motor::io::location_t( "meshes.giraffe.obj" ), "wavefront", &db ) ;
        }
    }

    motor::memory::release_ptr( mod_reg ) ;

    motor::io::global_t::deinit() ;
    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;

    return 0 ;
}