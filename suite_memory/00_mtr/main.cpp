
#include <motor/memory/global.h>
#include <motor/log/global.h>

#include <motor/std/string>
#include <motor/std/vector>

namespace this_file
{
    using namespace motor::core::types ;

    class test_class
    {
        motor::vector< int_t > _some_data ;

    public:

        test_class( void_t ) noexcept {}
        ~test_class( void_t ) noexcept {}

    };
    motor_typedef( test_class ) ;
}

// this test helped finding a bug in mtr release
// functionality.
int main( int argc, char ** argv )
{
    {
        auto o = motor::memory::global_t::create<this_file::test_class>() ;

        //motor::memory::global_t::release( o ) ;
        motor::memory::release_ptr( o ) ;
    }

    motor::memory::global_t::dump_to_std() ;
    
    return 0 ;
}
