


#include <motor/platform/global.h>

#include <motor/device/layouts/ascii_keyboard.hpp>
#include <motor/device/layouts/three_mouse.hpp>

#include <motor/tool/imgui/custom_widgets.h>
#include <motor/tool/imgui/timeline.h>
#include <motor/tool/imgui/player_controller.h>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>

#include <future>

namespace this_file
{
    using namespace motor::core::types ;

    class my_app : public motor::application::app
    {
        motor_this_typedefs( my_app ) ;

        motor::tool::timeline_t tl = motor::tool::timeline_t("my timeline") ;
        motor::tool::timeline_t tl2 = motor::tool::timeline_t("my timeline2") ;
        motor::tool::player_controller_t pc ;

        size_t cur_time = 0 ;

        virtual void_t on_init( void_t ) noexcept
        {
            {
                motor::application::window_info_t wi ;
                wi.x = 100 ;
                wi.y = 100 ;
                wi.w = 800 ;
                wi.h = 600 ;
                wi.gen = motor::application::graphics_generation::gen4_auto ;
                
                this_t::send_window_message( this_t::create_window( wi ), [&]( motor::application::app::window_view & wnd )
                {
                    wnd.send_message( motor::application::show_message( { true } ) ) ;
                    wnd.send_message( motor::application::cursor_message_t( {true} ) ) ;
                    wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                } ) ;
            }
        }

        virtual void_t on_event( window_id_t const wid, 
                motor::application::window_message_listener::state_vector_cref_t sv ) noexcept
        {
            if( sv.create_changed )
            {
                motor::log::global_t::status("[my_app] : window created") ;
            }
            if( sv.close_changed )
            {
                motor::log::global_t::status("[my_app] : window closed") ;
                this->close() ;
            }
        }

        virtual bool_t on_tool( this_t::window_id_t const wid, motor::application::app::tool_data_ref_t ) noexcept 
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
            return true ; 
        }
    };
}

int main( int argc, char ** argv )
{
    using namespace motor::core::types ;

    motor::application::carrier_mtr_t carrier = motor::platform::global_t::create_carrier(
        motor::shared( this_file::my_app() ) ) ;
    
    auto const ret = carrier->exec() ;
    
    motor::memory::release_ptr( carrier ) ;

    motor::concurrent::global::deinit() ;
    motor::log::global::deinit() ;
    motor::memory::global::dump_to_std() ;


    return ret ;
}
