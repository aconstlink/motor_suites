

#include <motor/application/window/window.h>

#include <motor/log/global.h>
#include <motor/memory/global.h>

namespace this_file
{
    using namespace motor::core::types ;

    class my_window : public motor::application::window
    {
        // no instance calling this one here.
        virtual void_t check_for_messages( void_t ) noexcept 
        {

        }
    };
    motor_typedef( my_window ) ;
}


int main( int argc, char ** argv )
{
    this_file::my_window_mtr_t win = motor::memory::create_ptr( 
        this_file::my_window(), "[test] : my_window" ) ;

    // some testing here
    {
    }

    motor::memory::release_ptr( win ) ;

    motor::log::global::deinit() ;
    motor::memory::global::dump_to_std() ;

    return 0 ;
}