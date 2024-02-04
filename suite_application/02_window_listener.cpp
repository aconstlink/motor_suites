

#include <motor/application/window/window.h>
#include <motor/application/window/window_message_listener.h>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <thread>
#include <future>
#include <chrono>

namespace this_file
{
    using namespace motor::core::types ;

    // ********************************************************************************************
    class my_window : public motor::application::window
    {
        using base_t = motor::application::window ;
        motor_this_typedefs( my_window ) ;

        motor::application::window_message_listener_mtr_t _lsn = nullptr ;

    public:
        
        virtual void_t check_for_messages( void_t ) noexcept 
        {
            motor::application::window_message_listener_t::state_vector sv ;
            if( _lsn->swap_and_reset( sv ) )
            {
                if( sv.resize_changed )
                {
                    motor::log::global_t::status("my_window received resize message") ;
                }

                if( sv.close_changed )
                {
                    motor::log::global_t::status("my_window received close message") ;
                }

                if( sv.dpi_msg_changed )
                {
                    motor::log::global_t::status("my_window received dpi message") ;
                }

                if( sv.fulls_msg_changed )
                {
                    motor::log::global_t::status("my_window received full screen message") ;
                }

                if( sv.msize_msg_changed )
                {
                    motor::log::global_t::status("my_window received screen size message") ;
                }

                if( sv.show_changed )
                {
                    motor::log::global_t::status("my_window received show message") ;
                }

                if( sv.vsync_msg_changed )
                {
                    motor::log::global_t::status("my_window received vsync message") ;
                }
            }
        }

    public:

        void_t emu_messages( void_t ) noexcept
        {
            // test message
            {
                motor::application::resize_message msg ;
                msg.x = 0 ;
                msg.y = 0 ;
                msg.w = 800 ;
                msg.w = 600 ;

                this->foreach_out( [&]( motor::application::iwindow_message_listener_mtr_t ptr ) 
                {
                    ptr->on_message( msg ) ;
                } ) ;
            }

            // test message
            {
                motor::application::vsync_message msg ;
                msg.on_off = true ;

                this->foreach_out( [&]( motor::application::iwindow_message_listener_mtr_t ptr ) 
                {
                    ptr->on_message( msg ) ;
                } ) ;
            }

            // test message
            {
                motor::application::fullscreen_message msg ;
                msg.on_off = true ;

                this->foreach_out( [&]( motor::application::iwindow_message_listener_mtr_t ptr ) 
                {
                    ptr->on_message( msg ) ;
                } ) ;
            }
        }

    public:

        my_window( void_t ) noexcept 
        {
            _lsn = motor::memory::create_ptr( 
                motor::application::window_message_listener_t(), "[test] : in listener #1" ) ;
            
            this->register_in( motor::share( _lsn ) ) ;
        }

        my_window( this_rref_t rhv ) noexcept : base_t( std::move( rhv ) )
        {
            _lsn = motor::move( rhv._lsn ) ;
        }

        ~my_window( void_t ) noexcept 
        {
            motor::memory::release_ptr( _lsn ) ;
        }
    };
    motor_typedef( my_window ) ;

    // ********************************************************************************************
    class my_app
    {
        motor_this_typedefs( my_app ) ;

    private:

        motor::application::window_message_listener_mtr_t _lsn = nullptr ;
        motor::application::window_mtr_t _wnd = nullptr ;

    public:

        my_app( motor::application::window_mtr_safe_t wnd ) noexcept : _wnd( motor::memory::copy_ptr(wnd) )
        {
            _lsn = motor::memory::create_ptr( 
                motor::application::window_message_listener_t(), "[test] : out listener" ) ;

            _wnd->register_out( motor::share( _lsn ) ) ;
        }

        my_app( this_rref_t rhv ) noexcept 
        {
            _lsn = motor::move( rhv._lsn ) ;
            _wnd = motor::move( rhv._wnd ) ;
        }

