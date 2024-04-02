#include <motor/platform/global.h>
#include <motor/application/window/window_message_listener.h>

#include <motor/log/global.h>
#include <motor/memory/global.h>

#include <motor/controls/types/xbox_controller.hpp>

#include <future>

int main( int argc, char ** argv )
{
    using namespace motor::core::types ;

    motor::application::carrier_mtr_t carrier = motor::platform::global_t::create_carrier() ;

    auto fut_update_loop = std::async( std::launch::async, [&]( void_t )
    {
        motor::application::window_message_listener_mtr_t msgl_out = motor::memory::create_ptr<
            motor::application::window_message_listener>( "[out] : message listener" ) ;

        motor::controls::xbc_device_mtr_t gp_dev = nullptr ;

        // looking for device. It is managed, so pointer must be copied.
        carrier->device_system()->search( [&] ( motor::controls::device_borrow_t::mtr_t dev_in )
        {
            // find the first one
            if( auto * ptr1 = dynamic_cast<motor::controls::xbc_device_mtr_t>(dev_in); ptr1 != nullptr && gp_dev == nullptr )
            {
                gp_dev = motor::share( ptr1 ) ;
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
                    if( gp_dev != nullptr )
                    {
                        motor::controls::types::xbox_controller_t ctrl( gp_dev ) ;

                        // buttons
                        {
                            auto button_funk = [&] ( motor::controls::types::xbox_controller_t::button const button )
                            {
                                static bool_t pressing = false ;

                                if( ctrl.is( button, motor::controls::components::button_state::pressed ) )
                                {
                                    motor::log::global_t::status( "button pressed: " + motor::controls::types::xbox_controller_t::to_string( button ) ) ;
                                }
                                else if( ctrl.is( button, motor::controls::components::button_state::pressing ) )
                                {
                                    pressing = true ;
                                    motor::log::global_t::status( "button pressing: " + motor::controls::types::xbox_controller_t::to_string( button ) ) ;
                                }
                                else if( ctrl.is( button, motor::controls::components::button_state::released ) )
                                {
                                    pressing = false ;
                                    motor::log::global_t::status( "button released: " + motor::controls::types::xbox_controller_t::to_string( button ) ) ;
                                }
                            } ;

                            using part_t = motor::controls::types::xbox_controller_t::button ;

                            button_funk( part_t::start ) ;
                            button_funk( part_t::back ) ;
                            button_funk( part_t::a ) ;
                            button_funk( part_t::b ) ;
                            button_funk( part_t::x ) ;
                            button_funk( part_t::y ) ;
                        }

                        // dpad
                        {
                            auto button_funk = [&] ( motor::controls::types::xbox_controller_t::dpad const button )
                            {
                                static bool_t pressing = false ;

                                if( ctrl.is( button, motor::controls::components::button_state::pressed ) )
                                {
                                    motor::log::global_t::status( "dpad pressed: " + motor::controls::types::xbox_controller_t::to_string( button ) ) ;
                                }
                                else if( ctrl.is( button, motor::controls::components::button_state::pressing ) )
                                {
                                    pressing = true ;
                                    motor::log::global_t::status( "dpad pressing: " + motor::controls::types::xbox_controller_t::to_string( button ) ) ;
                                }
                                else if( ctrl.is( button, motor::controls::components::button_state::released ) )
                                {
                                    pressing = false ;
                                    motor::log::global_t::status( "dpad released: " + motor::controls::types::xbox_controller_t::to_string( button ) ) ;
                                }
                            } ;

                            using part_t = motor::controls::types::xbox_controller_t::dpad ;

                            button_funk( part_t::left ) ;
                            button_funk( part_t::right ) ;
                            button_funk( part_t::up ) ;
                            button_funk( part_t::down ) ;
                        }

                        // shoulder
                        {
                            auto button_funk = [&] ( motor::controls::types::xbox_controller_t::shoulder const button )
                            {
                                static bool_t pressing = false ;

                                if( ctrl.is( button, motor::controls::components::button_state::pressed ) )
                                {
                                    motor::log::global_t::status( "shoulder pressed: " + motor::controls::types::xbox_controller_t::to_string( button ) ) ;
                                }
                                else if( ctrl.is( button, motor::controls::components::button_state::pressing ) )
                                {
                                    pressing = true ;
                                    motor::log::global_t::status( "shoulder pressing: " + motor::controls::types::xbox_controller_t::to_string( button ) ) ;
                                }
                                else if( ctrl.is( button, motor::controls::components::button_state::released ) )
                                {
                                    pressing = false ;
                                    motor::log::global_t::status( "shoulder released: " + motor::controls::types::xbox_controller_t::to_string( button ) ) ;
                                }
                            } ;

                            using part_t = motor::controls::types::xbox_controller_t::shoulder ;

                            button_funk( part_t::left ) ;
                            button_funk( part_t::right ) ;
                        }

                        // thumb
                        {
                            auto button_funk = [&] ( motor::controls::types::xbox_controller_t::thumb const button )
                            {
                                static bool_t pressing = false ;

                                if( ctrl.is( button, motor::controls::components::button_state::pressed ) )
                                {
                                    motor::log::global_t::status( "thumb pressed: " + motor::controls::types::xbox_controller_t::to_string( button ) ) ;
                                }
                                else if( ctrl.is( button, motor::controls::components::button_state::pressing ) )
                                {
                                    pressing = true ;
                                    motor::log::global_t::status( "thumb pressing: " + motor::controls::types::xbox_controller_t::to_string( button ) ) ;
                                }
                                else if( ctrl.is( button, motor::controls::components::button_state::released ) )
                                {
                                    pressing = false ;
                                    motor::log::global_t::status( "thumb released: " + motor::controls::types::xbox_controller_t::to_string( button ) ) ;
                                }
                            } ;

                            using part_t = motor::controls::types::xbox_controller_t::thumb ;

                            button_funk( part_t::left ) ;
                            button_funk( part_t::right ) ;
                        }

                        // trigger
                        {
                            auto button_funk = [&] ( motor::controls::types::xbox_controller_t::trigger const button )
                            {
                                static bool_t pressing = false ;

                                float_t value = 0.0f ;

                                if( ctrl.is( button, motor::controls::components::button_state::pressed, value ) )
                                {
                                    motor::log::global_t::status( "thumb pressed: " + motor::controls::types::xbox_controller_t::to_string( button ) + 
                                        " [" + motor::to_string(value) + "]") ;
                                }
                                else if( ctrl.is( button, motor::controls::components::button_state::pressing, value ) )
                                {
                                    pressing = true ;
                                    motor::log::global_t::status( "thumb pressing: " + motor::controls::types::xbox_controller_t::to_string( button ) + 
                                        " [" + motor::to_string(value) + "]") ;
                                }
                                else if( ctrl.is( button, motor::controls::components::button_state::released, value ) )
                                {
                                    pressing = false ;
                                    motor::log::global_t::status( "thumb released: " + motor::controls::types::xbox_controller_t::to_string( button ) + 
                                        " [" + motor::to_string(value) + "]") ;
                                }
                            } ;

                            using part_t = motor::controls::types::xbox_controller_t::trigger ;

                            button_funk( part_t::left ) ;
                            button_funk( part_t::right ) ;
                        }

                        // sticks
                        {
                            auto funk = [&] ( motor::controls::types::xbox_controller_t::stick const s )
                            {
                                motor::math::vec2f_t value ;

                                if( ctrl.is( s, motor::controls::components::stick_state::tilted, value ) )
                                {
                                    motor::log::global_t::status( "stick tilted: " + motor::controls::types::xbox_controller_t::to_string( s ) + 
                                        " [" + motor::to_string( value.x() ) + "," + motor::to_string( value.y() ) + "]" ) ;
                                }
                                else if( ctrl.is( s, motor::controls::components::stick_state::tilting, value ) )
                                {
                                    motor::log::global_t::status( "stick tilting: " + motor::controls::types::xbox_controller_t::to_string( s ) + 
                                        " [" + motor::to_string( value.x() ) + "," + motor::to_string( value.y() ) + "]" ) ;
                                }
                                else if( ctrl.is( s, motor::controls::components::stick_state::untilted, value ) )
                                {
                                    motor::log::global_t::status( "stick untiled: " + motor::controls::types::xbox_controller_t::to_string( s ) + 
                                        " [" + motor::to_string( value.x() ) + "," + motor::to_string( value.y() ) + "]" ) ;
                                }
                            } ;

                            using part_t = motor::controls::types::xbox_controller_t::stick ;

                            funk( part_t::left ) ;
                            funk( part_t::right ) ;
                        }

                        // motor
                        {
                            float_t value = 0.0f ;
                            {
                                if( ctrl.is( motor::controls::types::xbox_controller_t::trigger::left, motor::controls::components::button_state::pressing, value ) )
                                {
                                    ctrl.set_motor( motor::controls::types::xbox_controller_t::vibrator::left, value ) ;
                                }
                                else if( ctrl.is( motor::controls::types::xbox_controller_t::trigger::left, motor::controls::components::button_state::released, value ) )
                                {
                                    ctrl.set_motor( motor::controls::types::xbox_controller_t::vibrator::left, value ) ;
                                }
                            }

                            {
                                if( ctrl.is( motor::controls::types::xbox_controller_t::trigger::right, motor::controls::components::button_state::pressing, value ) )
                                {
                                    ctrl.set_motor( motor::controls::types::xbox_controller_t::vibrator::right, value ) ;
                                }
                                else if( ctrl.is( motor::controls::types::xbox_controller_t::trigger::right, motor::controls::components::button_state::released, value ) )
                                {
                                    ctrl.set_motor( motor::controls::types::xbox_controller_t::vibrator::right, value ) ;
                                }
                            }
                        }
                    }
                }
            }

            motor::memory::release_ptr( wnd ) ;
        }

        motor::memory::release_ptr( gp_dev ) ;
        
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