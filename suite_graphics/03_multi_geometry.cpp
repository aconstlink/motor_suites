
#include <motor/platform/global.h>

#include <motor/graphics/frontend/gen4/frontend.hpp>
#include <motor/graphics/object/render_object.h>
#include <motor/graphics/object/geometry_object.h>

#include <motor/gfx/camera/generic_camera.h>
#include <motor/math/utility/3d/transformation.hpp>
#include <motor/math/utility/angle.hpp>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>

#include <motor/math/camera/3d/orthographic_projection.hpp>
#include <motor/math/camera/3d/perspective_fov.hpp>

#include <future>

namespace this_file
{
    using namespace motor::core::types ;

    class my_app : public motor::application::app
    {
        motor_this_typedefs( my_app ) ;

        motor::math::vec4ui_t fb_dims = motor::math::vec4ui_t(0, 0, 1920, 1080) ;

        motor::graphics::state_object_t scene_so ;
        motor::graphics::geometry_object_t geo_obj0 ;
        motor::graphics::geometry_object_t geo_obj1 ;
        motor::graphics::msl_object_mtr_t msl_obj_scene ;
                
        motor::graphics::msl_object_mtr_t msl_obj ;

        motor::graphics::state_object_t fb_so ;
        motor::graphics::framebuffer_object_t fb_obj ;
        motor::graphics::geometry_object_t fb_geo ;

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
                    wnd.send_message( motor::application::cursor_message_t( {true} ) ) ;
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
                    wnd.send_message( motor::application::cursor_message_t( {true} ) ) ;
                    wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                } ) ;
            }

            {
                camera.perspective_fov( motor::math::angle<float_t>::degree_to_radian( 45.0f ) ) ;

                //
                // @todo: fix this
                // why setting camera position two times?
                //
                {
                    camera.look_at( motor::math::vec3f_t( 0.0f, 100.0f, -1000.0f ),
                            motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ), motor::math::vec3f_t( 0.0f, 100.0f, 0.0f )) ;
                }

                camera.set_transformation( motor::math::m3d::trafof_t( 1.0f, 
                    motor::math::vec3f_t(0.0f,0.0f,0.0f), 
                    motor::math::vec3f_t(0.0f,0.0f,-1000.0f) ) ) ;
            }
            {
                motor::graphics::render_state_sets_t rss ;
                rss.depth_s.do_change = true ;
                rss.depth_s.ss.do_activate = false ;
                rss.depth_s.ss.do_depth_write = false ;
                rss.polygon_s.do_change = true ;
                rss.polygon_s.ss.do_activate = true ;
                rss.polygon_s.ss.ff = motor::graphics::front_face::clock_wise ;
                rss.polygon_s.ss.cm = motor::graphics::cull_mode::back ;
                rss.polygon_s.ss.fm = motor::graphics::fill_mode::fill ;
                rss.clear_s.do_change = true ;
                rss.clear_s.ss.clear_color = motor::math::vec4f_t(0.5f, 0.9f, 0.5f, 1.0f ) ;
                rss.clear_s.ss.do_activate = true ;
                rss.clear_s.ss.do_color_clear = true ;
                rss.clear_s.ss.do_depth_clear = true ;
                rss.view_s.do_change = true ;
                rss.view_s.ss.do_activate = true ;
                rss.view_s.ss.vp = fb_dims ;

                scene_so = motor::graphics::state_object_t("scene_render_states") ;
                scene_so.add_render_state_set( rss ) ;
            }

            {
                motor::graphics::render_state_sets_t rss ;
                rss.depth_s.do_change = true ;
                rss.depth_s.ss.do_activate = false ;
                rss.depth_s.ss.do_depth_write = false ;

                rss.polygon_s.ss.do_activate = true ;
                rss.polygon_s.ss.ff = motor::graphics::front_face::clock_wise ;
                rss.polygon_s.ss.cm = motor::graphics::cull_mode::back ;

                rss.clear_s.do_change = false ;

                fb_so = motor::graphics::state_object_t("framebuffer_render_states") ;
                fb_so.add_render_state_set( rss ) ;
            }
                
            struct vertex { motor::math::vec4f_t pos ; motor::math::vec4f_t col ; } ;

            // geometry configuration 1
            {
                auto vb = motor::graphics::vertex_buffer_t()
                    .add_layout_element( motor::graphics::vertex_attribute::position, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                    .add_layout_element( motor::graphics::vertex_attribute::color0, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                    .resize( 4 ).update<vertex>( [=] ( vertex* array, size_t const ne )
                {
                    array[ 0 ].pos = motor::math::vec4f_t( -0.5f, -0.5f, 1.5f, 1.0f ) ;
                    array[ 1 ].pos = motor::math::vec4f_t( -0.5f, +0.5f, 1.5f, 1.0f ) ;
                    array[ 2 ].pos = motor::math::vec4f_t( +0.5f, +0.5f, 1.5f, 1.0f ) ;
                    array[ 3 ].pos = motor::math::vec4f_t( +0.5f, -0.5f, 1.5f, 1.0f ) ;

                    array[ 0 ].col = motor::math::vec4f_t( 1.0f, 0.0f,0.0f,1.0f ) ;
                    array[ 1 ].col = motor::math::vec4f_t( 1.0f, 0.0f,0.0f,1.0f ) ;
                    array[ 2 ].col = motor::math::vec4f_t( 1.0f, 0.0f,0.0f,1.0f ) ;
                    array[ 3 ].col = motor::math::vec4f_t( 1.0f, 0.0f,0.0f,1.0f ) ;
                } );

                auto ib = motor::graphics::index_buffer_t().
                    set_layout_element( motor::graphics::type::tuint ).resize( 6 ).
                    update<uint_t>( [] ( uint_t* array, size_t const ne )
                {
                    array[ 0 ] = 0 ;
                    array[ 1 ] = 1 ;
                    array[ 2 ] = 2 ;

                    array[ 3 ] = 0 ;
                    array[ 4 ] = 2 ;
                    array[ 5 ] = 3 ;
                } ) ;

                geo_obj0 = motor::graphics::geometry_object_t( "geo0",
                    motor::graphics::primitive_type::triangles, std::move( vb ), std::move( ib ) ) ;
                    
            }

            // geometry configuration 2
            {
                auto vb = motor::graphics::vertex_buffer_t()
                    .add_layout_element( motor::graphics::vertex_attribute::position, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                    .add_layout_element( motor::graphics::vertex_attribute::color0, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                    .resize( 9 ).update<vertex>( [=] ( vertex* array, size_t const ne )
                {
                    array[ 0 ].pos = motor::math::vec4f_t( -0.25f, +0.50f, 0.0f, 1.0f ) ;
                    array[ 1 ].pos = motor::math::vec4f_t( +0.25f, +0.50f, 0.0f, 1.0f ) ;
                    array[ 2 ].pos = motor::math::vec4f_t( +0.50f, +0.25f, 0.0f, 1.0f ) ;
                    array[ 3 ].pos = motor::math::vec4f_t( +0.50f, -0.25f, 0.0f, 1.0f ) ;
                    array[ 4 ].pos = motor::math::vec4f_t( +0.25f, -0.50f, 0.0f, 1.0f ) ;
                    array[ 5 ].pos = motor::math::vec4f_t( -0.25f, -0.50f, 0.0f, 1.0f ) ;
                    array[ 6 ].pos = motor::math::vec4f_t( -0.50f, -0.25f, 0.0f, 1.0f ) ;
                    array[ 7 ].pos = motor::math::vec4f_t( -0.50f, +0.25f, 0.0f, 1.0f ) ;
                    array[ 8 ].pos = motor::math::vec4f_t( 0.0f, 0.0f, 0.0f, 1.0f ) ;

                    array[ 0 ].col = motor::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    array[ 1 ].col = motor::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    array[ 2 ].col = motor::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    array[ 3 ].col = motor::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    array[ 4 ].col = motor::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    array[ 5 ].col = motor::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    array[ 6 ].col = motor::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    array[ 7 ].col = motor::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    array[ 8 ].col = motor::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ) ;
                } );

                auto ib = motor::graphics::index_buffer_t().
                    set_layout_element( motor::graphics::type::tuint ).resize( 24 ).
                    update<uint_t>( [] ( uint_t* array, size_t const ne )
                {
                    array[ 0 ] = 0 ;
                    array[ 1 ] = 1 ;
                    array[ 2 ] = 8 ;

                    array[ 3 ] = 1 ;
                    array[ 4 ] = 2 ;
                    array[ 5 ] = 8 ;

                    array[ 6 ] = 2 ;
                    array[ 7 ] = 3 ;
                    array[ 8 ] = 8 ;

                    array[ 9 ] = 3 ;
                    array[ 10 ] = 4 ;
                    array[ 11 ] = 8 ;

                    array[ 12 ] = 4 ;
                    array[ 13 ] = 5 ;
                    array[ 14 ] = 8 ;

                    array[ 15 ] = 5 ;
                    array[ 16 ] = 6 ;
                    array[ 17 ] = 8 ;

                    array[ 18 ] = 6 ;
                    array[ 19 ] = 7 ;
                    array[ 20 ] = 8 ;

                    array[ 21 ] = 7 ;
                    array[ 22 ] = 0 ;
                    array[ 23 ] = 8 ;
                } ) ;

                geo_obj1 = motor::graphics::geometry_object_t( "geo1",
                    motor::graphics::primitive_type::triangles, std::move( vb ), std::move( ib ) ) ;
            }

            struct vertex1 { motor::math::vec2f_t pos ; } ;

            // geometry configuration for post quad
            {
                auto vb = motor::graphics::vertex_buffer_t()
                    .add_layout_element( motor::graphics::vertex_attribute::position, motor::graphics::type::tfloat, motor::graphics::type_struct::vec2 )
                    .resize( 4 ).update<vertex1>( [=] ( vertex1* array, size_t const ne )
                {
                    array[ 0 ].pos = motor::math::vec2f_t( -0.5f, -0.5f ) ;
                    array[ 1 ].pos = motor::math::vec2f_t( -0.5f, +0.5f ) ;
                    array[ 2 ].pos = motor::math::vec2f_t( +0.5f, +0.5f ) ;
                    array[ 3 ].pos = motor::math::vec2f_t( +0.5f, -0.5f ) ;
                } );

                auto ib = motor::graphics::index_buffer_t().
                    set_layout_element( motor::graphics::type::tuint ).resize( 6 ).
                    update<uint_t>( [] ( uint_t* array, size_t const ne )
                {
                    array[ 0 ] = 0 ;
                    array[ 1 ] = 1 ;
                    array[ 2 ] = 2 ;

                    array[ 3 ] = 0 ;
                    array[ 4 ] = 2 ;
                    array[ 5 ] = 3 ;
                } ) ;

                fb_geo = motor::graphics::geometry_object_t( "quad",
                    motor::graphics::primitive_type::triangles, std::move( vb ), std::move( ib ) ) ;
            }

            // msl object for scene
            {
                // quad object
                {
                    motor::graphics::msl_object_t mslo("scene") ;

                    mslo.add( motor::graphics::msl_api_type::msl_4_0, R"(
                    config test_multi_geometry
                    {
                        vertex_shader
                        {
                            in vec4_t pos : position ;
                            in vec4_t col : color0 ;
                            out vec4_t pos : position ;
                            out vec4_t col : color ;

                            mat4_t u_proj : projection ;
                            mat4_t u_view : view ;
                            mat4_t u_world : world ;

                            void main()
                            {
                                out.col = in.col ;
                                out.pos = u_proj * u_view * u_world * in.pos ;
                            }
                        }

                        pixel_shader
                        {
                            in vec4_t col : color0 ;
                            out vec4_t color : color0 ;

                            void main()
                            {
                                out.color = in.col ;
                            }
                        }
                    })" ) ;

                        
                    mslo.link_geometry({"geo0","geo1"}) ;

                    msl_obj_scene = motor::shared( std::move( mslo ) ) ;
                }
                    
                {
                    motor::graphics::variable_set_t vars ;
                    msl_obj_scene->add_variable_set( motor::memory::create_ptr( std::move( vars ), "a variable set" ) ) ;
                }
                
                {
                    motor::graphics::variable_set_t vars ;
                    msl_obj_scene->add_variable_set( motor::memory::create_ptr( std::move(vars), "a variable set" ) ) ;
                }
            }

            // shaders
            {
                // post quad object
                {
                    motor::graphics::msl_object_t mslo("post_quad") ;

                    mslo.add( motor::graphics::msl_api_type::msl_4_0, R"(
                    config post_quad
                    {
                        vertex_shader
                        {
                            in vec2_t pos : position ;

                            out vec2_t tx : texcoord0 ;
                            out vec4_t pos : position ;

                            void main()
                            {
                                out.tx = sign( in.pos.xy ) *0.5 + 0.5 ;
                                out.pos = vec4_t( sign( in.pos.xy ), 0.0, 1.0 ) ; 
                            }
                        }

                        pixel_shader
                        {
                            tex2d_t u_tex ;

                            in vec2_t tx : texcoord0 ;
                            out vec4_t color : color ;

                            void main()
                            {
                                out.color = texture( u_tex, in.tx ) ;
                            }
                        }
                    } )" ) ;

                    mslo.link_geometry("quad") ;

                    msl_obj = motor::shared( std::move( mslo ) ) ;
                }

                // variable sets
                {
                    motor::graphics::variable_set_t vars ;
                        
                    {
                        auto * var = vars.texture_variable( "u_tex" ) ;
                        var->set( "the_scene.0" ) ;
                    }

                    msl_obj->add_variable_set( motor::memory::create_ptr( std::move(vars), "a variable set" ) ) ;
                }
            }

            // framebuffer
            {
                fb_obj = motor::graphics::framebuffer_object_t( "the_scene" ) ;
                fb_obj.set_target( motor::graphics::color_target_type::rgba_uint_8, 1 )
                    .resize( fb_dims.z(), fb_dims.w() ) ;
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
                camera.set_dims( w, h, 0.1f, 1000.0f ) ;
                camera.perspective_fov() ;
            }
        }

        virtual void_t on_render( this_t::window_id_t const wid, motor::graphics::gen4::frontend_ptr_t fe,
            motor::application::app::render_data_in_t rd ) noexcept 
        {            
            if( rd.first_frame )
            {
                {
                    fe->configure<motor::graphics::state_object_t>( &scene_so ) ;
                    fe->configure<motor::graphics::state_object_t>( &fb_so ) ;
                }

                {
                    fe->configure<motor::graphics::geometry_object_t>( &geo_obj0 ) ;
                    fe->configure<motor::graphics::geometry_object_t>( &geo_obj1 ) ;
                    fe->configure<motor::graphics::msl_object_t>( msl_obj_scene ) ;
                }

                {
                    fe->configure<motor::graphics::geometry_object_t>( &fb_geo ) ;
                    fe->configure<motor::graphics::framebuffer_object_t>( &fb_obj ) ;
                    fe->configure<motor::graphics::msl_object_t>( msl_obj ) ;
                }
            }

            // render
            {
                {
                    size_t i = 0 ;
                    for( auto * vs : msl_obj_scene->borrow_varibale_sets() )
                    {
                        {
                            auto * var = vs->data_variable<motor::math::mat4f_t>( "u_proj" ) ;
                            var->set( camera.mat_proj() ) ;
                        }
                        
                        {
                            auto * var = vs->data_variable<motor::math::mat4f_t>("u_view") ;
                            var->set( camera.mat_view() ) ;
                        }
                        
                        {
                            auto const of = float_t(i) * 2.0 - 1.0 ;
                            motor::math::m3d::trafof_t t ;
                            t.scale_fl( 100.0f ) ;
                            t.translate_fl( motor::math::vec3f_t( 100.0f * (float_t(i) * 2.0f - 1.0f), 0.0f, 0.0f ) ) ;

                            auto * var = vs->data_variable<motor::math::mat4f_t>("u_world") ;
                            var->set( t.get_transformation() ) ;
                        }
                        
                        ++i ;
                    }
                }

                
                fe->use( &fb_obj ) ;
                fe->push( &scene_so ) ;
                {
                    motor::graphics::gen4::backend_t::render_detail_t detail ;
                    detail.varset = 0 ;
                    detail.geo = 0 ;
                    fe->render(  msl_obj_scene, detail ) ;
                }
                {
                    motor::graphics::gen4::backend_t::render_detail_t detail ;
                    detail.varset = 1 ;
                    detail.geo = 1 ;
                    fe->render(  msl_obj_scene, detail ) ;
                }
                fe->pop( motor::graphics::gen4::backend::pop_type::render_state ) ; 
                fe->unuse( motor::graphics::gen4::backend::unuse_type::framebuffer ) ;

                fe->push( &fb_so ) ;
                {
                    motor::graphics::gen4::backend_t::render_detail_t detail ;
                    detail.varset = 0 ;
                    fe->render(  msl_obj, detail ) ;
                }
                fe->pop( motor::graphics::gen4::backend::pop_type::render_state ) ;
                fe->fence( [=]( void_t ){} ) ;
            }
        }

        virtual void_t on_shutdown( void_t ) noexcept 
        {
            motor::release( motor::move( msl_obj ) ) ;
            motor::release( motor::move( msl_obj_scene ) ) ;
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
