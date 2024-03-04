
#include <motor/platform/global.h>

#include <motor/gfx/primitive/primitive_render_3d.h>
#include <motor/gfx/camera/pinhole_camera.h>

#include <motor/math/utility/fn.hpp>
#include <motor/math/utility/angle.hpp>
#include <motor/math/animation/keyframe_sequence.hpp>

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
        motor::gfx::pinhole_camera_t camera ;

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
                camera.perspective_fov( motor::math::angle<float_t>::degree_to_radian( 45.0f ) ) ;
                camera.look_at( motor::math::vec3f_t( 0.0f, 0.0f, -500.0f ),
                            motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ), motor::math::vec3f_t( 0.0f, 0.0f, 0.0f )) ;
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
        }

        virtual void_t on_graphics( motor::application::app::graphics_data_in_t gd ) noexcept 
        {
            // change camera position
            {
                static float_t t = 0.0f ;
                t += gd.sec_dt *0.25f ;
                if( t > 1.0f ) t = 0.0f ;

                
                {
                    float_t const t2 = motor::math::fn<float_t>::abs( t * 2.0f - 1.0f ) ;

                    motor::math::vec3f_t const pos = motor::math::interpolation<motor::math::vec3f_t>::linear(
                        motor::math::vec3f_t(-1000.0f,0.0f,-600.0f ), motor::math::vec3f_t(1000.0,0.0f,-800.0f ), t2 ) ;

                    camera.look_at( pos, motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ), motor::math::vec3f_t( 50.0f, 0.0f, 0.0f )) ;
                }
            }

            static size_t time = 0 ;
            time += gd.milli_dt ;

            // 
            {
                typedef motor::math::cubic_hermit_spline< motor::math::vec3f_t > spline_t ;
                typedef motor::math::keyframe_sequence< spline_t > keyframe_sequence_t ;

                keyframe_sequence_t kf ;

                kf.insert( keyframe_sequence_t::keyframe_t( 0, motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) ) ) ;
                kf.insert( keyframe_sequence_t::keyframe_t( 1000, motor::math::vec3f_t( 1.0f, 0.0f, 0.0f ) ) ) ;
                kf.insert( keyframe_sequence_t::keyframe_t( 4000, motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) ) ) ;
                kf.insert( keyframe_sequence_t::keyframe_t( 2000, motor::math::vec3f_t( 0.0f, 0.0f, 1.0f ) ) ) ;
                kf.insert( keyframe_sequence_t::keyframe_t( 3000, motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ) ) ) ;

                size_t ltime = time % kf.back().get_time() ;

                motor::math::vec4f_t color = motor::math::vec4f_t( kf(ltime), 1.0f ) ;

                {
                    motor::math::vec3f_t const start( -1000.0f, 0.0f, 0.0f ) ;
                    size_t const ne = 100 ;
                    float_t const step = 1.0f / ne ;
                    for( size_t i=0; i<ne; ++i )
                    {
                        float_t const i_f = float_t(i) / ne ;
                        motor::math::vec3f_t pos = start + motor::math::vec3f_t(float_t(i), 1.0f, 1.0f) * motor::math::vec3f_t(50.0f, 1.0f, 1.0f ) ;
                        pr.draw_circle( motor::math::mat3f_t::make_identity(), pos, 30.0f, color, color, 20 ) ;
                    }
                }

                {
                    motor::math::vec3f_t const start( -300.0f, -100.0f, -100.0f ) ;
                    size_t const ne = 100 ;
                    float_t const step = 1.0f / ne ;
                    for( size_t i=0; i<ne; ++i )
                    {
                        float_t const i_f = float_t(i)  * step ; ;
                        float_t const sin_y = motor::math::fn<float_t>::sin( i_f*2.0f*motor::math::constants<float_t>::pi() ) ;
                        motor::math::vec3f_t const off( 0.0f, 200.0f * sin_y, 0.0f ) ;
                        motor::math::vec3f_t pos = start + off + motor::math::vec3f_t(float_t(i), 1.0f, 1.0f) * motor::math::vec3f_t(10.0f, 1.0f, 1.0f ) ;
                        pr.draw_circle( motor::math::mat3f_t::make_identity(), pos, 10.0f, 
                            motor::math::vec4f_t( (motor::math::vec4f_t(1.0f) - color).xyz(), 1.0f ), color, 20 ) ;
                    }
                }
            }

            // 
            {
                motor::math::vec3f_t const df( 100.0f ) ;

                typedef motor::math::cubic_hermit_spline< motor::math::vec3f_t > spline_t ;
                typedef motor::math::keyframe_sequence< spline_t > keyframe_sequence_t ;

                typedef motor::math::linear_bezier_spline< float_t > splinef_t ;
                typedef motor::math::keyframe_sequence< splinef_t > keyframe_sequencef_t ;

                keyframe_sequence_t kf ;

                motor::math::vec3f_t const p0 = motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) + df * motor::math::vec3f_t( -1.0f,-1.0f, 1.0f) ;
                motor::math::vec3f_t const p1 = motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) + df * motor::math::vec3f_t( -1.0f,1.0f, -1.0f) ;
                motor::math::vec3f_t const p2 = motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) + df * motor::math::vec3f_t( 1.0f,1.0f, 1.0f) ;
                motor::math::vec3f_t const p3 = motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) + df * motor::math::vec3f_t( 1.0f,-1.0f, -1.0f) ;

                kf.insert( keyframe_sequence_t::keyframe_t( 0, p0 ) ) ;
                kf.insert( keyframe_sequence_t::keyframe_t( 1000, p1 ) ) ;
                kf.insert( keyframe_sequence_t::keyframe_t( 2000, p2 ) ) ;
                kf.insert( keyframe_sequence_t::keyframe_t( 3000, p3 ) ) ;
                kf.insert( keyframe_sequence_t::keyframe_t( 4000, p0 ) ) ;


                keyframe_sequencef_t kf2 ;
                kf2.insert( keyframe_sequencef_t::keyframe_t( 0, 30.0f ) ) ;
                kf2.insert( keyframe_sequencef_t::keyframe_t( 1000, 50.0f ) ) ;
                kf2.insert( keyframe_sequencef_t::keyframe_t( 2000, 20.0f ) ) ;
                kf2.insert( keyframe_sequencef_t::keyframe_t( 3000, 60.0f ) ) ;
                kf2.insert( keyframe_sequencef_t::keyframe_t( 10000, 10.0f ) ) ;
                kf2.insert( keyframe_sequencef_t::keyframe_t( 11000, 30.0f ) ) ;


                size_t const ltime = time % kf.back().get_time() ;
                size_t const ltime2 = time % kf2.back().get_time() ;

                motor::math::vec4f_t color = motor::math::vec4f_t(1.0f) ;

                pr.draw_circle( 
                    ( camera.get_transformation()  ).get_rotation_matrix(), kf( ltime ), kf2( ltime2 ), color, color, 10 ) ;
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
