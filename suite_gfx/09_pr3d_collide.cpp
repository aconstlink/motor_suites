
#include <motor/platform/global.h>

#include <motor/gfx/primitive/primitive_render_3d.h>
#include <motor/gfx/camera/generic_camera.h>

#include <motor/format/global.h>
#include <motor/format/future_items.hpp>
#include <motor/property/property_sheet.hpp>

#include <motor/collide/3d/aabb.hpp>
#include <motor/collide/3d/obb.hpp>
#include <motor/collide/3d/sphere.hpp>
#include <motor/collide/3d/transform.hpp>
#include <motor/collide/3d/hit_test.hpp>

#include <motor/math/primitive/3d/ray.hpp>

#include <motor/math/utility/angle.hpp>
#include <motor/math/utility/fn.hpp>
#include <motor/math/interpolation/interpolate.hpp>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>
#include <motor/profiling/global.h>

#include <future>

namespace this_file
{
using namespace motor::core::types;

class my_app : public motor::application::app
{
    motor_this_typedefs( my_app );

    motor::graphics::state_object_t rs;

    motor::gfx::primitive_render_3d_t pr;
    motor::gfx::generic_camera_t camera;

    enum camera_view_id : size_t
    {
        normal,
        above,
        num_camera_views
    };

    bool_t _camera_views[ num_camera_views ] = { true, false };

    enum test_id : size_t
    {
        volumes,
        volume_point,
        volume_ray,
        num_tests
    };

    bool_t _show_tests[ num_tests ] = { false, false, true };

    bool_t _ortho = false;

    float_t _base_speed = 1.0f;

    virtual void_t on_init( void_t ) noexcept
    {
        {
            motor::application::window_info_t wi;
            wi.x = 400;
            wi.y = 100;
            wi.w = 800;
            wi.h = 600;
            wi.gen = motor::application::graphics_generation::gen4_gl4;

            this_t::send_window_message( this_t::create_window( wi ),
                [ & ]( motor::application::app::window_view & wnd )
            {
                wnd.send_message( motor::application::show_message( { true } ) );
                wnd.send_message( motor::application::cursor_message_t( { true } ) );
                wnd.send_message( motor::application::vsync_message_t( { true } ) );
            } );
        }

        {
            motor::application::window_info_t wi;
            wi.x = 400 + 800;
            wi.y = 100;
            wi.w = 800;
            wi.h = 600;
            wi.gen = motor::application::graphics_generation::gen4_auto;

            this_t::send_window_message( this_t::create_window( wi ),
                [ & ]( motor::application::app::window_view & wnd )
            {
                wnd.send_message( motor::application::show_message( { true } ) );
                wnd.send_message( motor::application::cursor_message_t( { true } ) );
                wnd.send_message( motor::application::vsync_message_t( { true } ) );
            } );
        }

        pr.init( "my_prim_render" );

        {
            camera.set_dims( 800.0f, 600.0f, 1.0f, 1000.0f );
            camera.perspective_fov( motor::math::angle< float_t >::degree_to_radian( 45.0f ) );
        }

        {
            motor::graphics::state_object_t so =
                motor::graphics::state_object_t( "root_render_states" );

            {
                motor::graphics::render_state_sets_t rss;
                rss.depth_s.do_change = true;
                rss.depth_s.ss.do_activate = true;
                rss.depth_s.ss.do_depth_write = true;
                rss.polygon_s.do_change = true;
                rss.polygon_s.ss.do_activate = true;
                rss.polygon_s.ss.ff = motor::graphics::front_face::clock_wise;
                rss.polygon_s.ss.cm = motor::graphics::cull_mode::back;
                rss.clear_s.do_change = true;
                rss.clear_s.ss.clear_color = motor::math::vec4f_t( 0.035f, 0.04f, 0.045f, 1.0f );
                rss.clear_s.ss.do_activate = true;
                rss.clear_s.ss.do_color_clear = true;
                rss.clear_s.ss.do_depth_clear = true;
                rss.view_s.do_change = false;
                rss.view_s.ss.do_activate = false;
                rss.view_s.ss.vp = motor::math::vec4ui_t( 0, 0, 500, 500 );
                so.add_render_state_set( rss );
            }

            rs = std::move( so );
        }
    }

