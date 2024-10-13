
#include <motor/math/vector/vector2.hpp>
#include <motor/math/vector/vector3.hpp>
#include <motor/math/vector/vector4.hpp>

#include <motor/math/vector/normalization.hpp>

#include <limits>

namespace this_file
{
    using namespace motor::core::types ;

    static void_t some_funk( motor::math::is_normalized< motor::math::vec2f_t > const & nn ) noexcept
    {
        motor::math::vec2f_t v = nn ;

        assert( (v.normalized().length() - 1.0f) < std::numeric_limits< float_t >::epsilon() ) ; 
        
    }

    static void_t some_funk( motor::math::not_normalized< motor::math::vec2f_t > const & in ) noexcept
    {
        some_funk( motor::math::is_normalized< motor::math::vec2f_t >( in ) ) ;
    }

    
}
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

    {
        // does not work
        //motor::math::vec4f_t a = motor::math::vec3f_t() ;
        motor::math::vec4f_t const a( motor::math::vec3f_t(1.0f, 2.0f, 3.0f ) ) ;
        motor::math::vec4f_t b = a ;

        motor::math::vec3f_t c = motor::math::vec3f_t( b ) ;

        int bp = 0 ;
    }

    {
        this_file::some_funk( motor::math::vector_is_not_normalized( motor::math::vec2f_t( 1.0f, 2.0f ) ) ) ;
    }

    return 0 ;
}