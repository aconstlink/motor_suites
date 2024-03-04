

#include <motor/platform/global.h>

#include <motor/gfx/camera/pinhole_camera.h>
#include <motor/gfx/font/text_render_2d.h>
#include <motor/gfx/primitive/primitive_render_2d.h>

#include <motor/format/global.h>
#include <motor/format/future_items.hpp>
#include <motor/property/property_sheet.hpp>

#include <motor/math/utility/angle.hpp>

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

        motor::io::database db = motor::io::database( motor::io::path_t( DATAPATH ), "./working", "data" ) ;

        motor::gfx::pinhole_camera_t camera ;
        motor::gfx::primitive_render_2d_t pr ;

        bool_t _draw_grid = true ;
        bool_t _draw_debug = false ;
        bool_t _view_preload_extend = false ;

        motor::vector< motor::math::vec2f_t > _points ;

        motor::math::vec2ui_t _extend ;
        motor::math::vec2f_t _cur_mouse ;

        //***************************************************************************************************
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

            {
                motor::application::window_info_t wi ;
                wi.x = 100 ;
                wi.y = 100 ;
                wi.w = 800 ;
                wi.h = 600 ;
                wi.gen = motor::application::graphics_generation::gen4_gl4 ;

                this_t::send_window_message( this_t::create_window( wi ), [&]( motor::application::app::window_view & wnd )
                {
                    wnd.send_message( motor::application::show_message( { true } ) ) ;
                    wnd.send_message( motor::application::cursor_message_t( {true} ) ) ;
                    wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                } ) ;
            }

            // init primitive renderer
            {
                pr.init( "my_prim_render" ) ;
            }

            // init camera
            {
                camera.look_at( motor::math::vec3f_t( 0.0f, 0.0f, -1000.0f ),
                    motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ), motor::math::vec3f_t( 0.0f, 0.0f, 0.0f )) ;
            }

            {
                srand( uint_t( time(NULL) ) ) ;
                for( size_t i=0; i<100; ++i )
                {
                    float_t const x = float_t((rand() % 100)-50)/50.0f ;
                    float_t const y = float_t((rand() % 100)-50)/50.0f ;

                    _points.emplace_back( motor::math::vec2f_t( x, y ) ) ;
                }
            }
        }

        //***************************************************************************************************
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
            if( sv.resize_changed )
            {
                _extend = motor::math::vec2ui_t( uint_t(sv.resize_msg.w), uint_t( sv.resize_msg.h ) ) ;
                camera.set_dims( float_t(_extend.x()), float_t(_extend.y()), 1.0f, 1000.0f ) ;
                camera.orthographic() ;
            }
        }

        //***************************************************************************************************
        virtual void_t on_device( device_data_in_t dd ) noexcept 
        {
            motor::device::layouts::three_mouse mouse( dd.mouse ) ;
            _cur_mouse = mouse.get_local() * motor::math::vec2f_t( 2.0f ) - motor::math::vec2f_t( 1.0f ) ;
        }

        //***************************************************************************************************
        virtual void_t on_graphics( motor::application::app::graphics_data_in_t ) noexcept 
        {
            float_t const radius = 3.0f ;

            // draw points
            {
                for( size_t i=0; i<_points.size(); ++i )
                {
                    auto const p = _points[i] * motor::math::vec2f_t( _extend ) ;
                    pr.draw_circle( 3, 10, p, radius, 
                        motor::math::vec4f_t(1.0f), motor::math::vec4f_t(1.0f) ) ;
                }
            }

            // test pick points
            {
                auto const ray = camera.get_camera().create_ray_norm( _cur_mouse ) ;
                auto const plane = motor::math::vec3f_t(0.0f,0.0f,-1.0f) ;
                float_t const lambda = - ray.get_origin().dot( plane ) / ray.get_direction().dot( plane ) ;

                // point on plane
                motor::math::vec2f_t const pop = ray.point_at( lambda ).xy() ;
                //_pr->draw_circle( 4, 10, pop, radius, 
                 //           motor::math::vec4f_t(1.0f,1.0f,0.0f,1.0f), motor::math::vec4f_t(1.0f) ) ;

                for( size_t i=0; i<_points.size(); ++i )
                {
                    auto const p = _points[i] * motor::math::vec2f_t( _extend ) ;
                    auto const b = (p - pop).length() < radius*1.0f ;  
                    if( b )
                    {
                        pr.draw_circle( 4, 10, p, radius, 
                            motor::math::vec4f_t(1.0f,0.0f,0.0f,1.0f), motor::math::vec4f_t(1.0f) ) ;
                    }
                }
            }

            {
                pr.set_view_proj( camera.mat_view(), camera.mat_proj() ) ;
                pr.prepare_for_rendering() ;
            }
        } 

        //***************************************************************************************************
        virtual void_t on_render( this_t::window_id_t const wid, motor::graphics::gen4::frontend_ptr_t fe, 
            motor::application::app::render_data_in_t rd ) noexcept 
        {
            if( rd.first_frame )
            {
                pr.configure( fe ) ;
            }

            {
                pr.prepare_for_rendering( fe ) ;
                for( size_t i=0; i<100; ++i )
                {
                    pr.render( fe, i ) ;
                }
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
