

#include <motor/profiling/probe_guard.hpp>

#include <motor/platform/global.h>

#include <motor/wire/node/node.h>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>

#include <future>

namespace this_file
{
    using namespace motor::core::types ;

    // simply testing basic functionality
    class my_app : public motor::application::app
    {
        motor_this_typedefs( my_app ) ;

        virtual void_t on_init( void_t ) noexcept
        {
            MOTOR_PROBE( "application", "on_init" ) ;

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
            MOTOR_PROBE( "application", "on_event" ) ;

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
            MOTOR_PROBE( "application", "on_tool" ) ;

            // #1 : task graph window
            if( ImGui::Begin( "Testing ImNodes" ) )
            {
                ImNodes::BeginNodeEditor();

                ImVec2 cur_pos(0.0f,0.0f) ;//= ImGui::GetCursorPos() ; 
                
                ImNodes::BeginNode( 0 );

                {
                    ImNodes::BeginNodeTitleBar();
                    ImGui::TextUnformatted( "node" );
                    ImNodes::EndNodeTitleBar();
                }

                {
                    ImNodes::BeginInputAttribute( 1 );
                    ImGui::Text( "input" );
                    ImNodes::EndInputAttribute();


                    ImNodes::BeginOutputAttribute( 2 );
                    ImGui::Indent( 40 );
                    ImGui::Text( "output" );
                    ImNodes::EndOutputAttribute();

                    ImNodes::BeginStaticAttribute(3) ;
                    ImGui::Text( "static" );
                    ImNodes::EndStaticAttribute() ;
                }

                ImNodes::EndNode();
                ImNodes::SetNodeGridSpacePos( 0, ImVec2(0.0f,0.0f) ) ;

                
                //ImNodes::MiniMap();
                ImNodes::EndNodeEditor();


                ImGui::End();
            }

            return true ; 
        }

        virtual void_t on_shutdown( void_t ) noexcept {}
    };
}

int main( int argc, char ** argv )
{
    return motor::platform::global_t::create_and_exec< this_file::my_app >() ;
}