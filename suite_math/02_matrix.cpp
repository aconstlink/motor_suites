
#include <motor/math/matrix/matrix2.hpp>
#include <motor/math/matrix/matrix3.hpp>
#include <motor/math/matrix/matrix4.hpp>

int main( int argc, char ** argv )
{
    {
        auto const m = motor::math::mat3f_t::make_identity() ;
    }

    {
        auto const m = motor::math::mat4f_t::make_identity() ;
    }
    return 0 ;
}