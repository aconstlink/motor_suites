
#include <motor/math/vector/vector2.hpp>
#include <motor/math/vector/vector3.hpp>
#include <motor/math/vector/vector4.hpp>
#include <motor/math/matrix/matrix3.hpp>

#include <motor/math/vector/normalization.hpp>

#include <motor/math/utility/3d/ortho_basis.hpp>
#include <motor/math/camera/3d/camera_util.hpp>

#include <limits>


int main( int argc, char ** argv )
{
    // should be:
    // 1 0 0
    // 0 1 0
    // 0 0 1
    {
        motor::math::mat3f_t mat ;
        motor::math::m3d::orthonormal_basis< float_t>::create( motor::math::vec3f_t(0.0f,0.0f,1.0f), mat ) ;

        int bp = 0 ;
    }

    // should be:
    // 1 0 0
    // 0 1 0
    // 0 0 -1
    {
        motor::math::mat3f_t mat ;
        motor::math::m3d::orthonormal_basis< float_t>::create( motor::math::vec3f_t(0.0f,0.0f,-1.0f), mat ) ;

        int bp = 0 ;
    }

    // should be:
    // 1 0 0
    // 0 0 1
    // 0 1 0
    {
        motor::math::mat3f_t mat ;
        motor::math::m3d::orthonormal_basis< float_t >::create( motor::math::vec3f_t(0.0f,1.0f,0.0f), mat ) ;

        int bp = 0 ;
    }    

    // frame should be:
    // 1 0 0
    // 0 1 0
    // 0 0 -1
    {
        motor::math::mat4f_t mat ;
        motor::math::m3d::camera_util<float_t>::create_lookat( 
            motor::math::vec3f_t(0.0f,0.0f,500.0f), motor::math::vec3f_t(0.0f,0.0f,0.0f), mat ) ;

        int bp = 0;
    }

    return 0 ;
}