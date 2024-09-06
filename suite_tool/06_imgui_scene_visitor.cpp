

#include <motor/profiling/probe_guard.hpp>

#include <motor/platform/global.h>

#include <motor/controls/types/ascii_keyboard.hpp>
#include <motor/controls/types/three_mouse.hpp>

#include <motor/scene/node/group/logic_group.h>
#include <motor/scene/node/leaf/logic_leaf.h>
#include <motor/scene/component/name_component.hpp>

#include <motor/tool/imgui/imgui_node_visitor.h>

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

        motor::scene::node_mtr_t _root ;

        //******************************************************************************************************
        virtual void_t on_init( void_t ) noexcept
        {
            MOTOR_PROBE( "application", "on_init" ) ;

            // #1 : init window
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

            // #2 : init scene tree
            {
                motor::scene::logic_group_t root ;
                root.add_component( motor::shared( motor::scene::name_component_t( "my root name" ) ) ) ;

                {
                    auto g = motor::shared( motor::scene::logic_group() ) ;

                    // add more children
                    {
                        g->add_child( motor::shared( motor::scene::logic_leaf_t() ) ) ;
                        g->add_child( motor::shared( motor::scene::logic_leaf_t() ) ) ;
                        g->add_child( motor::shared( motor::scene::logic_leaf_t() ) ) ;
                        {
                            auto leaf = motor::shared( motor::scene::logic_leaf_t() ) ;
                            leaf->add_component( motor::shared( motor::scene::name_component_t( "my leaf name" ) ) ) ;
                            g->add_child( motor::move( leaf ) ) ;
                        }
                    }

                    root.add_child( motor::move( g ) ) ;
                }

                _root = motor::shared( std::move( root ) ) ;
            }
        }

        //******************************************************************************************************
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

        //******************************************************************************************************
        virtual bool_t on_tool( this_t::window_id_t const wid, motor::application::app::tool_data_ref_t ) noexcept 
        { 
            MOTOR_PROBE( "application", "on_tool" ) ;

            {
                if( ImGui::Begin("test window") )
                {
                    motor::tool::imgui_node_visitor_t v ;
                    motor::scene::node_t::traverser( _root ).apply( &v ) ;
                }
                ImGui::End() ;
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