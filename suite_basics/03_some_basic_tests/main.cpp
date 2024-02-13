
#include <motor/memory/global.h>
#include <motor/log/global.h>

#include <motor/std/string>
#include <motor/std/vector>

namespace this_file
{
    template< typename T >
    struct a
    {
        using ptr_t = T * ;

        ptr_t _ptr ;

        constexpr a( ptr_t p ) : _ptr(p){}
    };

    template< typename T >
    struct b
    {
        using ptr_t = T * ;
    };

    struct my_
    {
        int xyz ;
    };
    using my_a = a< my_ > ;
    using my_b = b< my_ > ;

    void some_funk( my_a v ){}
    void some_funk( my_b v ){}
}

int main( int argc, char ** argv )
{

    return 0 ;
}
