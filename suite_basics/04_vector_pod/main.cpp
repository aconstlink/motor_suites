
#include <motor/memory/global.h>
#include <motor/log/global.h>

#include <motor/math/vector/vector2.hpp>

#include <motor/std/string>
#include <motor/std/vector_pod>

#include <motor/memory/global.h>

namespace this_file
{
    using namespace motor::core::types ;

    struct my_struct
    {
        void_ptr_t ptr = nullptr ;
        motor::math::vec2f_t vec ;
    };
    using my_struct_vector_pod_t = motor::vector_pod< my_struct > ;
}

int main( int argc, char ** argv )
{
    {
        this_file::my_struct_vector_pod_t vec( 100 ) ;
        for ( size_t i = 0; i < vec.size(); ++i )
        {
            vec[ i ].ptr = motor::memory::global::alloc( 10 ) ;
        }

        // planty of time can pass

        for ( size_t i = 0; i < vec.size(); ++i )
        {
            motor::memory::global::dealloc( vec[ i ].ptr ) ;
        }
    }

    // erase entry
    {
        motor::vector_pod< size_t > some_value { 1, 2, 3, 4, 5, 6, 7, 8 } ;
        for( size_t i=0; i<some_value.size(); ++i ) std::printf( "%zx,", some_value[i] ) ;
        std::printf( "\n" ) ;
        some_value.erase( 5 ) ;
        for( size_t i=0; i<some_value.size(); ++i ) std::printf( "%zx,", some_value[i] ) ;
        std::printf( "\n" ) ;
    }

    motor::memory::global::dump_to_std() ;
    return 0 ;
}
