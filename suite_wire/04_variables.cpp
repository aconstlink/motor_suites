

#include <motor/math/utility/angle.hpp>
#include <motor/math/utility/3d/transformation.hpp>
#include <motor/math/vector/vector2.hpp>
#include <motor/math/vector/vector3.hpp>
#include <motor/math/vector/vector4.hpp>

#include <motor/wire/variable.hpp>
#include <motor/wire/variables/trafo_variables.hpp>
#include <motor/wire/variables/vector_variables.hpp>

#include <motor/log/global.h>

int main( int argc, char ** argv )
{
    // some sibs for curiosity
    {
        int sib0 = sizeof( motor::wire::any ) ;
        int sib2 = sizeof( motor::wire::vec2fv_t ) ;
        int sib4 = sizeof( motor::wire::trafo3fv_t ) ;
        int bp = 0 ;
    }
    
    // define simple float variable
    {
        motor::wire::floatv_t my_float( "my float variable" )  ;
    }

    // vec2
    {
        motor::log::global_t::status( "********** Inspecting a vec2 variable" ) ;

        motor::wire::vec2fv_t var( "pos" ) ;
        auto inputs = var.inputs() ;
        inputs.for_each_slot( [&]( motor::string_in_t name, motor::wire::iinput_slot_ptr_t )
        {
            motor::log::global_t::status( name ) ;
        } ) ;

        motor::log::global_t::status( "***********************************" ) ;
    }

    // vec3
    {
        motor::log::global_t::status( "********** Inspecting a vec3 variable" ) ;

        motor::wire::vec3fv_t var( "scale" ) ;
        auto inputs = var.inputs() ;
        inputs.for_each_slot( [&] ( motor::string_in_t name, motor::wire::iinput_slot_ptr_t )
        {
            motor::log::global_t::status( name ) ;
        } ) ;

        motor::log::global_t::status( "***********************************" ) ;
    }

    // vec4
    {
        motor::log::global_t::status( "********** Inspecting a vec4 variable" ) ;

        motor::wire::vec4fv_t var( "color" ) ;
        auto inputs = var.inputs() ;
        inputs.for_each_slot( [&] ( motor::string_in_t name, motor::wire::iinput_slot_ptr_t )
        {
            motor::log::global_t::status( name ) ;
        } ) ;

        motor::log::global_t::status( "***********************************" ) ;
    }

    {
        motor::log::global_t::status( "********** Inspecting a detailed trafo 3d variable" ) ;

        motor::wire::trafo3fv_t var( "trafo" ) ;
        auto inputs = var.inputs() ;
        inputs.for_each_slot( [&] ( motor::string_in_t name, motor::wire::iinput_slot_ptr_t )
        {
            motor::log::global_t::status( name ) ;
        } ) ;

        motor::log::global_t::status( "***********************************" ) ;
    }

    // test connection
    {
        motor::math::m3d::trafof_t const tin(
            motor::math::vec3f_t( 1.0f, 2.0f, 3.0f ),
            motor::math::vector_is_not_normalized( motor::math::vec4f_t( 1.0f, 1.0f, 0.0f, motor::math::angle<float_t>::degree_to_radian( 45.0f ) ) ),
            motor::math::vec3f_t( 10.0f, 20.0f, 30.0f ) ) ;

        motor::wire::trafo3fv_t trafo( "trafo", tin ) ;
        motor::wire::vec3fv_t pos( "position", tin.get_translation() ) ;

        {
            
            motor::wire::floatv_t other_var( "other" ) ;
            
            auto tinputs = trafo.inputs() ;
            auto pinputs = pos.inputs() ;

            pinputs.connect( "position.x", other_var.get_os() ) ;
            tinputs.connect( "trafo.position", pos.get_os() ) ;

            other_var.set_value( 1000.0f ) ;

            // will exchange and compute all new values
            // setting variable values does NOT automatically
            // pass through the new value.
            other_var.update() ;
        }

        pos.update() ;
        trafo.update() ;
        
        motor::wire::trafo3fv_t::value_t the_trafo =
            trafo.get_value() ;
    }

    motor::log::global::deinit() ;
    motor::memory::global::dump_to_std() ;

    return 0 ;
}