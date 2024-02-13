#include <motor/platform/global.h>
#include <motor/application/window/window_message_listener.h>
#include <motor/tool/imgui/custom_widgets.h>
#include <motor/tool/imgui/timeline.h>
#include <motor/tool/imgui/player_controller.h>

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
        carrier->device_system()->search( [&] ( motor::device::idevice_borrow_t::mtr_t dev_in )
        {
            if( auto * ptr1 = dynamic_cast<motor::device::three_device_mtr_t>(dev_in); ptr1 != nullptr )
            {
                mouse_dev = motor::share( ptr1 ) ;
            }
            else if( auto * ptr2 = dynamic_cast<motor::device::ascii_device_mtr_t>(dev_in); ptr2 != nullptr )
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
            wi.gen = motor::application::graphics_generation::gen4_auto;

            auto wnd = carrier->create_window( wi ) ;
            wnd->register_out( motor::share( msgl_out ) ) ;
            wnd->send_message( motor::application::show_message( { true } ) ) ;
            //wnd->send_message( motor::application::vsync_message_t( { true } ) ) ;

            {
                motor::tool::imgui_t imgui ;
                motor::tool::timeline_t tl("my timeline") ;
                motor::tool::timeline_t tl2("my timeline2") ;
                motor::tool::player_controller_t pc ;
                size_t cur_time = 0 ;

                bool_t created = false ;

                while( true ) 
                {
                    std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) ) ;

                    motor::application::window_message_listener_t::state_vector_t sv ;
                    if( msgl_out->swap_and_reset( sv ) )
                    {
                        if( sv.create_changed )
                        {
                            created = true ;
                        }

                        if( sv.close_changed )
                        {
                            break ;
                        }

                        if( sv.resize_changed )
                        {
                            imgui.update( {(int_t)sv.resize_msg.w,(int_t)sv.resize_msg.h} ) ;
                        }
                    }

                    // wait for window creation
                    // without creation, there is no rendering.
                    if( !created ) continue ;
                    
                    // must be done along with the user thread due
                    // to synchronization issues with any device 
                    carrier->update_device_system() ;
                    
                    wnd->render_frame< motor::graphics::gen4::frontend_t >( [&]( motor::graphics::gen4::frontend_ptr_t fe )
                    {
                        {
                            imgui.update( ascii_dev ) ;
                            imgui.update( mouse_dev ) ;
                        }

                        imgui.execute( [&]( void_t )
                        {
                            motor::tool::custom_imgui_widgets::text_overlay( "overlay", "Test Overlay" ) ;

                            {
                                if( ImGui::Begin("motor custom imgui elements") )
                                {
                                    {
                                        static motor::math::vec2f_t dir(1.0f, 0.0f) ;
                                        motor::tool::custom_imgui_widgets::direction( "direction", dir ) ;
                                    }

                                    {
                                        static float_t v = 0.0f ;
                                        motor::tool::custom_imgui_widgets::knob("my_knob", &v, 0.0f, 10.0f ) ;
                                    }
                                }
                                ImGui::End() ;
                            }

                            {
                                if( ImGui::Begin("motor custom widgets") )
                                {
                                    motor::tool::time_info_t ti {1000000, cur_time};

                                    // the timeline stores some state, so it 
                                    // is defined further above
                                    {
                                        tl.begin( ti ) ;
                                        tl.end() ;
                                        cur_time = ti.cur_milli ;
                                    }

                                    // the timeline stores some state, so it 
                                    // is defined further above
                                    {
                                        tl2.begin( ti ) ;
                                        tl2.end() ;
                                        cur_time = ti.cur_milli ;
                                    }

                                    // the player_controller stores some state, 
                                    // so it is defined further above
                                    {
                                        auto const s = pc.do_tool( "Player" ) ;
                                        if( s == motor::tool::player_controller_t::player_state::stop )
                                        {
                                            cur_time = 0  ;
                                        }
                                        else if( s == motor::tool::player_controller_t::player_state::play )
                                        {
                                            if( ti.cur_milli >= ti.max_milli ) 
                                            {
                                                cur_time = 0 ;
                                                
                                            }
                                        }

                                        if( pc.get_state() == motor::tool::player_controller_t::player_state::play )
                                        {
                                            cur_time += 100 ;
                                        }

                                        if( cur_time > ti.max_milli )
                                        {
                                            cur_time = ti.max_milli ;
                                            pc.set_stop() ;
                                        }
                                    }
                                }
                                ImGui::End() ;
                            }

                            //bool_t show = true ;
                            //ImGui::ShowDemoWindow( &show ) ;
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