


#include <motor/format/global.h>
#include <motor/scene/global.h>
#include <motor/scene/visitor/log_visitor.h>

#include <motor/io/database.h>
#include <motor/log/global.h>

#include <motor/math/vector/vector4.hpp>

// this test only tests formal correctness. It allows the programmer
// to step through the import process and observe the values and 
// structures created. 
// @todo in the future, here we could place some real tests for testing
// the correct importing proces.
int main( int argc, char ** argv )
{
    motor::scene::global::init() ;

    motor::format::module_registry_mtr_t mod_reg = motor::format::global::register_default_modules( 
        motor::shared( motor::format::module_registry_t(), "mod registry"  ) ) ;
    
    {
        motor::io::database_t db = motor::io::database_t( motor::io::path_t( DATAPATH ), "./working", "data" ) ;

        // import the gltf asset.
        {
            //auto item = mod_reg->import_from( motor::io::location_t( "gltf.2CylinderEngine.glTF.2CylinderEngine.gltf" ), &db ) ;
            //auto item = mod_reg->import_from( motor::io::location_t( "gltf.box.glTF.Box.gltf" ), &db ) ;
            auto item = mod_reg->import_from( motor::io::location_t( "gltf.simple_camera.simple_camera.gltf" ), &db ) ;

            auto * ret_item = item.get() ;

            // test scene with visitor
            if( auto * scene_item = dynamic_cast<motor::format::scene_item_ptr_t>( ret_item ); scene_item!= nullptr )
            {
                motor::scene::log_visitor_t lv ;
                auto * root = scene_item->root ;

                motor::scene::node_t::traverser( root ).apply( &lv ) ;
                
            }

            motor::release( motor::move( ret_item ) ) ;
        }
        
        // @todo make some real tests here.
        {
        }
    }

    motor::memory::release_ptr( mod_reg ) ;

    motor::scene::global::deinit() ;
    motor::io::global_t::deinit() ;
    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;

    return 0 ;
}