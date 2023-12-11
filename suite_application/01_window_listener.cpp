

#include <motor/application/window/window.h>
#include <motor/application/window/window_message_listener.h>

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

    // #1 test in-listener
    {
        motor::application::window_message_listener_mtr_t lsn = motor::memory::create_ptr( 
            motor::application::window_message_listener_t(), "[test] : in listener #1" ) ;
        
        win->register_in( motor::unique(lsn) ) ;
    }

    // #2 test in-listener
    {
        motor::application::window_message_listener_mtr_t lsn = motor::memory::create_ptr( 
            motor::application::window_message_listener_t(), "[test] : in listener #2" ) ;
        
        win->register_in( motor::share(lsn) ) ;

        // passed shared ptr to function, so need to 
        // release our intance here
        motor::memory::release_ptr( lsn ) ;
    }

    // #3 test out-listener
    {
        motor::application::window_message_listener_mtr_t lsn = motor::memory::create_ptr( 
            motor::application::window_message_listener_t(), "[test] : out listener #1" ) ;
        
        win->register_out( motor::share(lsn) ) ;

        // passed shared ptr to function, so need to 
        // release our intance here
        motor::memory::release_ptr( lsn ) ;
    }

    // #4 test out-listener
    {
        motor::application::window_message_listener_mtr_t lsn = motor::memory::create_ptr( 
            motor::application::window_message_listener_t(), "[test] : out listener #2" ) ;
        
        win->register_out( motor::unique(lsn) ) ;
    }

    motor::memory::release_ptr( win ) ;

    motor::log::global::deinit() ;
    motor::memory::global::dump_to_std() ;

    return 0 ;
}