    virtual void_t on_event( window_id_t const wid,
        motor::application::window_message_listener::state_vector_cref_t sv ) noexcept
    {
        if( sv.create_changed )
        {
            motor::log::global_t::status( "[my_app] : window created" );
        }
        if( sv.close_changed )
        {
            motor::log::global_t::status( "[my_app] : window closed" );
            this->close();
        }
        if( sv.resize_changed )
        {
            float_t const w = float_t( sv.resize_msg.w );
            float_t const h = float_t( sv.resize_msg.h );
            camera.set_sensor_dims( w, h );
            camera.perspective_fov();
        }
    }

    virtual void_t on_graphics( motor::application::app::graphics_data_in_t gd ) noexcept
    {
        static float_t t = 0.0f;
        t += ( 1.0f * gd.sec_dt ) * _base_speed;
        if( t > 2.0f * motor::math::constants< float_t >::pi() ) t = 0.0f;

        if( _camera_views[ camera_view_id::normal ] )
        {

            camera.look_at( motor::math::vec3f_t( 80.0f, 105.0f, 170.0f ),
                motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ),
                motor::math::vec3f_t( 0.0f, 10.0f, 0.0f ) );
        }
        else if( _camera_views[ camera_view_id::above ] )
        {

            camera.look_at( motor::math::vec3f_t( 0.0f, 200.0f, 0.0f ),
                motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ),
                motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) );
        }

        if( _show_tests[ test_id::volumes ] )
        {

            // Keep the boxes apart, leaving room for the sphere in the center.
            motor::math::vec3f_t const back_left( -42.0f, 10.0f, -32.0f );
            motor::math::vec3f_t const back_right( 42.0f, 10.0f, -32.0f );
            motor::math::vec3f_t const front_left( -48.0f, 10.0f, 36.0f );
            motor::math::vec3f_t const front_right( 36.0f, 10.0f, 36.0f );

            {
                motor::collide::c3d::sphere< float_t > const sphere(
                    motor::math::vec3f_t( 0.0f, 10.0f, 0.0f ), 12.0f );
                pr.draw_wire_sphere( sphere.get_center(), sphere.get_radius(),
                    motor::math::vec4f_t( 0.95f, 0.45f, 0.75f, 1.0f ), 48 );
            }

            {
                motor::collide::aabb3f_t aabb( back_left + motor::math::vec3f_t( 10.0f ),
                    back_left - motor::math::vec3f_t( 10.0f ) );
                pr.draw_wire_cube(
                    aabb.points(), motor::math::vec4f_t( 1.0f, 0.48f, 0.32f, 1.0f ) );
            }

            {
                motor::collide::aabb3f_t aabb( back_right, 10.0f );
                pr.draw_wire_cube(
                    aabb.points(), motor::math::vec4f_t( 1.0f, 0.78f, 0.28f, 1.0f ) );
            }

            {
                auto const orientation = ( motor::math::quat4f_t( t, motor::math::vec3f_t::up ) *
                                           motor::math::quat4f_t( t, motor::math::vec3f_t::right ) )
                                             .to_matrix();
                motor::collide::obb3f_t obb(
                    front_left, motor::math::vec3f_t( 10.0f, 10.0f, 10.0f ), orientation );
                pr.draw_wire_cube( obb.points(), motor::math::vec4f_t( 0.2f, 0.85f, 0.7f, 1.0f ) );
            }

            {
                motor::math::m3d::trafof_t trafo( motor::math::vec3f_t( 1.0f, 1.0f, 4.0f ),
                    motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ),
                    motor::math::vec3f_t( 20.0f, -30.0f, 0.0f ) );

                trafo.rotate_by_quaternion_fl(
                    motor::math::quat4f_t( t, motor::math::vec3f_t::up ) );

                motor::collide::aabb3f_t aabb( front_right, -10.0f );
                auto const obb =
                    motor::collide::c3d::transform< float_t >::from_aabb( aabb, trafo );

                auto const aabb2 = motor::collide::c3d::transform_bounds< float_t >::to_aabb( obb );

                pr.draw_wire_cube( obb.points(), motor::math::vec4f_t( 0.3f, 0.65f, 1.0f, 1.0f ) );
                pr.draw_wire_cube(
                    aabb2.points(), motor::math::vec4f_t( 0.9f, 0.93f, 0.97f, 1.0f ) );
            }
        }
        else if( _show_tests[ test_id::volume_point ] )
        {
            auto const p = motor::math::quat4f_t( t, motor::math::vec3f_t::up ) *
                           motor::math::vec3f_t( 0.0f, 1.0f * std::sin( t * 5.0f ), 50.0f );

            {
                pr.draw_circle( camera.get_camera_matrix(), p, 2.0f,
                    motor::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ), motor::math::vec4f_t(), 10 );
            }

            // aabb/point
            {
                motor::collide::aabb3f_t const aabb(
                    motor::math::vec3f_t( 50.0f, 0.0f, 0.0f ), 20.0 );

                motor::math::vec4f_t color( 0.0f, 1.0f, 0.0, 1.0f );
                auto const ht =
                    motor::collide::c3d::hit_test< float_t >::hit_test_box_point( aabb, p );
                if( ht != motor::collide::hit_test_type::outside )
                    color = motor::math::vec4f_t( 1.0f, 0.0f, 0.0, 1.0f );

                pr.draw_wire_cube( aabb.points(), color );
            }

            // sphere/point
            {
                motor::collide::sphere3f_t s( motor::math::vec3f_t( 0.0f, 0.0f, -50.0f ), 20.0f );
                motor::math::vec4f_t color( 0.0f, 1.0f, 0.0, 1.0f );

                auto ht = motor::collide::c3d::hit_test< float_t >::hit_test_sphere_point( s, p );
                if( ht != motor::collide::hit_test_type::outside )
                    color = motor::math::vec4f_t( 1.0f, 0.0f, 0.0, 1.0f );

                pr.draw_wire_sphere( s.get_center(), s.get_radius(), color, 50 );
            }
        }
        else if( _show_tests[ test_id::volume_ray ] )
        {
            auto const dir = motor::math::quat4f_t( t, motor::math::vec3f_t::up ) *
                             motor::math::vec3f_t( 0.0f, 0.05f * std::sin( t * 6.0f ), 1.0f );

            motor::math::m3d::ray< float_t > r( motor::math::vec3f_t(), dir );

            {
                pr.draw_line( r.get_origin(), r.point_at( 100.0f ),
                    motor::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ) );
            }

            // aabb/ray
            {
                motor::collide::aabb3f_t const aabb(
                    motor::math::vec3f_t( 50.0f, 0.0f, 0.0f ), 20.0 );

                motor::math::vec4f_t color( 0.0f, 1.0f, 0.0, 1.0f );

                float_t hit = 0.0f;
                auto const ht =
                    motor::collide::c3d::hit_test< float_t >::hit_test_box_ray( aabb, r, hit );
                if( ht != motor::collide::hit_test_type::outside )
                    color = motor::math::vec4f_t( 1.0f, 0.0f, 0.0, 1.0f );

                pr.draw_wire_cube( aabb.points(), color );

                pr.draw_circle( camera.get_camera_matrix(), r.point_at( hit ), 1.0f,
                    motor::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ), motor::math::vec4f_t(), 10 );
            }

            // sphere/ray
            {
                motor::collide::sphere3f_t s( motor::math::vec3f_t( 0.0f, 0.0f, -50.0f ), 20.0f );
                motor::math::vec4f_t color( 0.0f, 1.0f, 0.0, 1.0f );
                float_t hit = 0.0f;
                auto ht =
                    motor::collide::c3d::hit_test< float_t >::hit_test_sphere_ray( s, r, hit );
                if( ht != motor::collide::hit_test_type::outside )
                    color = motor::math::vec4f_t( 1.0f, 0.0f, 0.0, 1.0f );

                pr.draw_wire_sphere( s.get_center(), s.get_radius(), color, 50 );
                pr.draw_circle( camera.get_camera_matrix(), r.point_at( hit ), 1.0f,
                    motor::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ), motor::math::vec4f_t(), 10 );
            }
        }

        pr.set_view_proj( camera.mat_view(), camera.mat_proj() );

        pr.prepare_for_rendering();
    }

    virtual void_t on_first_frame( this_t::window_id_t const wid,
        motor::graphics::gen4::frontend_ptr_t fe,
        motor::application::app::render_data_in_t ) noexcept
    {
        fe->configure< motor::graphics::state_object_t >( &rs );
        pr.configure( fe );
    }

    virtual void_t on_render( this_t::window_id_t const wid,
        motor::graphics::gen4::frontend_ptr_t fe,
        motor::application::app::render_data_in_t ) noexcept
    {
        // render text layer 0 to screen
        {
            pr.prepare_for_rendering( fe );
            fe->push( &rs );
            pr.render( fe );
            fe->pop( motor::graphics::gen4::backend::pop_type::render_state );
        }
    }

    virtual bool_t on_tool(
        this_t::window_id_t const wid, motor::application::app::tool_data_ref_t ) noexcept
    {
        if( wid != 0 ) return false;

        if( ImGui::Begin( "Properties" ) )
        {
            ImGui::SliderFloat( "Base Speed", &_base_speed, 0.0f, 2.0f );

            ImGui::Separator();

            if( ImGui::Checkbox( "Orthographic", &_ortho ) )
            {
                if( _ortho )
                    camera.orthographic( 0.5f );
                else
                    camera.perspective_fov(
                        motor::math::angle< float_t >::degree_to_radian( 45.0f ) );
            }
            ImGui::Separator();

            // Choose one test and keep its array entry active.
            {
                char const * const test_names[ num_tests ] = {
                    "Volumes", "Point/Volume Intersection", "Ray/Volume Intersection" };

                int_t selected_test = 0;
                for( size_t i = 0; i < num_tests; ++i )
                {
                    if( _show_tests[ i ] )
                    {
                        selected_test = int_t( i );
                        break;
                    }
                }

                if( ImGui::Combo( "Test", &selected_test, test_names, int_t( num_tests ) ) )
                {
                    for( size_t i = 0; i < num_tests; ++i )
                        _show_tests[ i ] = i == size_t( selected_test );
                }
            }

            ImGui::Separator();

            {
                char const * const camera_names[ num_camera_views ] = { "Normal", "Above" };

                int_t selected_camera = 0;
                for( size_t i = 0; i < num_camera_views; ++i )
                {
                    if( _camera_views[ i ] )
                    {
                        selected_camera = int_t( i );
                        break;
                    }
                }

                if( ImGui::Combo(
                        "Camera", &selected_camera, camera_names, int_t( num_camera_views ) ) )
                {
                    for( size_t i = 0; i < num_camera_views; ++i )
                        _camera_views[ i ] = i == size_t( selected_camera );
                }
            }
        }
        ImGui::End();

        return true;
    }
};
} // namespace this_file

int main( int argc, char ** argv )
{
    using namespace motor::core::types;

    motor::application::carrier_mtr_t carrier =
        motor::platform::global_t::create_carrier( motor::shared( this_file::my_app() ) );

    auto const ret = carrier->exec();

    motor::memory::release_ptr( carrier );

    motor::io::global::deinit();
    motor::concurrent::global::deinit();
    motor::log::global::deinit();
    motor::profiling::global::deinit();
    motor::memory::global::dump_to_std();

    return ret;
}
