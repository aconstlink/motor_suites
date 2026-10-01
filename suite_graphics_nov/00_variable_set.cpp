#include <motor/math/vector/vector3.hpp>
#include <motor/math/matrix/matrix2.hpp>

#include <motor/graphics/variable/variable_set.hpp>
#include <motor/graphics/variable/variable_set_view.hpp>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>


int main( int argc, char ** argv )
{
    using namespace motor::core::types ;

    {
        motor::graphics::variable_set_t vs ;
        
        vs.data_variable< float_t >( "test_float")->set( 1.0f ) ;
        vs.data_variable< motor::math::vec3f_t >( "test_vec3")->set( motor::math::vec3f_t() ) ;
        vs.data_variable< motor::math::mat2f_t >( "test_mat2")->set( motor::math::mat2f_t() ) ;
    }

    // At the moment, the variable set view is not usable, but I would like to 
    // safe it for a later point in development. 
    {
        motor::graphics::variable_set_view_t vsv ;

        
    }


    return 0 ;
}