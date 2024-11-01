

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
    // simple variable size
    {
        int sib0 = sizeof( motor::wire::any*) ;
        int sib1 = sizeof( motor::wire::floatv_t ) ;
        int sib2 = sizeof( motor::wire::vec3fv_t ) ;
        int sib3 = sizeof( motor::wire::trafo3fv_t ) ;
        int bp = 0 ;
    }

    // detailed variable size
    {
        int sib0 = sizeof( motor::wire::any * ) ;
        int sib2 = sizeof( motor::wire::vec3fvd_t ) ;
        int sib4 = sizeof( motor::wire::trafo3fvd_t ) ;
        int bp = 0 ;
    }

    // type checking
    // testing if variable is a simple one
    {
        motor::wire::any_ptr_t a = motor::shared( motor::wire::vec3fv_t( "my name" ) ) ;
        if( motor::wire::is_simple( a ) )
        {
            motor::log::global::status("this vec3 is a simple variable") ;
        }
        else 
        {
            motor::log::global::status("this vec3 is not a simple variable") ;
        }

        motor::release( motor::move( a ) ) ;
    }

    // type checking
    // testing if variable is a detailed one
    {
        motor::wire::any_ptr_t a = motor::shared( motor::wire::vec3fvd_t( "my name" ) ) ;
        if ( motor::wire::is_detailed( a ) )
        {
            motor::log::global::status( "this vec3 is a detailed variable" ) ;
        }
        else
        {
            motor::log::global::status( "this vec3 is not a detailed variable" ) ;
        }

        motor::release( motor::move( a ) ) ;
    }

    // test the underlying type
    {
        motor::wire::any_ptr_t a = motor::shared(  motor::wire::vec3fv_t( "my name" ) ) ;
        motor::wire::any_ptr_t b = motor::shared(  motor::wire::vec3fvd_t( "my name" ) ) ;

        bool const boola = motor::wire::is_type_compatible<motor::math::vec3f_t>( a ) ;
        bool const boolb = motor::wire::is_type_compatible<motor::math::vec3f_t>( b ) ;

        if ( boola && boolb)
        {
            motor::log::global::status( "simple and detailed vec3 variables are compatible" ) ;
        }

        motor::release( motor::move( a ) ) ;
        motor::release( motor::move( b ) ) ;
    }

    // test the underlying type
    {
        motor::wire::any_ptr_t a = motor::shared(  motor::wire::vec3fv_t( "my name" ) ) ;
        motor::wire::any_ptr_t b = motor::shared(  motor::wire::vec2fvd_t( "my name" ) ) ;

        bool const boola = motor::wire::is_type_compatible<motor::math::vec3f_t>( a ) ;
        bool const boolb = motor::wire::is_type_compatible<motor::math::vec3f_t>( b ) ;

        if ( boola )
        {
            motor::log::global::status( "simple and detailed vec3 variables are compatible" ) ;
        }

        if ( !boolb )
        {
            motor::log::global::status( "vec3 and vec2 variables are not compatible" ) ;
        }

        motor::release( motor::move( a ) ) ;
        motor::release( motor::move( b ) ) ;
    }

    // define simple float variable
    {
        motor::wire::floatv_t my_float( "my float variable" )  ;
    }

    // define simple vec3 variable
    // and inspect its sub-members,
    // there are none...
    {
        motor::log::global_t::status( "********** Inspecting a simple vec3 variable" ) ;

        motor::wire::vec3fv_t var( "pos" ) ;
        var.inspect( [&] ( motor::wire::any_ref_t, motor::wire::any::member_info_in_t info )
        {
            motor::log::global_t::status( info.full_name ) ;
        } ) ;

        motor::log::global_t::status( "***********************************" ) ;
    }

    // ... otherwise define detailed vec3 variable
    // and inspect its sub-members
    {
        motor::log::global_t::status( "********** Inspecting a detailed vec3 variable" ) ;

        motor::wire::vec3fvd_t var( "pos" ) ;
        var.inspect( [&]( motor::wire::any_ref_t, motor::wire::any::member_info_in_t info )
        {
            motor::log::global_t::status(info.full_name) ;
        } ) ;

        motor::log::global_t::status( "***********************************" ) ;
    }

    // define detailed trafo 3d variable
    // and inspect its sub-members
    {
        motor::log::global_t::status( "********** Inspecting a detailed trafo 3d variable" ) ;

        motor::wire::trafo3fvd_t var( "trafo" ) ;
        var.inspect( [&] ( motor::wire::any_ref_t, motor::wire::any::member_info_in_t info )
        {
            motor::log::global_t::status( info.full_name ) ;
        } ) ;

        motor::log::global_t::status( "***********************************" ) ;
    }

    // test inputs
    {
        motor::wire::trafo3fvd_t var( "trafo" ) ;
        auto inputs = var.inputs() ;
        int bp = 0 ;
    }

    // test connection
    {
        motor::math::m3d::trafof_t const tin( 
            motor::math::vec3f_t( 1.0f, 2.0f, 3.0f), 
            motor::math::vector_is_not_normalized( motor::math::vec4f_t( 1.0f, 1.0f, 0.0f, motor::math::angle<float_t>::degree_to_radian( 45.0f ) ) ), 
            motor::math::vec3f_t( 10.0f, 20.0f, 30.0f ) ) ;

        motor::wire::trafo3fvd_t var( "trafo", tin ) ;

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