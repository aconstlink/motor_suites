#include <motor/platform/global.h>
#include <motor/application/window/window_message_listener.h>

#include <motor/log/global.h>
#include <motor/memory/global.h>

#include <motor/device/layouts/ascii_keyboard.hpp>
#include <motor/device/layouts/three_mouse.hpp>

#include <motor/tool/imgui/imgui.h>

#include <future>

int main( int argc, char ** argv )
{
    using namespace motor::core::types ;

    motor::application::carrier_mtr_t carrier = motor::platform::global_t::create_carrier() ;

    auto fut_update_loop = std::async( std::launch::async, [&]( void_t )
    {
        motor::application::window_message_listener_mtr_t msgl_out = motor::memory::create_ptr<
            motor::application::window_message_listener>( "[out] : message listener" ) ;

        motor::device::ascii_device_mtr_t ascii_dev = nullptr ;
        motor::device::three_device_mtr_t mouse_dev = nullptr ;

        // looking for device. It is managed, so pointer must be copied.
        carrier->device_system()->search( [&] ( motor::device::idevice_mtr_shared_t dev_in )
        {
            if( auto * ptr1 = dynamic_cast<motor::device::three_device_mtr_t>(dev_in.mtr()); ptr1 != nullptr )
            {
                mouse_dev = motor::memory::copy_ptr( ptr1 ) ;
            }
            else if( auto * ptr2 = dynamic_cast<motor::device::ascii_device_mtr_t>(dev_in.mtr()); ptr2 != nullptr )
            {
                ascii_dev = motor::memory::copy_ptr( ptr2 ) ;
            }
        } ) ;

        {
            motor::application::window_info_t wi ;
            wi.x = 100 ;
            wi.y = 100 ;
            wi.w = 800 ;
            wi.h = 600 ;
            wi.gen = motor::application::graphics_generation::gen4_d3d11;

            auto wnd = carrier->create_window( wi ) ;
            wnd->register_out( motor::share( msgl_out ) ) ;
            wnd->send_message( motor::application::show_message( { true } ) ) ;
            //wnd->send_message( motor::application::vsync_message_t( { true } ) ) ;

            // wait for window creation
            {
                while( true ) 
                {
                    motor::application::window_message_listener_t::state_vector_t sv ;
                    if( msgl_out->swap_and_reset( sv ) )
                    {
                        if( sv.create_changed )
                        {
                            break ;
                        }
                    }
                }
            }

            {
                motor::tool::imgui_t imgui ;

                int_t w = 1000 ;
                int_t h = 1000 ;

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

                        if( sv.resize_changed )
                        {
                            w = (int_t)sv.resize_msg.w ;
                            h = (int_t)sv.resize_msg.h ;
                            imgui.update( {w,h} ) ;
                        }
                    }
                    
                    // must be done along with the user thread due
                    // to synchronization issues with any device 
                    carrier->update_device_system() ;

                    {
                        imgui.update( motor::share( ascii_dev ) ) ;
                        imgui.update( motor::share( mouse_dev ) ) ;
                    }
                    
                    wnd->render_frame< motor::graphics::gen4::frontend_t >( [&]( motor::graphics::gen4::frontend_ptr_t fe )
                    {
                        imgui.execute( [&]( void_t )
                        {
                            if( ImGui::Begin("test window") )
                            {}
                            ImGui::End() ;

                            bool_t show = true ;
                            ImGui::ShowDemoWindow( &show ) ;
                        } ) ;
                        imgui.render( fe ) ;
                    } ) ;
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