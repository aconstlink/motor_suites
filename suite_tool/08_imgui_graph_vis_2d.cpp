// SPDX-FileCopyrightText: 2023-2026 Alex Link
// SPDX-License-Identifier: MIT

#include <motor/profiling/probe_guard.hpp>

#include <motor/platform/global.h>

#include <motor/controls/types/ascii_keyboard.hpp>
#include <motor/controls/types/three_mouse.hpp>

#include <motor/tool/imgui/canvas_2d.h>
#include <motor/tool/imgui/graph_viewer.h>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>

#include <future>

namespace this_file
{
using namespace motor::core::types;

class my_app : public motor::application::app
{
    motor_this_typedefs( my_app );

    motor::tool::imgui::canvas_2d_t _can;
    motor::tool::imgui::graph_viewer_t _gv;

  public:

    my_app( void_t ) noexcept {}
    my_app( this_rref_t rhv ) noexcept : _can( std::move( rhv._can ) ), _gv( std::move( rhv._gv ) )
    {
    }
    virtual ~my_app( void_t ) noexcept {}

    virtual void_t on_init( void_t ) noexcept
    {
        MOTOR_PROBE( "application", "on_init" );

        {
            motor::application::window_info_t wi;
            wi.x = 100;
            wi.y = 100;
            wi.w = 800;
            wi.h = 600;
            wi.gen = motor::application::graphics_generation::gen4_gl4;
            // wi.gen = motor::application::graphics_generation::gen4_auto ;

            this_t::send_window_message( this_t::create_window( wi ),
                [ & ]( motor::application::app::window_view & wnd )
            {
                wnd.send_message( motor::application::show_message( { true } ) );
                wnd.send_message( motor::application::cursor_message_t( { true } ) );
                wnd.send_message( motor::application::vsync_message_t( { true } ) );
            } );
        }

        _nodes = this_t::add_test_nodes_with_link( _gv, _can );
    }

    virtual void_t on_event( window_id_t const wid,
        motor::application::window_message_listener::state_vector_cref_t sv ) noexcept
    {
        MOTOR_PROBE( "application", "on_event" );

        if( sv.create_changed )
        {
            motor::log::global_t::status( "[my_app] : window created" );
        }
        if( sv.close_changed )
        {
            motor::log::global_t::status( "[my_app] : window closed" );
            this->close();
        }
    }

    virtual bool_t on_tool(
        this_t::window_id_t const wid, motor::application::app::tool_data_ref_t ) noexcept
    {
        MOTOR_PROBE( "application", "on_tool" );

        static float_t pos_x = 0.0f;
        static float_t pos_y = 0.0f;

#if 0 
        {
            bool_t show = true;
            ImGui::ShowDemoWindow( &show );
            ImPlot::ShowDemoWindow( &show );
        }
#endif

        ImGui::SetNextWindowSize( ImVec2( 640.0f, 450.0f ), ImGuiCond_FirstUseEver );
        if( ImGui::Begin( "Canvas Test" ) )
        {
            if( _can.begin( "canvas", ImVec2() ) )
            {
                if( _gv.begin( &_can ) )
                {
                    {
                        this_t::draw_nodes_and_links( _gv, _nodes );
                    }
                    _gv.end();
                }
                _can.end();
            }
        }
        ImGui::End();
        return true;
    }

#if 0
    static void_t draw_test_node( motor::tool::imgui::graph_viewer_ref_t gv,
        motor::tool::imgui::canvas_2d_ref_t can ) noexcept
    {
        motor::tool::imgui::graph_viewer_t::node_info ni;
        ni.title = "Hello World";
        ni.pos = motor::math::vec2f_t( -210.0f, -65.0f );
        ni.size = motor::math::vec2f_t( 100.0f, 100.0f );

        gv.draw_node( ni );
    }
#endif

    struct add_nodes_return
    {
        std::pair< size_t, size_t > nodes_ab;
        std::pair< size_t, size_t > nodes_cd;
    };
    this_t::add_nodes_return _nodes;

    static add_nodes_return add_test_nodes_with_link( motor::tool::imgui::graph_viewer_ref_t gv,
        motor::tool::imgui::canvas_2d_ref_t can ) noexcept
    {
        // the node ids
        std::pair< size_t, size_t > nodes_ab;
        std::pair< size_t, size_t > nodes_cd;

        // node a
        {
            motor::tool::imgui::graph_viewer_t::node_info ni;
            ni.title = "Node A";
            ni.pos = motor::math::vec2f_t( -210.0f, -265.0f );
            ni.size = motor::math::vec2f_t( 100.0f, 100.0f );

            nodes_ab.first = gv.add_node( ni );
        }

        // node b
        {
            motor::tool::imgui::graph_viewer_t::node_info ni;
            ni.title = "Node B";
            ni.pos = motor::math::vec2f_t( 10.0f, -305.0f );
            ni.size = motor::math::vec2f_t( 100.0f, 100.0f );

            nodes_ab.second = gv.add_node( ni );
        }

        // node c
        {
            motor::tool::imgui::graph_viewer_t::node_info ni;
            ni.title = "Node C";
            ni.pos = motor::math::vec2f_t( 150.0f, -155.0f );
            ni.size = motor::math::vec2f_t( 100.0f, 100.0f );

            nodes_cd.first = gv.add_node( ni );
        }

        // node c
        {
            motor::tool::imgui::graph_viewer_t::node_info ni;
            ni.title = "Node D";
            ni.pos = motor::math::vec2f_t( 140.0f, -405.0f );
            ni.size = motor::math::vec2f_t( 100.0f, 100.0f );

            nodes_cd.second = gv.add_node( ni );
        }

        return { nodes_ab, nodes_cd };
    }

    void_t draw_nodes_and_links(
        motor::tool::imgui::graph_viewer_ref_t gv, add_nodes_return const & n ) noexcept
    {
        // the link ab
        {
            motor::tool::imgui::graph_viewer_t::link_info li{
                .node_id_from = n.nodes_ab.first, .node_id_to = n.nodes_ab.second };

            gv.draw_link( li );
        }

        // link ac
        {
            motor::tool::imgui::graph_viewer_t::link_info li{
                .node_id_from = n.nodes_ab.first, .node_id_to = n.nodes_cd.first };

            gv.draw_link( li );
        }

        // link ad
        {
            motor::tool::imgui::graph_viewer_t::link_info li{
                .node_id_from = n.nodes_ab.first, .node_id_to = n.nodes_cd.second };

            gv.draw_link( li );
        }

        // link bc
        {
            motor::tool::imgui::graph_viewer_t::link_info li{
                .node_id_from = n.nodes_ab.second, .node_id_to = n.nodes_cd.first };

            gv.draw_link( li );
        }

        {
            gv.draw_node( n.nodes_ab.first );
            gv.draw_node( n.nodes_ab.second );
            gv.draw_node( n.nodes_cd.first );
            gv.draw_node( n.nodes_cd.second );
        }
    }

    virtual void_t on_shutdown( void_t ) noexcept {}
};
} // namespace this_file

int main( int argc, char ** argv )
{
    return motor::platform::global_t::create_and_exec< this_file::my_app >();
}
