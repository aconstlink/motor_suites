


#include <motor/platform/global.h>

#include <motor/controls/types/ascii_keyboard.hpp>
#include <motor/controls/types/three_mouse.hpp>

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
            return true ; 
        }
    };
}

int main( int argc, char ** argv )
{
    return motor::platform::global_t::create_and_exec< this_file::my_app >() ;
}
