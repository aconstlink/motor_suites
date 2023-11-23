
#include <motor/math/vector/vector2.hpp>
#include <motor/math/vector/vector3.hpp>
#include <motor/math/vector/vector4.hpp>

#include <immintrin.h>

#include <iostream>
#include <array>

namespace tmath
{
    template< typename T >
    class vector2
    {
    };

    template<>
    class vector2< float >
    {
        motor_this_typedefs( vector2< float > ) ;

    private:

        // 2 floats
        __m64 _data ;

    } ;

    template< typename T >
    class vector4
    {
    };

    template<>
    class vector4< float >
    {
        motor_this_typedefs( vector4< float > ) ;

    private:

        // 4 floats
        __m128 _data = {0.0f, 0.0f, 0.0f, 0.0f};

    public:

        vector4( void ) noexcept {}
        vector4( float x, float y, float z, float w ) noexcept : _data({x,y,z,w}) {}
        vector4( std::array< float, 4 > const & d ) noexcept : _data( _mm_load_ps( d.data() ) ){}
        vector4( this_cref_t rhv ) noexcept : _data( rhv._data ) {}
        vector4( __m128 const d ) noexcept : _data(d) {}


    public:

        this_t operator + ( this_cref_t rhv ) const noexcept
        {
            return _mm_add_ps( _data, rhv._data ) ;
        }

        this_ref_t operator = ( this_cref_t rhv ) noexcept
        {
            _data = rhv._data ;
            return *this ;
        }
    };
    using vec4f_t = vector4< float > ;
}

int main( int argc, char ** argv )
{
    {
        std::array< float, 4 > d = {1.0f, 2.0f, 3.0f, 4.0f} ;
        tmath::vec4f_t v( d ) ;
        int bp = 0 ;
    }

    {
       tmath::vec4f_t v( 1.0f, 2.0f, 3.0f, 4.0f ) ;
    }

    {
       tmath::vec4f_t v1( 1.0f, 2.0f, 3.0f, 4.0f ) ;
       tmath::vec4f_t v2( 1.0f, 2.0f, 3.0f, 4.0f ) ;

       auto v3 = v1 + v2 ;

       int bp = 0 ;
    }

    return 0 ;
}