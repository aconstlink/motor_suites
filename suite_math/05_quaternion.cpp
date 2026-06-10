
#include <motor/math/quaternion/quaternion4.hpp>
#include <motor/math/vector/vector3.hpp>
#include <motor/math/interpolation/slerp.hpp>

#include <motor/math/utility/angle.hpp>

#include <limits>


int main( int argc, char ** argv )
{
    // rotate vectors 90°
    {
        auto const angle = motor::math::angle<float_t>::degree_to_radian(90.0f) ;
        auto const axis = motor::math::vec3f_t(1.0f,0.0f,0.0f) ;
        motor::math::quat4f_t const quat( angle, axis ) ;

        // should be X -> X
        {
            motor::math::vec3f_t v(1.0f, 0.0f, 0.0f ) ;
            auto const res = quat * v ;
            int bp = 0 ;
        }

        // should be Y -> Z
        {
            motor::math::vec3f_t v(0.0f, 1.0f, 0.0f ) ;
            auto const res = quat * v ;
            int bp = 0 ;
        }

        // should be Z -> ->
        {
            motor::math::vec3f_t v(0.0f, 0.0f, 1.0f ) ;
            auto const res = quat * v ;
            int bp = 0 ;
        }
    }

    // rotate vectors 90°
    {
        auto const angle = motor::math::angle<float_t>::degree_to_radian(90.0f) ;
        auto const axis = motor::math::vec3f_t(0.0f,0.0f,1.0f) ;
        motor::math::quat4f_t const quat( angle, axis ) ;

        // should be X -> Y
        {
            motor::math::vec3f_t v(1.0f, 0.0f, 0.0f ) ;
            auto const res = quat * v ;
            int bp = 0 ;
        }

        // should be Y -> -X
        {
            motor::math::vec3f_t v(0.0f, 1.0f, 0.0f ) ;
            auto const res = quat * v ;
            int bp = 0 ;
        }

        // should be Z -> Z
        {
            motor::math::vec3f_t v(0.0f, 0.0f, 1.0f ) ;
            auto const res = quat * v ;
            int bp = 0 ;
        }
    }

    // rotate vectors -90° about X
    {
        motor::math::quat4f_t const quat( 0.70710678f, -0.70710678f, 0.0f, 0.0f ) ;

        // should be X -> X
        {
            motor::math::vec3f_t v(1.0f, 0.0f, 0.0f ) ;
            auto const res = quat * v ;
            int bp = 0 ;
        }

        // should be Y -> -Z
        {
            motor::math::vec3f_t v(0.0f, 1.0f, 0.0f ) ;
            auto const res = quat * v ;
            int bp = 0 ;
        }

        // should be Z -> Y
        {
            motor::math::vec3f_t v(0.0f, 0.0f, 1.0f ) ;
            auto const res = quat * v ;
            int bp = 0 ;
        }
    }

    // rotate vectors 45°
    {
        auto const angle = motor::math::angle<float_t>::degree_to_radian(45.0f) ;
        auto const axis = motor::math::vec3f_t(1.0f,0.0f,0.0f) ;
        motor::math::quat4f_t const quat( angle, axis ) ;

        {
            motor::math::vec3f_t v(1.0f, 0.0f, 0.0f ) ;
            auto const res = quat * v ;
            int bp = 0 ;
        }

        {
            motor::math::vec3f_t v(0.0f, 1.0f, 0.0f ) ;
            auto const res = quat * v ;
            int bp = 0 ;
        }
    }

    // slerp
    {
        motor::math::quat4f_t q1(0.0f,1.0f,1.0f,0.0f) ;
        motor::math::quat4f_t q2(1.0f,1.0f,1.0f,0.0f) ;

        auto q3 = motor::math::interpolation< motor::math::quat4f_t >::linear( q1, q2, 0.5f ) ;
    }

    return 0 ;
}