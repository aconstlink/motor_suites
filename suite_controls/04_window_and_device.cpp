#include <motor/platform/global.h>
#include <motor/application/window/window_message_listener.h>

#include <motor/log/global.h>
#include <motor/memory/global.h>

#include <motor/controls/types/ascii_keyboard.hpp>
#include <motor/controls/types/three_mouse.hpp>

#include <future>

int main( int argc, char ** argv )
{
    using namespace motor::core::types ;

    motor::application::carrier_mtr_t carrier = motor::platform::global_t::create_carrier() ;

    auto fut_update_loop = std::async( std::launch::async, [&]( void_t )
    {
        motor::application::window_message_listener_mtr_t msgl_out = motor::memory::create_ptr<
            motor::application::window_message_listener>( "[out] : message listener" ) ;

        motor::controls::ascii_device_mtr_t ascii_dev = nullptr ;
        motor::controls::three_device_mtr_t mouse_dev = nullptr ;

        // looking for device. It is managed, so pointer must be copied.
        carrier->device_system()->search( [&] ( motor::controls::device_borrow_t::mtr_t dev_in )
        {
            if( auto * ptr1 = dynamic_cast<motor::controls::three_device_mtr_t>(dev_in); ptr1 != nullptr && mouse_dev == nullptr )
            {
                mouse_dev = motor::share( ptr1 ) ;
            }
            else if( auto * ptr2 = dynamic_cast<motor::controls::ascii_device_mtr_t>(dev_in); ptr2 != nullptr && ascii_dev == nullptr )
            {
                ascii_dev = motor::share( ptr2 ) ;
            }
        } ) ;

        {
            motor::application::window_info_t wi ;
            wi.x = 100 ;
            wi.y = 100 ;
            wi.w = 800 ;
            wi.h = 600 ;

            auto wnd = carrier->create_window( wi ) ;
            wnd->register_out( motor::share( msgl_out ) ) ;
            wnd->send_message( motor::application::show_message( { true } ) ) ;

            {
                while( true ) 
                {
                    std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) ) ;
                
                    motor::application::window_message_listener_t::state_vector_t sv ;
                    if( msgl_out->swap_and_reset( sv ) )
                    {
                        if( sv.close_changed )
                        {
                            break ;
                        }
                    }

                    // must be done along with the user thread due
                    // to synchronization issues with any device 
                    carrier->update_device_system() ;

                    // mouse
                    if( mouse_dev != nullptr )
                    {
                        // test buttons
                        {
                            motor::controls::types::three_mouse_t mouse( mouse_dev ) ;

                            auto button_funk = [&] ( motor::controls::types::three_mouse_t::button const button )
                            {
                                if( mouse.is_pressed( button ) )
                                {
                                    motor::log::global_t::status( "button pressed: " + motor::controls::types::three_mouse_t::to_string( button ) ) ;
                                    return true ;
                                }
                                else if( mouse.is_pressing( button ) )
                                {
                                    motor::log::global_t::status( "button pressing: " + motor::controls::types::three_mouse_t::to_string( button ) ) ;
                                    return true ;
                                }
                                else if( mouse.is_released( button ) )
                                {
                                    motor::log::global_t::status( "button released: " + motor::controls::types::three_mouse_t::to_string( button ) ) ;
                                }
                                return false ;
                            } ;

                            
                            {
                                auto const l = button_funk( motor::controls::types::three_mouse_t::button::left ) ;
                                auto const r = button_funk( motor::controls::types::three_mouse_t::button::right ) ;
                                button_funk( motor::controls::types::three_mouse_t::button::middle ) ;

                                // test coords
                                {
                                    static bool_t show_coords = false ;
                                    if( mouse.is_released( motor::controls::types::three_mouse_t::button::right ) )
                                    {
                                        show_coords = !show_coords ;
                                    }
                                    if( show_coords )
                                    {
                                        auto const locals = mouse.get_local() ;
                                        auto const globals = mouse.get_global() ;

                                        motor::log::global_t::status(
                                            "local : [" + motor::from_std( std::to_string( locals.x() ) )+ ", " +
                                            motor::from_std( std::to_string( locals.y() ) ) + "]"
                                        ) ;

                                        motor::log::global_t::status(
                                            "global : [" + motor::from_std( std::to_string( globals.x() ) ) + ", " +
                                            motor::from_std( std::to_string( globals.y() ) ) + "]"
                                        ) ;
                                    }
                                }

                                // test scroll
                                {
                                    float_t s ;
                                    if( mouse.get_scroll( s ) )
                                    {
                                        motor::log::global_t::status( "scroll : " + motor::from_std( std::to_string( s ) ) ) ;
                                    }
                                }
                            }
                        }
                    }

                    // keyboard
                    if( ascii_dev != nullptr )
                    {
                        {
                            motor::controls::types::ascii_keyboard_t keyboard( ascii_dev ) ;
            
                            using layout_t = motor::controls::types::ascii_keyboard_t ;
                            using key_t = layout_t::ascii_key ;
            
                            for( size_t i=0; i<size_t(key_t::num_keys); ++i )
                            {
                                auto const ks = keyboard.get_state( key_t( i ) ) ;
                                if( ks != motor::controls::components::key_state::none ) 
                                {
                                    motor::log::global_t::status( layout_t::to_string( key_t(i) ) + " : " + 
                                        motor::controls::components::to_string(ks) ) ;
                                }
                            }

                            {
                                auto const ks = keyboard.get_state( key_t::escape ) ;
                                if( ks == motor::controls::components::key_state::released )
                                    break ;
                            }
                        }        
                    }
                }
            }

            motor::memory::release_ptr( wnd ) ;
        }

        motor::memory::release_ptr( ascii_dev ) ;
        motor::memory::release_ptr( mouse_dev ) ;
        
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