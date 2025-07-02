
#include <motor/memory/global.h>
#include <motor/log/global.h>

#include <motor/std/string>
#include <motor/std/vector>

namespace this_file
{
    using namespace motor::core::types ;

    class my_base 
    {
    };

    class my_class : public my_base
    {
        int_t _some_data = 0 ;

    public:

        my_class( void_t ) noexcept {}
        virtual ~my_class( void_t ) noexcept {}

        void_t set( int_t const d ) noexcept { _some_data = d ;}

    };
    motor_typedef( my_class ) ;
}


int main( int argc, char ** argv )
{
    // want to test the function pointer in the
    // memory global on init of the system
    {
        motor::memory::global_t::dump_to_std() ;
    }

    // want to test memory allocation and deallocation
    // of motor log system components.
    {
        //motor::memory::global_t::init() ;

        motor::log::global_t::init() ;
        motor::memory::global_t::dump_to_std() ;
        motor::log::global_t::deinit() ;

        motor::memory::global_t::dump_to_std() ;
    }

    // want to test motor std allocator with 
    // std string from the engine.
    {
        motor::string_t s ;

        s = "Hello World" ;

        motor::memory::global_t::dump_to_std() ;
    }

    // want to test more complex allocation scenario.
    {
        typedef motor::vector< this_file::my_class > my_vector_t ;

        // state the purpose in the ctor so memory tracking will be
        // simplified if memory leaks occurred and memory need to be 
        // observed. See memory dump and the end.
        my_vector_t some_data( my_vector_t::allocator_type(" *** some purpose *** ") ) ;
        some_data.reserve( 1000 ) ;
        some_data.reserve( 2000 ) ;
        some_data.reserve( 5000 ) ;
        motor::memory::global_t::dump_to_std() ;
    }

    // deinit log system so ...
    motor::log::global_t::deinit() ;

    // ... memory dump should show 0 sib allocated!
    return motor::memory::global_t::dump_to_std() > 0 ? 1 : 0 ;
}