        ~my_app( void_t ) noexcept
        {
            motor::memory::release_ptr( _lsn ) ;
            motor::memory::release_ptr( _wnd ) ;
        }

        void_t update_send_to_window( void_t ) noexcept 
        {
            // test resize
            {
                motor::application::resize_message msg ;
                msg.x = 0 ;
                msg.y = 0 ;
                msg.w = 800 ;
                msg.w = 600 ;

                _wnd->foreach_in( [&]( motor::application::iwindow_message_listener_mtr_t ptr ) 
                {
                    ptr->on_message( msg ) ;
                } ) ;
            }
        }

        void_t update_receive_from_window( void_t ) noexcept 
        {
            motor::application::window_message_listener_t::state_vector sv ;
            if( _lsn->swap_and_reset( sv ) )
            {
                if( sv.resize_changed )
                {
                    motor::log::global_t::status("my_app received resize message") ;
                }

                if( sv.close_changed )
                {
                    motor::log::global_t::status("my_app received close message") ;
                }

                if( sv.dpi_msg_changed )
                {
                    motor::log::global_t::status("my_app received dpi message") ;
                }

                if( sv.fulls_msg_changed )
                {
                    motor::log::global_t::status("my_app received full screen message") ;
                }

                if( sv.msize_msg_changed )
                {
                    motor::log::global_t::status("my_app received screen size message") ;
                }

                if( sv.show_changed )
                {
                    motor::log::global_t::status("my_app received show message") ;
                }

                if( sv.vsync_msg_changed )
                {
                    motor::log::global_t::status("my_app received vsync message") ;
                }
            }
        }

    };
    motor_typedef( my_app ) ;

}

using namespace motor::core::types ;

//
// TEST : test window listener
//
int main( int argc, char ** argv )
{
    this_file::my_window_mtr_t wnd = motor::memory::create_ptr( 
        this_file::my_window_t(), "[test] : my window" ) ;

    this_file::my_app_mtr_t app = motor::memory::create_ptr(
        this_file::my_app_t( motor::share( wnd ) ), "[test] : my_app" ) ;

    
    bool_t done = false ;

    // window async:
    // update window: checks for incoming window messages
    // a platform window would need to translate those messages
    // to real platform window messages. usually this is done
    // near the platform application.
    auto fut_wnd = std::async( std::launch::async, [&]( void_t )
    {
        while( !done )
        {
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) ) ;
            //motor::log::global_t::status("update window") ;

            // other entity -> window
            wnd->check_for_messages() ;
            // window -> other entity
            wnd->emu_messages() ;
        }
    } ) ;

    // app async:
    // update app: a user app that uses the window handle to send
    // and receive window messages. 
    // send: app -> window
    // send: send user messages that could come from any app 
    //       logic to the platform window
    // recv: window -> app
    // recv: receive any platform window messages the app will 
    //       receive for further processing.
    auto fut_app = std::async( std::launch::async, [&]( void_t )
    {
        while( !done ) 
        {
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) ) ;
            //motor::log::global_t::status("update app") ;

            // app -> window
            app->update_send_to_window() ;
            // window -> app
            app->update_receive_from_window() ;
        }
    } ) ;

    // control time loop
    {
        using _clock_t = std::chrono::high_resolution_clock ;

        // start time 
        _clock_t::time_point tps = _clock_t::now() ;

        while( true ) 
        {
            auto sec = std::chrono::duration_cast< std::chrono::seconds>(_clock_t::now() - tps).count() ;
            if( sec > 3 ) break ;

            std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) ) ;
        }

        done = true ;

        fut_wnd.wait() ;
        fut_app.wait() ;
    }

    motor::memory::release_ptr( wnd ) ;
    motor::memory::release_ptr( app ) ;

    motor::log::global::deinit() ;
    motor::memory::global::dump_to_std() ;

    return 0 ;
}