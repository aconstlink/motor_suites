

#include <motor/geometry/mesh/tri_mesh.h>
#include <motor/geometry/mesh/flat_tri_mesh.h>
#include <motor/geometry/3d/cube.h>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>

#include <future>

int main( int argc, char ** argv )
{
    using namespace motor::core::types ;

    {
        motor::geometry::cube_t::input_params ip ;
        ip.scale = motor::math::vec3f_t( 1.0f ) ;
        ip.tess = 1 ;

        motor::geometry::tri_mesh_t tm ;
        motor::geometry::cube_t::make( &tm, ip ) ;

        motor::geometry::flat_tri_mesh_t ftm ;
        tm.flatten( ftm ) ;
    }

    return 0 ;
}
