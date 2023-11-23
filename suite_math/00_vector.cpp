
#include <motor/math/vector/vector2.hpp>
#include <motor/math/vector/vector3.hpp>
#include <motor/math/vector/vector4.hpp>

int main( int argc, char ** argv )
{
    {
        auto const v = motor::math::vec2b_t() ;
    }

    {
        auto const v = motor::math::vec3f_t() ;
    }

    {
        auto const v = motor::math::vec4f_t() ;
    }
    return 0 ;
}