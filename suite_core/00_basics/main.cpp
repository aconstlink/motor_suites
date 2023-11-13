
#include <motor/core/memory/global.h>
#include <motor/core/log/global.h>

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
    
    // pass by value means that ptr IS ref counted 
    // in this function.
    my_class_mtr_t do_something_value( my_class_mtr_t ptr )
    {
        motor::memory::copy_ptr<my_class_t>( ptr )->set( 10 ) ;
        return motor::memory::release_ptr( ptr ) ;
    }

    // pass by reference means that ptr IS NOT ref counted 
    // in this function.
    my_class_mtr_t do_something_ref( my_class_mtr_ref_t ptr )
    {
        ptr->set( 10 ) ;
        return ptr ;
    }

    // pass by reference means that ptr IS NOT ref counted 
    // in this function. It takes over the managed pointer
    my_class_mtr_t do_something_take_over( my_class_mtr_rref_t ptr )
    {
        ptr->set( 10 ) ;
        return motor::memory::release_ptr( ptr ) ;
    }
}

int main( int argc, char ** argv )
{
    motor::memory::global_t::init() ;
    motor::log::global_t::init() ;

    {
        this_file::my_class_mtr_t some_data = 
            motor::memory::global_t::create<this_file::my_class_t>( "my object from main" ) ;

        this_file::do_something_value( some_data ) ;
        this_file::do_something_ref( some_data ) ;

        {
            auto * ptr = motor::memory::copy_ptr( some_data ) ;

            auto * other = this_file::do_something_take_over( 
                motor::memory::move_ptr( ptr ) ) ;
        }

        {
            auto * other = this_file::do_something_take_over( 
                motor::memory::move_ptr( motor::memory::copy_ptr( some_data ) ) ) ;
        }

        motor::memory::global_t::release( some_data ) ;
    }

    motor::memory::global_t::dump_to_std() ;


    return 0 ;
}
