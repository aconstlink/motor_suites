
#include <motor/platform/global.h>

#include <motor/gfx/primitive/primitive_render_3d.h>
#include <motor/gfx/camera/pinhole_camera.h>

#include <motor/format/global.h>
#include <motor/format/future_items.hpp>
#include <motor/property/property_sheet.hpp>

#include <motor/math/utility/angle.hpp>
#include <motor/math/utility/fn.hpp>
#include <motor/math/interpolation/interpolate.hpp>

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
                camera.look_at( motor::math::vec3f_t( -200.0f, 0.0f, -50.0f ),
                            motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ), motor::math::vec3f_t( 0.0f, 0.0f, 0.0f )) ;
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
            static float_t t = 0.0f ;
            t += gd.sec_dt *0.25f ;
            if( t > 1.0f ) t = 0.0f ;

            {
                float_t const t2 = motor::math::fn<float_t>::abs( t * 2.0f - 1.0f ) ;

                motor::math::vec3f_t const pos = motor::math::interpolation<motor::math::vec3f_t>::linear(
                    motor::math::vec3f_t(-200.0f,0.0f,-100.0f ), motor::math::vec3f_t(500.0,0.0f,-200.0f ), t2 ) ;

                camera.look_at( pos, motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ), motor::math::vec3f_t( 0.0f, 0.0f, 0.0f )) ;
            }

            // draw some 3d lines
            {
                size_t const ns = 10 ;
                for( size_t i=0; i<ns; ++i )
                {
                    float_t const fac = (float_t( i ) / float_t( ns-1 )) ;
                    float_t const part =  fac * 2.0f * motor::math::constants<float_t>::pi()  ;
                    pr.draw_line( motor::math::vec3f_t(0.0f), 
                        motor::math::vec3f_t( 100.0f * std::cos(part), 100.0f * std::sin(part), 10.0f ), 
                        motor::math::vec4f_t( 1.0f )  ) ;
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
                pr.configure( fe ) ;
            }

            // render text layer 0 to screen
            {
                pr.prepare_for_rendering( fe ) ;
                pr.render( fe ) ;
                pr.render( fe ) ;
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

    motor::io::global::deinit() ;
    motor::concurrent::global::deinit() ;
    motor::log::global::deinit() ;
    motor::memory::global::dump_to_std() ;


    return ret ;
}
