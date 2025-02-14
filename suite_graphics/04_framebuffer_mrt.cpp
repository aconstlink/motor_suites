
#include <motor/platform/global.h>
#include <motor/application/window/window_message_listener.h>

#include <motor/graphics/frontend/gen4/frontend.hpp>

#include <motor/graphics/object/render_object.h>
#include <motor/graphics/object/geometry_object.h>

#include <motor/geometry/mesh/tri_mesh.h>
#include <motor/geometry/mesh/flat_tri_mesh.h>
#include <motor/geometry/3d/cube.h>

#include <motor/gfx/camera/generic_camera.h>
#include <motor/math/utility/3d/transformation.hpp>
#include <motor/math/utility/angle.hpp>

#include <motor/tool/imgui/imgui.h>

#include <motor/concurrent/global.h>
#include <motor/log/global.h>
#include <motor/memory/global.h>

#include <future>

namespace this_file
{
    using namespace motor::core::types ;

    class my_app : public motor::application::app
    {
        motor_this_typedefs( my_app ) ;

        motor::math::vec4ui_t fb_dims = motor::math::vec4ui_t( 0, 0, 1920, 1080 ) ;

        motor::graphics::state_object_t scene_so ;
        motor::graphics::msl_object_mtr_t msl_obj ;
        motor::graphics::geometry_object_t geo_obj ;
        motor::graphics::image_object_t img_obj ;

        motor::graphics::msl_object_mtr_t fb_msl ;

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
                camera.perspective_fov( motor::math::angle<float_t>::degree_to_radian( 90.0f ) ) ;

