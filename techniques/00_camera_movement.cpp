
#include <motor/platform/global.h>

#include <motor/tool/imgui/timeline.h>
#include <motor/tool/imgui/player_controller.h>

#include <motor/controls/types/ascii_keyboard.hpp>
#include <motor/controls/types/three_mouse.hpp>

#include <motor/gfx/primitive/primitive_render_3d.h>
#include <motor/gfx/camera/generic_camera.h>

#include <motor/math/utility/fn.hpp>
#include <motor/math/utility/angle.hpp>
#include <motor/math/animation/keyframe_sequence.hpp>
#include <motor/math/quaternion/quaternion4.hpp>

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

        // 0 : free camera
        // 1 : spline camera
        motor::gfx::generic_camera_t camera[2] ;
        size_t cam_idx = 0 ;

        struct cam_control
        {
            bool_t move_left = false ;
            bool_t move_right = false ;
            bool_t move_forwards = false ;
            bool_t move_backwards = false ;
            bool_t move_upwards = false ;
            bool_t move_downwards = false ;

            motor::math::vec2f_t mouse_coords ;

            int_t rotate_x = 0 ;
            int_t rotate_y = 0 ;
            int_t rotate_z = 0 ;
        };

        cam_control _cc ;

        motor::tool::player_controller_t pc ;
        motor::tool::timeline_t tl = motor::tool::timeline_t("my timeline") ;

        size_t cur_time = 0 ;

        //******************************************************************************************************
        virtual void_t on_init( void_t ) noexcept
        {
            {
                motor::application::window_info_t wi ;
                wi.x = 100 ;
                wi.y = 100 ;
                wi.w = 800 ;
                wi.h = 600 ;
                wi.gen = motor::application::graphics_generation::gen4_auto ;

                this_t::send_window_message( this_t::create_window( wi ), [&] ( motor::application::app::window_view & wnd )
                {
                    wnd.send_message( motor::application::show_message( { true } ) ) ;
                    wnd.send_message( motor::application::cursor_message_t( { true } ) ) ;
                    wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                } ) ;
            }
           
            #if 0
            {
                motor::application::window_info_t wi ;
                wi.x = 400 ;
                wi.y = 100 ;
                wi.w = 800 ;
                wi.h = 600 ;
                wi.gen = motor::application::graphics_generation::gen4_gl4 ;

                this_t::send_window_message( this_t::create_window( wi ), [&] ( motor::application::app::window_view & wnd )
                {
                    wnd.send_message( motor::application::show_message( { true } ) ) ;
                    wnd.send_message( motor::application::cursor_message_t( { true } ) ) ;
                    wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                } ) ;
            }
            #endif

            pr.init( "my_prim_render" ) ;


            for( size_t i=0; i<2; ++i )
            {
                camera[i].set_dims( 1.0f, 1.0f, 1.0f, 10000.0f ) ;
                camera[i].perspective_fov( motor::math::angle<float_t>::degree_to_radian( 45.0f ) ) ;
                camera[i].look_at( motor::math::vec3f_t( 0.0f, 0.0f, -500.0f ),
                    motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ), motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) ) ;
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
                    rss.clear_s.ss.clear_color = motor::math::vec4f_t( 0.5f, 0.2f, 0.2f, 1.0f ) ;
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

        //******************************************************************************************************
        virtual void_t on_event( window_id_t const wid,
            motor::application::window_message_listener::state_vector_cref_t sv ) noexcept
        {
            if ( sv.create_changed )
            {
                motor::log::global_t::status( "[my_app] : window created" ) ;
            }
            if ( sv.close_changed )
            {
                motor::log::global_t::status( "[my_app] : window closed" ) ;
                this->close() ;
            }
            if ( sv.resize_changed )
            {
                float_t const w = float_t( sv.resize_msg.w ) ;
                float_t const h = float_t( sv.resize_msg.h ) ;

                for ( size_t i = 0; i < 2; ++i )
                {
                    camera[i].set_sensor_dims( w, h ) ;
                    camera[i].perspective_fov() ;

                }
            }
        }

        //******************************************************************************************************
        virtual void_t on_device( device_data_in_t dd ) noexcept
        {
            bool_t ctrl = false ;

            // keyboard testing
            {
                motor::controls::types::ascii_keyboard_t keyboard( dd.ascii ) ;

                using layout_t = motor::controls::types::ascii_keyboard_t ;
                using key_t = layout_t::ascii_key ;

                auto const left = keyboard.get_state( key_t::a ) ;
                auto const right = keyboard.get_state( key_t::d ) ;
                auto const forw = keyboard.get_state( key_t::w ) ;
                auto const back = keyboard.get_state( key_t::s ) ;
                auto const asc = keyboard.get_state( key_t::q ) ;
                auto const dsc = keyboard.get_state( key_t::e ) ;

                _cc.move_left = left != motor::controls::components::key_state::none ;
                _cc.move_right = right != motor::controls::components::key_state::none ;
                _cc.move_forwards = forw != motor::controls::components::key_state::none ;
                _cc.move_backwards = back != motor::controls::components::key_state::none ;
                _cc.move_upwards = asc != motor::controls::components::key_state::none ;
                _cc.move_downwards = dsc != motor::controls::components::key_state::none ;

                ctrl = keyboard.get_state( key_t::ctrl_left ) != 
                    motor::controls::components::key_state::none ;


                if ( keyboard.get_state( key_t::k_1 ) ==
                    motor::controls::components::key_state::released )
                {
                    cam_idx = 0 ;
                }
                else if ( keyboard.get_state( key_t::k_2 ) ==
                    motor::controls::components::key_state::released )
                {
                    cam_idx = 1 ;
                }
                else if ( keyboard.get_state( key_t::k_3 ) ==
                    motor::controls::components::key_state::released )
                {
                    cam_idx = ++cam_idx % 2 ;
                }
            }

            // mouse testing
            {
                motor::controls::types::three_mouse_t mouse( dd.mouse ) ;

                motor::math::vec2f_t const mouse_coords = mouse.get_local() ;
                auto const dif = mouse_coords - _cc.mouse_coords ;
                _cc.mouse_coords = mouse_coords ;

                auto button_funk = [&] ( motor::controls::types::three_mouse_t::button const button )
                {
                    if ( mouse.is_pressed( button ) )
                    {
                        return true ;
                    }
                    else if ( mouse.is_pressing( button ) )
                    {
                        return true ;
                    }
                    else if ( mouse.is_released( button ) )
                    {
                    }
                    return false ;
                } ;

                auto const l = button_funk( motor::controls::types::three_mouse_t::button::left ) ;
                auto const r = button_funk( motor::controls::types::three_mouse_t::button::right ) ;
                auto const m = button_funk( motor::controls::types::three_mouse_t::button::middle ) ;

                _cc.rotate_x = r ? int_t( -motor::math::fn<float_t>::sign( dif.y() ) ) : 0 ;
                _cc.rotate_y = r ? int_t( +motor::math::fn<float_t>::sign( dif.x() ) ) : 0 ;
                _cc.rotate_z = ctrl ? int_t( +motor::math::fn<float_t>::sign( dif.x() ) ) : 0 ;
            }
        }

        //******************************************************************************************************
        virtual void_t on_graphics( motor::application::app::graphics_data_in_t gd ) noexcept
        {
            // global time 
            size_t const time = cur_time ;

            // change free camera
            if( cam_idx == 0 )
            {
                // change camera translation
                {
                    motor::math::vec3f_t translate ;

                    // left/right
                    {
                        if ( _cc.move_left )
                        {
                            translate.x( -1000.0f * gd.sec_dt ) ;
                        }
                        else if ( _cc.move_right )
                        {
                            translate.x( +1000.0f * gd.sec_dt ) ;
                        }
                    }

                    // forwards/backwards
                    {
                        if ( _cc.move_backwards )
                        {
                            translate.z( -1000.0f * gd.sec_dt ) ;
                        }
                        else if ( _cc.move_forwards )
                        {
                            translate.z( +1000.0f * gd.sec_dt ) ;
                        }
                    }

                    // upwards/downwards
                    {
                        if ( _cc.move_upwards )
                        {
                            translate.y( -1000.0f * gd.sec_dt ) ;
                        }
                        else if ( _cc.move_downwards )
                        {
                            translate.y( +1000.0f * gd.sec_dt ) ;
                        }
                    }

                    camera[ 0 ].translate_by( translate ) ;
                }

                // change camera rotation
                {
                    motor::math::vec3f_t const angle(
                        float_t( _cc.rotate_x ) * 2.0f * gd.sec_dt,
                        float_t( _cc.rotate_y ) * 2.0f * gd.sec_dt,
                        float_t( _cc.rotate_z ) * 2.0f * gd.sec_dt ) ;

                    motor::math::quat4f_t const x( angle.x(), motor::math::vec3f_t( 1.0f, 0.0f, 0.0f ) ) ;
                    motor::math::quat4f_t const y( angle.y(), motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ) ) ;
                    motor::math::quat4f_t const z( angle.z(), motor::math::vec3f_t( 0.0f, 0.0f, 1.0f ) ) ;

                    auto const final_axis = x * y * z ;

                    auto const orientation = final_axis.to_matrix() ;

                    auto const t = motor::math::m3d::trafof_t::rotation_by_matrix( orientation ) ;

                    camera[ 0 ].transform_by( t ) ;
                }
            }

            // change other camera
            {
                typedef motor::math::linear_bezier_spline< motor::math::vec3f_t > linearf_t ;
                typedef motor::math::cubic_hermit_spline< motor::math::vec3f_t > splinef_t ;

                typedef motor::math::keyframe_sequence< splinef_t > keyframe_sequencef_t ;
                typedef motor::math::keyframe_sequence< splinef_t > keyframe_sequencef_t ;

                keyframe_sequencef_t kf( motor::math::time_remap_funk_type::cycle ) ;
                kf.insert( keyframe_sequencef_t::keyframe_t( 0, motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) ) ) ;
                kf.insert( keyframe_sequencef_t::keyframe_t( 1000, motor::math::vec3f_t( 1000.0f, 0.0f, 0.0f ) ) ) ;
                kf.insert( keyframe_sequencef_t::keyframe_t( 3000, motor::math::vec3f_t( 0.0f, 500.0f, 0.0f ) ) ) ;
                kf.insert( keyframe_sequencef_t::keyframe_t( 3500, motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) ) ) ;

                keyframe_sequencef_t kf2( motor::math::time_remap_funk_type::cycle ) ;
                kf2.insert( keyframe_sequencef_t::keyframe_t( 0, motor::math::vec3f_t( 0.0f, 0.0f, -1000.0f ) ) ) ;
                kf2.insert( keyframe_sequencef_t::keyframe_t( 1000, motor::math::vec3f_t( 1000.0f, 0.0f, -1000.0f ) ) ) ;
                kf2.insert( keyframe_sequencef_t::keyframe_t( 3000, motor::math::vec3f_t( 0.0f, 500.0f, -1000.0f ) ) ) ;
                kf2.insert( keyframe_sequencef_t::keyframe_t( 5000, motor::math::vec3f_t( -1000.0f, -100.0f, -1000.0f ) ) ) ;
                kf2.insert( keyframe_sequencef_t::keyframe_t( 6500, motor::math::vec3f_t( 0.0f, 0.0f, -1000.0f ) ) ) ;

                motor::math::vec3f_t const up = motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ).normalized() ;
                camera[ 1 ].look_at( kf2( time ), up , kf(time) ) ;

                // draw keyframe sequance as path
                if( cam_idx == 0 )
                {
                    splinef_t spline = kf2.get_spline() ;

                    // draw spline using lines
                    {
                        size_t const num_steps = 300 ;
                        for ( size_t i = 0; i < num_steps - 1; ++i )
                        {
                            float_t const t0 = float_t( i + 0 ) / float_t ( num_steps - 1 ) ;
                            float_t const t1 = float_t( i + 1 ) / float_t ( num_steps - 1 ) ;

                            auto const v0 = spline( t0 ) ;
                            auto const v1 = spline( t1 ) ;

                            pr.draw_line( v0, v1, motor::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ) ) ;
                        }
                    }

                    // draw current position
                    {
                        pr.draw_circle( motor::math::mat3f_t::make_identity(), kf2( time ), 10.0f,
                            motor::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ), motor::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ), 20 ) ;
                    }

                    // draw control points
                    {
                        auto const points = spline.control_points() ;
                        for( size_t i=0; i<points.size(); ++i )
                        {
                            auto const pos = points[i] ;
                            pr.draw_circle( motor::math::mat3f_t::make_identity(), pos, 10.0f,
                                motor::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ), motor::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ), 20 ) ;
                        }
                    }
                }
            }

            // draw camera view volume
            if( cam_idx != 1 )
            {
                motor::gfx::generic_camera_ptr_t cam = &camera[1] ;

                // 0-3 : front plane
                // 4-7 : back plane
                motor::math::vec3f_t points[8] ;

                if( cam->is_perspective() )
                {
                    auto const frust = cam->get_frustum() ;

                    //auto const nf = cam->get_near_far() ;
                    auto const nf = motor::math::vec2f_t( 50.0f, 1000.0f ) ;

                    auto const cs = cam->near_far_plane_half_dims( nf ) ;
                    {
                        motor::math::vec3f_t const scale( cs.x(), cs.y(), nf.x() ) ;
                        points[ 0 ] = motor::math::vec3f_t( -1.0f, -1.0f, 1.0f ) * scale ;
                        points[ 1 ] = motor::math::vec3f_t( -1.0f, +1.0f, 1.0f ) * scale ;
                        points[ 2 ] = motor::math::vec3f_t( +1.0f, +1.0f, 1.0f ) * scale ;
                        points[ 3 ] = motor::math::vec3f_t( +1.0f, -1.0f, 1.0f ) * scale ;
                    }

                    {
                        motor::math::vec3f_t const scale( cs.z(), cs.w(), nf.y() ) ;
                        points[ 4 ] = motor::math::vec3f_t( -1.0f, -1.0f, 1.0f ) * scale ;
                        points[ 5 ] = motor::math::vec3f_t( -1.0f, +1.0f, 1.0f ) * scale ;
                        points[ 6 ] = motor::math::vec3f_t( +1.0f, +1.0f, 1.0f ) * scale ;
                        points[ 7 ] = motor::math::vec3f_t( +1.0f, -1.0f, 1.0f ) * scale ;
                    }
                }
                else if( cam->is_orthographic() )
                {
                    auto const nf = motor::math::vec2f_t( 50.0f, 1000.0f ) ;

                    auto const cs = cam->get_dims().xy() * motor::math::vec2f_t(0.5f) ;
                    {
                        motor::math::vec3f_t const scale( cs.x(), cs.y(), nf.x() ) ;
                        points[ 0 ] = motor::math::vec3f_t( -1.0f, -1.0f, 1.0f ) * scale ;
                        points[ 1 ] = motor::math::vec3f_t( -1.0f, +1.0f, 1.0f ) * scale ;
                        points[ 2 ] = motor::math::vec3f_t( +1.0f, +1.0f, 1.0f ) * scale ;
                        points[ 3 ] = motor::math::vec3f_t( +1.0f, -1.0f, 1.0f ) * scale ;
                    }

                    {
                        motor::math::vec3f_t const scale( cs.x(), cs.y(), nf.y() ) ;
                        points[ 4 ] = motor::math::vec3f_t( -1.0f, -1.0f, 1.0f ) * scale ;
                        points[ 5 ] = motor::math::vec3f_t( -1.0f, +1.0f, 1.0f ) * scale ;
                        points[ 6 ] = motor::math::vec3f_t( +1.0f, +1.0f, 1.0f ) * scale ;
                        points[ 7 ] = motor::math::vec3f_t( +1.0f, -1.0f, 1.0f ) * scale ;
                    }
                }

                for( size_t i=0; i<8; ++i )
                {
                    points[i] = (cam->get_transformation().get_transformation() * motor::math::vec4f_t( points[i], 1.0f )).xyz() ;
                }

                // front
                for( size_t i=0; i<4; ++i )
                {
                    size_t const i0 = i + 0 ;
                    size_t const i1 = (i + 1) % 4 ;
                    pr.draw_line( points[ i0 ], points[ i1 ], motor::math::vec4f_t( 1.0f ) ) ;
                }

                // back
                for ( size_t i = 0; i < 4; ++i )
                {
                    size_t const i0 = (i + 0) + 4;
                    size_t const i1 = (( i + 1 ) % 4) + 4 ;
                    pr.draw_line( points[ i0 ], points[ i1 ], motor::math::vec4f_t( 1.0f ) ) ;
                }


                // sides
                for ( size_t i = 0; i < 4; ++i )
                {
                    size_t const i0 = ( i + 0 ) ;
                    size_t const i1 = ( i + 4 ) % 8 ;
                    pr.draw_line( points[ i0 ], points[ i1 ], motor::math::vec4f_t( 1.0f ) ) ;
                }
            }

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

                motor::math::vec4f_t color = motor::math::vec4f_t( kf( ltime ), 1.0f ) ;

                {
                    motor::math::vec3f_t const start( -1000.0f, 0.0f, 0.0f ) ;
                    size_t const ne = 100 ;
                    float_t const step = 1.0f / ne ;
                    for ( size_t i = 0; i < ne; ++i )
                    {
                        float_t const i_f = float_t( i ) / ne ;
                        motor::math::vec3f_t pos = start + motor::math::vec3f_t( float_t( i ), 1.0f, 1.0f ) * motor::math::vec3f_t( 50.0f, 1.0f, 1.0f ) ;
                        pr.draw_circle( motor::math::mat3f_t::make_identity(), pos, 30.0f, color, color, 20 ) ;
                    }
                }

                {
                    motor::math::vec3f_t const start( -300.0f, -100.0f, -100.0f ) ;
                    size_t const ne = 100 ;
                    float_t const step = 1.0f / ne ;
                    for ( size_t i = 0; i < ne; ++i )
                    {
                        float_t const i_f = float_t( i ) * step ; 
                        float_t const sin_y = motor::math::fn<float_t>::sin( i_f * 2.0f * motor::math::constants<float_t>::pi() ) ;
                        motor::math::vec3f_t const off( 0.0f, 200.0f * sin_y, 0.0f ) ;
                        motor::math::vec3f_t pos = start + off + motor::math::vec3f_t( float_t( i ), 1.0f, 1.0f ) * motor::math::vec3f_t( 10.0f, 1.0f, 1.0f ) ;
                        pr.draw_circle( motor::math::mat3f_t::make_identity(), pos, 10.0f,
                            motor::math::vec4f_t( ( motor::math::vec4f_t( 1.0f ) - color ).xyz(), 1.0f ), color, 20 ) ;
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

                motor::math::vec3f_t const p0 = motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) + df * motor::math::vec3f_t( -1.0f, -1.0f, 1.0f ) ;
                motor::math::vec3f_t const p1 = motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) + df * motor::math::vec3f_t( -1.0f, 1.0f, -1.0f ) ;
                motor::math::vec3f_t const p2 = motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) + df * motor::math::vec3f_t( 1.0f, 1.0f, 1.0f ) ;
                motor::math::vec3f_t const p3 = motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) + df * motor::math::vec3f_t( 1.0f, -1.0f, -1.0f ) ;

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

                motor::math::vec4f_t color = motor::math::vec4f_t( 1.0f ) ;

                pr.draw_circle(
                    ( camera[cam_idx].get_transformation() ).get_rotation_matrix(), kf( ltime ), kf2( ltime2 ), color, color, 10 ) ;
            }

            pr.set_view_proj( camera[cam_idx].mat_view(), camera[cam_idx].mat_proj() ) ;
            pr.prepare_for_rendering() ;
        }

        //******************************************************************************************************
        virtual void_t on_render( this_t::window_id_t const wid, motor::graphics::gen4::frontend_ptr_t fe,
            motor::application::app::render_data_in_t rd ) noexcept
        {
            if ( rd.first_frame )
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

        //******************************************************************************************************
        virtual bool_t on_tool( this_t::window_id_t const wid, motor::application::app::tool_data_ref_t td ) noexcept 
        { 
            if ( ImGui::Begin( "Camera Window" ) )
            {
                {
                    int used_cam = int_t( cam_idx ) ;
                    ImGui::SliderInt( "Choose Camera", &used_cam, 0, 1 ) ;
                    cam_idx = std::min( size_t( used_cam ), size_t( 2 ) ) ;
                }

                {
                    auto const cam_pos = camera[ cam_idx ].get_position() ;
                    float x = cam_pos.x() ;
                    float y = cam_pos.y() ;
                    ImGui::SliderFloat( "Cur Cam X", &x, -100.0f, 100.0f ) ;
                    ImGui::SliderFloat( "Cur Cam Y", &y, -100.0f, 100.0f ) ;
                    camera[ cam_idx ].translate_to( motor::math::vec3f_t( x, y, cam_pos.z() ) ) ;

                }

                {
                    bool_t ortho = camera[ cam_idx ].is_orthographic() ;
                    if( ImGui::Checkbox( "Orthographic", &ortho ) )
                    {
                        if( ortho ) camera[ cam_idx ].orthographic() ;
                        else camera[ cam_idx ].perspective_fov() ;
                    }
                    

                }
            }
            ImGui::End() ;

            {
                motor::tool::time_info_t ti { 1000000, cur_time } ;

                {
                    // the timeline stores some state, so it 
                    // is defined further above
                    {
                        tl.begin( ti ) ;
                        tl.end() ;
                        cur_time = ti.cur_milli ;
                    }
                }

                // the player_controller stores some state, 
                // so it is defined further above
                {
                    auto const s = pc.do_tool( "Player" ) ;
                    if ( s == motor::tool::player_controller_t::player_state::stop )
                    {
                        cur_time = 0  ;
                    }
                    else if ( s == motor::tool::player_controller_t::player_state::play )
                    {
                        if ( ti.cur_milli >= ti.max_milli )
                        {
                            cur_time = 0 ;
                        }
                    }

                    if ( pc.get_state() == motor::tool::player_controller_t::player_state::play &&
                        wid == 0 )
                    {
                        cur_time += td.milli_dt ;
                    }

                    if ( cur_time > ti.max_milli )
                    {
                        cur_time = ti.max_milli ;
                        pc.set_stop() ;
                    }
                }
            }

            return true ; 
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
