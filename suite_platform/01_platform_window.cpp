#include <motor/platform/global.h>
#include <motor/application/window/window_message_listener.h>

#include <motor/log/global.h>
#include <motor/memory/global.h>

#include <future>

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

            motor::log::global_t::status( "[test] : show window" ) ;

            wnd->send_message( motor::application::show_message( { true } ) ) ;
            std::this_thread::sleep_for( std::chrono::milliseconds(500) ) ;

            motor::log::global_t::status( "[test] : show window fullscreen" ) ;
            wnd->send_message( motor::application::fullscreen_message_t { 
                motor::application::three_state::on, motor::application::three_state::on } ) ;

            std::this_thread::sleep_for( std::chrono::milliseconds(500) ) ;

            motor::log::global_t::status( "[test] : show window windowed" ) ;
            wnd->send_message( motor::application::fullscreen_message_t{ 
                motor::application::three_state::on, motor::application::three_state::on  } ) ;
            std::this_thread::sleep_for( std::chrono::milliseconds(500) ) ;

            motor::log::global_t::status( "[test] : resize window #1" ) ;
            wnd->send_message( motor::application::resize_message_t( { true, 0, 0, true, 100, 100 } ) ) ;
            std::this_thread::sleep_for( std::chrono::milliseconds(500) ) ;

            motor::log::global_t::status( "[test] : resize window #2" ) ;
            wnd->send_message( motor::application::resize_message_t( { true, 100, 100, true, 300, 50 } ) ) ;
            std::this_thread::sleep_for( std::chrono::milliseconds(500) ) ;

            motor::log::global_t::status( "[test] : resize window #3" ) ;
            wnd->send_message( motor::application::resize_message_t( { true, 200, 500, true, 300, 300 } ) ) ;
            std::this_thread::sleep_for( std::chrono::milliseconds(500) ) ;

            motor::log::global_t::status( "[test] : resize window #4" ) ;
            for( int_t i=0; i<50; ++i )
            {
                wnd->send_message( motor::application::resize_message_t( { true, 200+2*i, 500, false, 0, 0 } ) ) ;
                std::this_thread::sleep_for( std::chrono::milliseconds(10) ) ;
            }

            motor::log::global_t::status( "[test] : resize window #5" ) ;
            for( int_t i=0; i<50; ++i )
            {
                wnd->send_message( motor::application::resize_message_t( { true, 200, 500+2*i, false, 0, 0 } ) ) ;
                std::this_thread::sleep_for( std::chrono::milliseconds(10) ) ;
            }
            

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

            motor::memory::release_ptr( wnd ) ;
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

    motor::log::global::deinit() ;
    motor::memory::global::dump_to_std() ;


    return ret ;
}