                //
                // @todo: fix this
                // why setting camera position two times?
                //
                {
                    camera.look_at( motor::math::vec3f_t( 0.0f, 60.0f, -50.0f ),
                            motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ), motor::math::vec3f_t( 0.0f, 0.0f, 0.0f )) ;
                }
                    
            }
            {
                motor::graphics::render_state_sets_t rss ;

                rss.depth_s.do_change = true ;
                rss.depth_s.ss.do_activate = true ;
                rss.depth_s.ss.do_depth_write = true ;

                rss.polygon_s.do_change = true ;
                rss.polygon_s.ss.do_activate = true ;
                rss.polygon_s.ss.ff = motor::graphics::front_face::counter_clock_wise ;
                rss.polygon_s.ss.cm = motor::graphics::cull_mode::back ;
                rss.polygon_s.ss.fm = motor::graphics::fill_mode::fill ;
                   
                rss.clear_s.do_change = true ;
                rss.clear_s.ss.do_activate = true ;
                rss.clear_s.ss.clear_color = motor::math::vec4f_t(1.0f,1.0f,0.0f,1.0f) ;
                rss.clear_s.ss.do_color_clear = true ;
                rss.clear_s.ss.do_depth_clear = true ;

                rss.view_s.do_change = true ;
                rss.view_s.ss.do_activate = true ;
                rss.view_s.ss.vp = fb_dims ;

                scene_so = motor::graphics::state_object_t( "scene_render_states" ) ;
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
                
            struct vertex { motor::math::vec3f_t pos ; motor::math::vec3f_t nrm ; motor::math::vec2f_t tx ; } ;

            // cube
            {
                motor::geometry::cube_t::input_params ip ;
                ip.scale = motor::math::vec3f_t( 1.0f ) ;
                ip.tess = 100 ;

                motor::geometry::tri_mesh_t tm ;
                motor::geometry::cube_t::make( &tm, ip ) ;
                
                motor::geometry::flat_tri_mesh_t ftm ;
                tm.flatten( ftm ) ;

                auto vb = motor::graphics::vertex_buffer_t()
                    .add_layout_element( motor::graphics::vertex_attribute::position, motor::graphics::type::tfloat, motor::graphics::type_struct::vec3 )
                    .add_layout_element( motor::graphics::vertex_attribute::normal, motor::graphics::type::tfloat, motor::graphics::type_struct::vec3 )
                    .add_layout_element( motor::graphics::vertex_attribute::texcoord0, motor::graphics::type::tfloat, motor::graphics::type_struct::vec2 )
                    .resize( ftm.get_num_vertices() ).update<vertex>( [&] ( vertex* array, size_t const ne )
                {
                    for( size_t i=0; i<ne; ++i )
                    {
                        array[ i ].pos = ftm.get_vertex_position_3d( i ) ;
                        array[ i ].nrm = ftm.get_vertex_normal_3d( i ) ;
                        array[ i ].tx = ftm.get_vertex_texcoord( 0, i ) ;
                    }
                } );

                auto ib = motor::graphics::index_buffer_t().
                    set_layout_element( motor::graphics::type::tuint ).resize( ftm.indices.size() ).
                    update<uint_t>( [&] ( uint_t* array, size_t const ne )
                {
                    for( size_t i = 0; i < ne; ++i ) array[ i ] = ftm.indices[ i ] ;
                } ) ;

                geo_obj = motor::graphics::geometry_object_t( "cube",
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
                    motor::graphics::msl_object_t mslo("scene_obj") ;

                    mslo.add( motor::graphics::msl_api_type::msl_4_0, R"(
                    config just_render
                    {
                        vertex_shader
                        {
                            mat4_t proj : projection ;
                            mat4_t view : view ;
                            mat4_t world : world ;

                            in vec3_t pos : position ;
                            in vec3_t nrm : normal ;
                            in vec2_t tx : texcoord ;

                            out vec4_t pos : position ;
                            out vec2_t tx : texcoord ;
                            out vec3_t nrm : normal ;

                            void main()
                            {
                                vec3_t pos = in.pos ;
                                pos.xyz = pos.xyz * 10.0 ;
                                out.tx = in.tx ;
                                out.pos = proj * view * world * vec4_t( pos, 1.0 ) ;
                                out.nrm = normalize( world * vec4_t( in.nrm, 0.0 ) ).xyz ;
                            }
                        }

                        pixel_shader
                        {
                            tex2d_t tex ;
                            vec4_t color ;

                            in vec2_t tx : texcoord ;
                            in vec3_t nrm : normal ;
                            out vec4_t color0 : color0 ;
                            out vec4_t color1 : color1 ;
                            out vec4_t color2 : color2 ;

                            void main()
                            {
                                float_t light = dot( normalize( in.nrm ), normalize( vec3_t( 1.0, 1.0, 0.5) ) ) ;
                                out.color0 = color ' texture( tex, in.tx ) ;
                                out.color1 = vec4_t( in.nrm, 1.0 ) ;
                                out.color2 = vec4_t( light, light, light , 1.0 ) ;
                            }
                        }
                    })" ) ;

                        
                    mslo.link_geometry({"cube"}) ;

                    msl_obj = motor::shared( std::move( mslo ) ) ;
                }
                    
                // the rendering objects
                size_t const num_object = 30 ;
                for( size_t i=0; i<num_object;++i )
                {
                    motor::graphics::variable_set_t vars ;

                    {
                        auto* var = vars.data_variable< motor::math::vec4f_t >( "color" ) ;
                        var->set( motor::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ) ) ;
                    }

                    {
                        auto* var = vars.data_variable< float_t >( "u_time" ) ;
                        var->set( 0.0f ) ;
                    }

                    {
                        auto* var = vars.texture_variable( "tex" ) ;
                        var->set( "checker_board" ) ;
                    }

                    {
                        float_t const angle = ( float( i ) / float_t( num_object - 1 ) ) * 2.0f * 
                            motor::math::constants<float_t>::pi() ;
                        
                        motor::math::m3d::trafof_t trans ;

                        motor::math::m3d::trafof_t rotation ;
                        rotation.rotate_by_axis_fr( motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ), angle ) ;
                        
                        motor::math::m3d::trafof_t translation ;
                        translation.translate_fr( motor::math::vec3f_t( 
                            0.0f,
                            10.0f * std::sin( (angle/4.0f) * 2.0f * motor::math::constants<float_t>::pi() ), 
                            -50.0f ) ) ;

                        trans.transform_fl( rotation ) ;
                        trans.transform_fl( translation ) ;
                        trans.transform_fl( rotation ) ;

                        auto* var = vars.data_variable< motor::math::mat4f_t >( "world" ) ;
                        var->set( trans.get_transformation() ) ;
                    }
                    msl_obj->add_variable_set( motor::memory::create_ptr( std::move( vars ), "a variable set" ) ) ;
                }
            }

            // shaders for post process
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
                            in vec2_t tx : texcoord0 ;
                            out vec4_t color : color ;
        
                            tex2d_t u_tex_0 ;
                            tex2d_t u_tex_1 ;
                            tex2d_t u_tex_2 ;
                            tex2d_t u_tex_3 ;

                            void main()
                            {
                                out.color = vec4_t(0.5,0.5,0.5,1.0) ;

                                if( in.tx.x < 0.5 && in.tx.y > 0.5 )
                                {
                                    vec2_t tx = (in.tx - vec2_t( 0.0, 0.5 ) ) * 2.0 ; 
                                    out.color = rt_texture( u_tex_0, tx ) ; 
                                }
                                else if( in.tx.x > 0.5 && in.tx.y > 0.5 )
                                {
                                    vec2_t tx = (in.tx - vec2_t( 0.5, 0.5 ) ) * 2.0 ; 
                                    out.color = rt_texture( u_tex_1, tx ) ; 
                                }
                                else if( in.tx.x > 0.5 && in.tx.y < 0.5 )
                                {
                                    vec2_t tx = (in.tx - vec2_t( 0.5, 0.0 ) ) * 2.0 ; 
                                    out.color = vec4_t( rt_texture( u_tex_2, tx ).xyz, 1.0 ); 
                                }
                                else if( in.tx.x < 0.5 && in.tx.y < 0.5 )
                                {
                                    vec2_t tx = (in.tx - vec2_t( 0.0, 0.0 ) ) * 2.0 ; 
                                    float_t p = pow( rt_texture( u_tex_3, tx ).r, 2.0 ) ;
                                    out.color = vec4_t( vec3_t(p,p,p), 1.0 ); 
                                }
                            }
                        }
                    } )" ) ;

                    mslo.link_geometry("quad") ;

                    fb_msl = motor::shared( std::move( mslo ) ) ;
                }

                // variable sets
                {
                    motor::graphics::variable_set_t vars ;
                        
                    {
                        auto * var = vars.texture_variable( "u_tex_0" ) ;
                        var->set( "the_scene.0" ) ;
                    }
                    {
                        auto * var = vars.texture_variable( "u_tex_1" ) ;
                        var->set( "the_scene.1" ) ;
                    }
                    {
                        auto * var = vars.texture_variable( "u_tex_2" ) ;
                        var->set( "the_scene.2" ) ;
                    }
                    {
                        auto * var = vars.texture_variable( "u_tex_3" ) ;
                        var->set( "the_scene.depth" ) ;
                    }

                    fb_msl->add_variable_set( motor::memory::create_ptr( std::move(vars), "a variable set" ) ) ;
                }
            }

            // image configuration
            {
                motor::graphics::image_t img = motor::graphics::image_t( motor::graphics::image_t::dims_t( 100, 100 ) )
                    .update( [&]( motor::graphics::image_ptr_t, motor::graphics::image_t::dims_in_t dims, void_ptr_t data_in )
                {
                    typedef motor::math::vector4< uint8_t > rgba_t ;
                    auto* data = reinterpret_cast< rgba_t * >( data_in ) ;

                    size_t const w = 5 ;

                    size_t i = 0 ; 
                    for( size_t y = 0; y < dims.y(); ++y )
                    {
                        bool_t const odd = ( y / w ) & 1 ;

                        for( size_t x = 0; x < dims.x(); ++x )
                        {
                            bool_t const even = ( x / w ) & 1 ;

                            data[ i++ ] = even || odd ? rgba_t( 255 ) : rgba_t( 0, 0, 0, 255 );
                            //data[ i++ ] = rgba_t(255) ;
                        }
                    }
                } ) ;

                img_obj = motor::graphics::image_object_t( "checker_board", std::move( img ) )
                    .set_wrap( motor::graphics::texture_wrap_mode::wrap_s, motor::graphics::texture_wrap_type::repeat )
                    .set_wrap( motor::graphics::texture_wrap_mode::wrap_t, motor::graphics::texture_wrap_type::repeat )
                    .set_filter( motor::graphics::texture_filter_mode::min_filter, motor::graphics::texture_filter_type::nearest )
                    .set_filter( motor::graphics::texture_filter_mode::mag_filter, motor::graphics::texture_filter_type::nearest );
            }

            // framebuffer
            {
                fb_obj = motor::graphics::framebuffer_object_t( "the_scene" ) ;
                fb_obj.set_target( motor::graphics::color_target_type::rgba_uint_8, 3 )
                    .set_target( motor::graphics::depth_stencil_target_type::depth32 )
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
            if( sv.resize_changed )
            {
                float_t const w = float_t(sv.resize_msg.w) ;
                float_t const h = float_t(sv.resize_msg.h) ;
                camera.set_dims( w, h, 1.0f, 100.0f ) ;
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
                    fe->configure<motor::graphics::geometry_object_t>( &geo_obj ) ;
                    fe->configure<motor::graphics::image_object_t>( &img_obj ) ;
                    fe->configure<motor::graphics::msl_object_t>( msl_obj ) ;
                }

                {
                    fe->configure<motor::graphics::geometry_object_t>( &fb_geo ) ;
                    fe->configure<motor::graphics::framebuffer_object_t>( &fb_obj ) ;
                    fe->configure<motor::graphics::msl_object_t>( fb_msl ) ;
                }
            }

            typedef std::chrono::high_resolution_clock __clock_t ;
            static __clock_t::time_point _tp = __clock_t::now() ;

            // render
            {
                static float_t v = 0.0f ;
                v += 0.01f ;
                if( v > 1.0f ) v = 0.0f ;

                static __clock_t::time_point tp = __clock_t::now() ;
                float_t const dt = float_t ( double_t( std::chrono::duration_cast< std::chrono::milliseconds >( 
                    __clock_t::now() - tp ).count() ) / 1000.0 ) ;
                tp = __clock_t::now() ;

                {
                    size_t i = 0 ;
                    for( auto * vs : msl_obj->borrow_varibale_sets() )
                    {
                        {
                            auto * var = vs->data_variable< motor::math::mat4f_t>("view") ;
                            var->set( camera.mat_view() ) ;
                        }

                        {
                            auto * var = vs->data_variable< motor::math::mat4f_t>("proj") ;
                            var->set( camera.mat_proj() ) ;
                        }

                        {
                            auto* var = vs->data_variable< motor::math::vec4f_t >( "color" ) ;
                            var->set( motor::math::vec4f_t( v, 0.0f, 1.0f, 0.5f ) ) ;
                        }

                        {
                            static float_t  angle = 0.0f ;
                            angle = ( ( (dt/10.0f)  ) * 2.0f * motor::math::constants<float_t>::pi() ) ;
                            if( angle > 2.0f * motor::math::constants<float_t>::pi() ) angle = 0.0f ;
                        
                            auto* var = vs->data_variable< motor::math::mat4f_t >( "world" ) ;
                            motor::math::m3d::trafof_t trans( var->get() ) ;

                            motor::math::m3d::trafof_t rotation ;
                            rotation.rotate_by_axis_fr( motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ), angle ) ;
                        
                            trans.transform_fl( rotation ) ;

                            var->set( trans.get_transformation() ) ;
                        }
                                
                        ++i ;
                    }
                }

                fe->use( &fb_obj ) ;
                fe->push( &scene_so ) ;

                auto var_sets = msl_obj->borrow_varibale_sets() ;

                for( size_t i=0; i<var_sets.size(); ++i )
                {
                    auto * var_set = var_sets[i] ;

                    motor::graphics::gen4::backend_t::render_detail_t detail ;
                    detail.varset = i ;
                    fe->render(  msl_obj, detail ) ;
                }

                fe->pop( motor::graphics::gen4::backend::pop_type::render_state ) ; 
                fe->unuse( motor::graphics::gen4::backend::unuse_type::framebuffer ) ;

                fe->push( &fb_so ) ;
                {
                    motor::graphics::gen4::backend_t::render_detail_t detail ;
                    detail.varset = 0 ;
                    fe->render(  fb_msl, detail ) ;
                }
                fe->pop( motor::graphics::gen4::backend::pop_type::render_state ) ;
                fe->fence( [=]( void_t ){} ) ;
            }
        }

        //******************************************************************************************************
        virtual bool_t on_tool( this_t::window_id_t const wid, motor::application::app::tool_data_ref_t td ) noexcept
        {
            if( ImGui::Begin("LookiLooki" ) )
            {
                ImGui::Image( td.imgui->texture( "the_scene.0" ), ImGui::GetWindowSize() ) ;
            }
            ImGui::End() ;
            return true ;
        }

        virtual void_t on_shutdown( void_t ) noexcept 
        {
            motor::release( motor::move( msl_obj ) ) ;
            motor::release( motor::move( fb_msl ) ) ;
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