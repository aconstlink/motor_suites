#include <motor/platform/global.h>
#include <motor/application/window/window_message_listener.h>

#include <motor/log/global.h>
#include <motor/device/global.h>
#include <motor/memory/global.h>

#include <future>

#include <motor/platform/application/win32/win32_carrier.h>

int main( int argc, char ** argv )
{
    using namespace motor::core::types ;

    motor::application::carrier_mtr_t carrier = motor::platform::global_t::create_carrier() ;

    auto fut_update_loop = std::async( std::launch::async, [&]( void_t )
    {
        motor::application::window_message_listener_mtr_t msgl_in = motor::memory::create_ptr<
            motor::application::window_message_listener>( "[in] : message listener" ) ;

        motor::application::window_message_listener_mtr_t msgl_out = motor::memory::create_ptr<
            motor::application::window_message_listener>( "[out] : message listener" ) ;

        {
            motor::application::window_info_t wi ;
            wi.x = 100 ;
            wi.y = 100 ;
            wi.w = 800 ;
            wi.h = 600 ;

            auto wnd = carrier->create_window( wi ) ;

            wnd->register_in( motor::share( msgl_in ) ) ;
            wnd->register_out( motor::share( msgl_out ) ) ;

            wnd->send_message( motor::application::show_message( { true } ) ) ;
            wnd->send_message( motor::application::cursor_message_t( {true} ) ) ;

            {
                while( true ) 
                {
                    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) ) ;
                
                    motor::application::window_message_listener_t::state_vector_t sv ;
                    if( msgl_out->swap_and_reset( sv ) )
                    {
                        if( sv.close_changed )
                        {
                            break ;
                        }
                    }
                }
            }
        }

        motor::memory::release_ptr( msgl_in ) ;
        motor::memory::release_ptr( msgl_out ) ;
    }) ;

    // end the program by closing the carrier
    auto fut_end = std::async( std::launch::async, [&]( void_t )
    {
        fut_update_loop.wait() ;

        carrier->close() ;
    } ) ;

    auto const ret = carrier->exec() ;
    
    motor::memory::release_ptr( carrier ) ;

    motor::device::global_t::deinit() ;
    motor::log::global::deinit() ;
    motor::memory::global::dump_to_std() ;


    return ret ;
}