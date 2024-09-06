
#include <motor/platform/global.h>

#include <motor/gfx/primitive/primitive_render_3d.h>
#include <motor/gfx/camera/generic_camera.h>

#include <motor/math/utility/angle.hpp>
#include <motor/math/utility/fn.hpp>
#include <motor/math/interpolation/interpolate.hpp>
#include <motor/math/spline/quadratic_bezier_spline.hpp>
#include <motor/math/spline/linear_bezier_spline.hpp>

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

        motor::graphics::state_object_t rs ;

        motor::gfx::primitive_render_3d_t pr ;
        motor::gfx::generic_camera_t camera ;

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
                    wnd.send_message( motor::application::cursor_message_t( {false} ) ) ;
                    wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                } ) ;
            }

            {
                motor::application::window_info_t wi ;
                wi.x = 400 ;
                wi.y = 100 ;
                wi.w = 800 ;
                wi.h = 600 ;
                wi.gen = motor::application::graphics_generation::gen4_gl4 ;

                this_t::send_window_message( this_t::create_window( wi ), [&]( motor::application::app::window_view & wnd )
                {
                    wnd.send_message( motor::application::show_message( { true } ) ) ;
                    wnd.send_message( motor::application::cursor_message_t( {false} ) ) ;
                    wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                } ) ;
            }

            pr.init( "my_prim_render" ) ;


            {
                camera.set_dims( 1.0f, 1.0f, 1.0f, 1000.0f ) ;
                camera.perspective_fov( motor::math::angle<float_t>::degree_to_radian( 45.0f ) ) ;
                camera.look_at( motor::math::vec3f_t( -200.0f, 0.0f, -50.0f ),
                            motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ), motor::math::vec3f_t( 0.0f, 500.0f, 0.0f )) ;
            }

            {
                motor::graphics::state_object_t so = motor::graphics::state_object_t(
                    "root_render_states" ) ;

                {
                    motor::graphics::render_state_sets_t rss ;
                    rss.depth_s.do_change = true ;
                    rss.depth_s.ss.do_activate = true ;
                    rss.depth_s.ss.do_depth_write = true ;
                    rss.polygon_s.do_change = true ;
                    rss.polygon_s.ss.do_activate = true ;
                    rss.polygon_s.ss.ff = motor::graphics::front_face::clock_wise ;
                    rss.polygon_s.ss.cm = motor::graphics::cull_mode::back ;
                    rss.clear_s.do_change = true ;
                    rss.clear_s.ss.clear_color = motor::math::vec4f_t(0.5f, 0.2f, 0.2f, 1.0f ) ;
                    rss.clear_s.ss.do_activate = true ;
                    rss.clear_s.ss.do_color_clear = true ;
                    rss.clear_s.ss.do_depth_clear = true ;
                    rss.view_s.do_change = false ;
                    rss.view_s.ss.do_activate = false ;
                    rss.view_s.ss.vp = motor::math::vec4ui_t( 0, 0, 500, 500 ) ;
                    so.add_render_state_set( rss ) ;
                }

                rs = std::move( so ) ; 
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
            if ( sv.resize_changed )
            {
                float_t const w = float_t( sv.resize_msg.w ) ;
                float_t const h = float_t( sv.resize_msg.h ) ;
                camera.set_sensor_dims( w, h ) ;
                camera.perspective_fov() ;
            }
        }

        virtual void_t on_graphics( motor::application::app::graphics_data_in_t gd ) noexcept 
        {
            static float_t t = 0.0f ;
            t += gd.sec_dt *0.25f ;
            if( t > 1.0f ) t = 0.0f ;

            // change camera position
            {
                float_t const t2 = motor::math::fn<float_t>::abs( t * 2.0f - 1.0f ) ;

                motor::math::vec3f_t const pos = motor::math::interpolation<motor::math::vec3f_t>::linear(
                    motor::math::vec3f_t(-200.0f,0.0f,-600.0f ), motor::math::vec3f_t(200.0,0.0f,-600.0f ), t2 ) ;

                camera.look_at( pos, motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ), motor::math::vec3f_t( 50.0f, 0.0f, 0.0f )) ;
            }

            motor::math::vec3f_t off( 0.0f, 0.0f, 0.0f) ;

            // draw splines #1
            {
                off += motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) ;

                typedef motor::math::quadratic_bezier_spline<motor::math::vec3f_t> spline_t ;

                spline_t s( {
                    motor::math::vec3f_t(-50.0f, 0.0f, 0.0f) + off,
                    motor::math::vec3f_t(-25.0f, 50.0f, 0.0f) + off,
                    motor::math::vec3f_t(-0.0f, 50.0f, 0.0f) + off,
                    motor::math::vec3f_t(50.0f, 0.0f, -0.0f) + off,
                    motor::math::vec3f_t(100.0f, -50.0f, 0.0f) + off,
                    motor::math::vec3f_t(150.0f, 0.0f, 0.0f) + off,
                    motor::math::vec3f_t(200.0f, -100.0f, 0.0f) + off,
                    motor::math::vec3f_t(250.0f, 0.0f, 0.0f) + off
                }, spline_t::init_type::construct  ) ;
                
                
                static size_t change_idx = 1 ;

                // change control point
                {
                    
                    static float_t t2 = 0.0f ;
                    t2 += gd.sec_dt * 0.5f  ;
                    if( t2 > 1.0f ) 
                    {
                        change_idx = ++change_idx < s.ncp() ? change_idx : 0 ;
                        t2 = 0.0f ;
                    }

                    float_t f = t * motor::math::constants<float_t>::pix2() ;
                    motor::math::vec3f_t dif( 
                        motor::math::fn<float_t>::cos( f ), 
                        motor::math::fn<float_t>::sin( f ) * motor::math::fn<float_t>::cos( f ), 
                        motor::math::fn<float_t>::sin( f ) ) ;
                    
                    auto const cp = s.get_control_point( change_idx ) + dif * 30.0f ;
                    s.change_point( change_idx, cp ) ;
                }


                motor::math::linear_bezier_spline<motor::math::vec3f_t> ls = s.control_points() ;

                size_t const samples = 100 ;
                for( size_t i=0; i<(samples>>1); ++i )
                {
                    float_t const frac0 = float_t((i<<1)+0) / float_t(samples-1) ;
                    float_t const frac1 = float_t((i<<1)+1) / float_t(samples-1) ;

                    {
                        auto const p0 = s( frac0 ) ;
                        auto const p1 = s( frac1 ) ;

                        pr.draw_line( p0, p1, motor::math::vec4f_t( 1.0f, 1.0f, 0.4f, 1.0f )  ) ;
                    }

                    {
                        auto const p0 = ls( frac0 ) ;
                        auto const p1 = ls( frac1 ) ;

                        pr.draw_line( p0, p1, motor::math::vec4f_t( 1.0f, 0.5f, 0.4f, 1.0f )  ) ;
                    }
                }

                {
                    s.for_each_control_point( [&]( size_t const i, motor::math::vec3f_cref_t p )
                    {
                        auto const r = motor::math::m3d::trafof_t::rotation_by_axis( motor::math::vec3f_t(1.0f,0.0f,0.0f), 
                                motor::math::angle<float_t>::degree_to_radian(90.0f) ) ;

                        motor::math::vec4f_t color( 1.0f ) ;
                        if( i == change_idx ) color = motor::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ) ;

                        pr.draw_circle( 
                            ( camera.get_transformation() ).get_rotation_matrix(), p, 
                            2.0f, color, color, 20 ) ;
                    } ) ;
                }
            }

            pr.set_view_proj( camera.mat_view(), camera.mat_proj() ) ;

            pr.prepare_for_rendering() ;
        } 

        virtual void_t on_render( this_t::window_id_t const wid, motor::graphics::gen4::frontend_ptr_t fe, 
            motor::application::app::render_data_in_t rd ) noexcept 
        {
            if( rd.first_frame )
            {
                fe->configure< motor::graphics::state_object_t>( &rs ) ;
                pr.configure( fe ) ;
            }

            // render text layer 0 to screen
            {
                pr.prepare_for_rendering( fe ) ;
                fe->push( &rs ) ;
                pr.render( fe ) ;
                fe->pop( motor::graphics::gen4::backend::pop_type::render_state ) ;
            }
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
