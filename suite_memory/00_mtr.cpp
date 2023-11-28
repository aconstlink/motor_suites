
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

    void_t funk( test_class_mtr_shared_t x ) noexcept
    {
        // x needs to be copied
    }

    void_t funk( test_class_mtr_unique_t x ) noexcept
    {
        // ownership of x left here.
    }
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

    {
        auto o = motor::memory::global_t::create<this_file::test_class>() ;

        //motor::memory::global_t::release( o ) ;
        this_file::test_class_mtr_t o2 = motor::move( std::move(o) ).mtr() ;

        motor::memory::release_ptr( o2 ) ;
    }

    {
        auto o = motor::memory::global_t::create<this_file::test_class>() ;

        this_file::funk( motor::share( o ) ) ;
        this_file::funk( motor::unique( o ) ) ;

        motor::memory::release_ptr( o ) ;
    }

    motor::memory::global_t::dump_to_std() ;
    
    return 0 ;
}
