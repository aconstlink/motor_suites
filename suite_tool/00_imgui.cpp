

#include <motor/profiling/probe_guard.hpp>

#include <motor/platform/global.h>

#include <motor/controls/types/ascii_keyboard.hpp>
#include <motor/controls/types/three_mouse.hpp>

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

            static float_t pos_x = 0.0f ;
            static float_t pos_y = 0.0f ;

            #if 0
            {
                if( ImGui::Begin("test window") ){}
                ImGui::End() ;
            }
            
            {
                bool_t show = true ;
                ImGui::ShowDemoWindow( &show ) ;
                ImPlot::ShowDemoWindow( &show ) ;
            }
            #endif
            {
                ImGui::Begin( "simple node editor" );

                ImGui::SliderFloat( "move node x", &pos_x, -1000.0f, 1000.0f) ;
                ImGui::SliderFloat( "move node y", &pos_y, -1000.0f, 1000.0f) ;

                ImVec2 cur_pos = ImGui::GetCursorPos() ;

                
                
                ImNodes::BeginNodeEditor();

                {
                    auto style = ImNodes::GetStyle() ;
                    style.GridSpacing = 20.0f ;
                    //ImNodes::PushStyleVar( ImNodesStyleVar_GridSpacing, 1.0f ) ;
                    
                    //ImNodes::GetCurrentContext()-> ;
                }

                #if 1
                {
                    for( int_t i=0; i<3; ++i )
                    {
                        ImNodes::BeginNode( i );

                        ImNodes::BeginNodeTitleBar();
                        ImGui::TextUnformatted( "node" );
                        ImNodes::EndNodeTitleBar();

                        #if 0
                        ImNodes::BeginInputAttribute( 2 );
                        ImGui::Text( "input" );
                        ImNodes::EndInputAttribute();
                         
                        ImNodes::BeginOutputAttribute( 3 );
                        ImGui::Indent( 40 );
                        ImGui::Text( "output" );
                        ImNodes::EndOutputAttribute();
                        #else
                        ImGui::Dummy(ImVec2() ) ;
                        #endif
                        cur_pos = ImGui::GetCursorPos() ;
                        
                        ImNodes::EndNode();
                        ImNodes::SetNodeGridSpacePos( i, ImVec2( pos_x, pos_y+float_t(i)*1000.0f ) ) ;
                    }
                }
                #else
                {
                  

                    ImNodes::BeginNode( 1 );

                    ImNodes::BeginNodeTitleBar();
                    ImGui::TextUnformatted( "node" );
                    ImNodes::EndNodeTitleBar();

                    ImNodes::BeginInputAttribute( 2 );
                    ImGui::Text( "input" );
                    ImNodes::EndInputAttribute();

                    ImNodes::BeginOutputAttribute( 3 );
                    ImGui::Indent( 40 );
                    ImGui::Text( "output" );
                    ImNodes::EndOutputAttribute();

                    cur_pos = ImGui::GetCursorPos() ;

                    ImNodes::EndNode();
                    ImNodes::SetNodeGridSpacePos( 1, ImVec2( pos_x, pos_y ) ) ;
                }
                
                {
                    ImNodes::BeginNode( 4 );

                    ImNodes::BeginNodeTitleBar();
                    ImGui::TextUnformatted( "node" );
                    ImNodes::EndNodeTitleBar();

                    ImNodes::BeginInputAttribute( 5 );
                    ImGui::Text( "input" );
                    ImNodes::EndInputAttribute();

                    ImNodes::BeginOutputAttribute( 6 );
                    ImGui::Indent( 40 );
                    ImGui::Text( "output" );
                    ImNodes::EndOutputAttribute();

                    ImNodes::EndNode();
                    ImNodes::SetNodeGridSpacePos( 4, ImVec2( pos_x+10.0f, pos_y ) ) ;
                }
                #endif
                {
                    
                    
                    //ImNodes::SetNodeGridSpacePos( 4, ImVec2( style.GridSpacing*3.0f, style.GridSpacing *3.0f ) ) ;

                    //ImNodes::EditorContextMoveToNode( 1 ) ;
                     
                }

                {
                    //ImNodes::Link( 3, 3, 5  ) ;
                }

                ImNodes::MiniMap();
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