

#include <motor/profiling/probe_guard.hpp>

#include <motor/platform/global.h>

#include <motor/controls/types/ascii_keyboard.hpp>
#include <motor/controls/types/three_mouse.hpp>

#include <motor/tool/imgui/canvas_2d.h>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>

#include <future>

namespace this_file
{
using namespace motor::core::types;

namespace palette
{
ImColor const mint( 112, 210, 185 );
ImColor const cornflower( 143, 169, 230 );
ImColor const apricot( 239, 169, 130 );
ImColor const grid( 82, 89, 97 );
} // namespace palette

class my_app : public motor::application::app
{
    motor_this_typedefs( my_app );

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

    motor::tool::imgui::canvas_2d_t can;

    virtual bool_t on_tool(
        this_t::window_id_t const wid, motor::application::app::tool_data_ref_t ) noexcept
    {
        MOTOR_PROBE( "application", "on_tool" );

        static float_t pos_x = 0.0f;
        static float_t pos_y = 0.0f;

#if 1
        {
            if( ImGui::Begin( "test window" ) )
            {
            }
            ImGui::End();
        }

        {
            bool_t show = true;
            ImGui::ShowDemoWindow( &show );
            ImPlot::ShowDemoWindow( &show );
        }
#endif

        ImGui::SetNextWindowSize( ImVec2( 640.0f, 450.0f ), ImGuiCond_FirstUseEver );
        if( ImGui::Begin( "Canvas Test" ) )
        {
            if( can.begin( "canvas", ImVec2(), { .grid_color = palette::grid } ) )
            {
                can.draw_line( { motor::math::vec2f_t( -192.0f, 57.0f ),
                    motor::math::vec2f_t( -128.0f, 87.0f ), palette::mint } );

                can.draw_point( { motor::math::vec2f_t( -52.0f, 72.0f ), palette::apricot } );

                can.draw_circle(
                    { motor::math::vec2f_t( 52.0f, 72.0f ), 19.0f, palette::cornflower, true } );

                can.draw_circle(
                    { motor::math::vec2f_t( 160.0f, 72.0f ), 19.0f, palette::cornflower, false } );

                can.draw_poly_line( this_t::make_test_polyline() );

                can.draw_polygon( this_t::make_test_polygon() );

                this_t::draw_test_rects( can );
                this_t::draw_test_text( can );

                can.end();
            }
        }
        ImGui::End();

        return true;
    }

    static motor::tool::imgui::canvas_2d_t::poly_line_info make_test_polyline( void_t ) noexcept
    {
        using vec2_t = motor::math::vec2f_t;

        motor::tool::imgui::canvas_2d_t::poly_line_info line{};
        line.thickness = 2.0f;
        line.color = palette::mint;
        line.points = { vec2_t( -192.0f, -12.0f ), vec2_t( -168.0f, -12.0f ),
            vec2_t( -144.0f, 20.0f ), vec2_t( -120.0f, 0.0f ), vec2_t( -92.0f, 0.0f ),
            vec2_t( -64.0f, -20.0f ), vec2_t( -24.0f, -20.0f ) };
        return line;
    }

    static motor::tool::imgui::canvas_2d_t::polygon_info make_test_polygon( void_t ) noexcept
    {
        using vec2_t = motor::math::vec2f_t;

        motor::tool::imgui::canvas_2d_t::polygon_info poly{};
        poly.outline_width = 2.0f;
        poly.fill_color = ImColor( 56, 107, 99 );
        poly.outline_color = palette::mint;
        poly.points = {
            vec2_t( 48.0f, 4.0f ),
            vec2_t( 152.0f, 20.0f ),
            vec2_t( 192.0f, 0.0f ),
            vec2_t( 160.0f, -20.0f ),
            vec2_t( 76.0f, -20.0f ),
        };
        return poly;
    }

    static void_t draw_test_rects( motor::tool::imgui::canvas_2d_ref_t can ) noexcept
    {
        using vec2_t = motor::math::vec2f_t;

        {
            motor::tool::imgui::canvas_2d_t::rect_filled_info info{
                .pos = vec2_t( -192.0f, -110.0f ),
                .size = vec2_t( 96.0f, 48.0f ),
                .color = palette::mint };
            can.draw_rect_filled( info );
        }

        {
            motor::tool::imgui::canvas_2d_t::rect_filled_info info{
                .pos = vec2_t( -48.0f, -110.0f ),
                .size = vec2_t( 96.0f, 48.0f ),
                .color = palette::cornflower,
                .radius = 6.0f,
                .flags = ImDrawFlags_RoundTop | ImDrawFlags_RoundBottom };
            can.draw_rect_filled( info );
        }

        {
            motor::tool::imgui::canvas_2d_t::rect_filled_info info{ .pos = vec2_t( 96.0f, -110.0f ),
                .size = vec2_t( 96.0f, 48.0f ),
                .color = palette::apricot,
                .radius = 6.0f,
                .flags = ImDrawFlags_RoundTop | ImDrawFlags_RoundBottom };
            can.draw_rect_filled( info );
        }
    }

    static void_t draw_test_text( motor::tool::imgui::canvas_2d_ref_t can ) noexcept
    {
        using vec2_t = motor::math::vec2f_t;

        {
            motor::tool::imgui::canvas_2d_t::text_info info{ .title = "Hello World",
                .pos = vec2_t( -192.0f, -110.0f - 10.0f ),
                .color = palette::mint };
            can.draw_text( info );
        }

        {
            motor::tool::imgui::canvas_2d_t::text_info info{ .title = "Hello World",
                .pos = vec2_t( -48.0f, -110.0f - 20.0f ),
                .font_height = 20,
                .color = palette::cornflower };
            can.draw_text( info );
        }

        {
            motor::tool::imgui::canvas_2d_t::text_info info{ .title = "Hello World",
                .pos = vec2_t( 96.0f, -110.0f - 30.0f ),
                .font_height = 30,
                .color = palette::apricot };
            can.draw_text( info );
        }
    }

    virtual void_t on_shutdown( void_t ) noexcept {}
};
} // namespace this_file

int main( int argc, char ** argv )
{
    return motor::platform::global_t::create_and_exec< this_file::my_app >();
}
