

#include <motor/math/utility/angle.hpp>
#include <motor/math/utility/3d/transformation.hpp>
#include <motor/math/vector/vector2.hpp>
#include <motor/math/vector/vector3.hpp>
#include <motor/math/vector/vector4.hpp>

#include <motor/wire/variable.hpp>
#include <motor/wire/variables/trafo_variables.hpp>
#include <motor/wire/variables/vector_variables.hpp>

#include <motor/log/global.h>




namespace this_file
{

}

int main( int argc, char ** argv )
{
    {
        motor::wire::floatv_t my_float("some_name")  ;
    }

    {
        motor::wire::vec3fv_t var( "pos" ) ;
        var.inspect( [&]( motor::wire::any_ref_t, motor::wire::any::member_info_in_t info )
        {
            motor::log::global_t::status(info.full_name) ;
        } ) ;
    }

    {
        motor::wire::trafo3fv_t var( "trafo" ) ;
        var.inspect( [&] ( motor::wire::any_ref_t, motor::wire::any::member_info_in_t info )
        {
            motor::log::global_t::status( info.full_name ) ;
        } ) ;
    }

    // test inputs
    {
        motor::wire::trafo3fv_t var( "trafo" ) ;
        auto inputs = var.inputs() ;
        int bp = 0 ;
    }

    // test connection
    {
        motor::math::m3d::trafof_t const tin( 
            motor::math::vec3f_t( 1.0f, 2.0f, 3.0f), 
            motor::math::vector_is_not_normalized( motor::math::vec4f_t( 1.0f, 1.0f, 0.0f, motor::math::angle<float_t>::degree_to_radian( 45.0f ) ) ), 
            motor::math::vec3f_t( 10.0f, 20.0f, 30.0f ) ) ;

        motor::wire::trafo3fv_t var( "trafo", tin, motor::wire::sub_update_strategy::always ) ;

        // now connect a float variable to another
        // float which is within the trafo variable
        // i.e. position.x
        {
            auto inputs = var.inputs() ;
            motor::wire::floatv_t other_var( "other" ) ;
            
            inputs.connect( "trafo.position.x", other_var.get_os() ) ;
            
            other_var.set_value( 10.0f ) ;
            
            // will exchange and compute all new values
            // setting variable values does NOT automatically
            // pass through the new value.
            other_var.update() ;
        }
        
        var.update() ;
        
        #if 0
        motor::wire::trafo3fv_t::value_t the_trafo =
            var.get_value() ;
        #endif
    }

    motor::log::global::deinit() ;
    motor::memory::global::dump_to_std() ;

    return 0 ;
}