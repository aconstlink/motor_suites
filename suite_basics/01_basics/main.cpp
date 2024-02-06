
#include <motor/memory/global.h>
#include <motor/log/global.h>

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
    
    // it is borrowed. Not refcounted
    my_class_mtr_t do_something_value( my_class_mtr_t ptr )
    {
        ptr->set( 10 ) ;
        return ptr ;
    }

    // a safe_t pointer needs to be shared or unique.
    my_class_mtr_t do_something_value( my_class_mtr_safe_t ptr )
    {
        ptr->set( 10 ) ;
        return motor::memory::release_ptr( ptr ) ;
    }
}

int main( int argc, char ** argv )
{
    motor::log::global_t::init() ;

    {
        this_file::my_class_mtr_t some_data = 
            motor::memory::global_t::create<this_file::my_class_t>( "my object from main" ) ;

        this_file::do_something_value( some_data ) ;

        {
            auto * other = this_file::do_something_value( motor::share( some_data ) ) ;
            assert( other == some_data ) ;
        }

        {
            auto * other = this_file::do_something_value( motor::unique( some_data ) ) ;
            assert( other == nullptr ) ;
        }

        // some_data should be released by now.
        motor::memory::global_t::release( some_data ) ;
    }

    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;


    return 0 ;
}
