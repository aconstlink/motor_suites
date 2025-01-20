
#include <motor/platform/global.h>
#include <motor/application/window/window_message_listener.h>

#include <motor/graphics/object/render_object.h>
#include <motor/graphics/object/geometry_object.h>

#include <motor/geometry/mesh/tri_mesh.h>
#include <motor/geometry/mesh/flat_tri_mesh.h>
#include <motor/geometry/3d/cube.h>

#include <motor/gfx/camera/generic_camera.h>
#include <motor/math/utility/3d/transformation.hpp>
#include <motor/math/utility/angle.hpp>
#include <motor/math/interpolation/interpolate.hpp>

#include <motor/concurrent/parallel_for.hpp>
#include <motor/log/global.h>
#include <motor/memory/global.h>

#include <future>

namespace this_file
{
    using namespace motor::core::types ;

    class my_app : public motor::application::app
    {
        motor_this_typedefs( my_app ) ;

        int_t num_objects = 0 ;
        int_t max_objects = 0 ;

        motor::math::vec4ui_t fb_dims = motor::math::vec4ui_t( 0, 0, 1920, 1080 ) ;

        motor::graphics::state_object_t scene_so ;
        motor::graphics::msl_object_t msl_obj ;
        motor::graphics::geometry_object_t geo_obj ;
        motor::graphics::image_object_t img_obj ;
        motor::graphics::array_object_t ar_obj ;

        motor::graphics::msl_object_t fb_msl ;
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

            // do camera
            {
                camera.set_dims( 800.0f, 600.0f, 1.0f, 1000000.0f ) ;
                camera.perspective_fov( motor::math::angle<float_t>::degree_to_radian( 90.0f ) ) ;
                camera.look_at( motor::math::vec3f_t( 100.0f, 4000.0f, 3000.0f ),
                    motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ), motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) ) ;
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
                rss.clear_s.ss.clear_color = motor::math::vec4f_t(0.4f,0.4f,0.0f,1.0f) ;
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
                
            {
                size_t const num_objects_x = 50 ;
                size_t const num_objects_y = 50 ;
                size_t const num_objects_ = num_objects_x * num_objects_y * 50 ;
                max_objects = num_objects_  ;
                num_objects = int_t(std::min( size_t(40000), size_t(num_objects_ / 2) )) ;
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
                    .resize( ftm.get_num_vertices() * max_objects ).update<vertex>( [&] ( vertex* array, size_t const ne )
                {
                    motor::concurrent::parallel_for<size_t>( motor::concurrent::range_1d<size_t>(0, max_objects),
                        [&]( motor::concurrent::range_1d<size_t> const & r )
                    {
                        for( size_t o=r.begin(); o<r.end(); ++o )
                        {
                            size_t const base = o * ftm.get_num_vertices() ;
                            for( size_t i=0; i<ftm.get_num_vertices(); ++i )
                            {
                                array[ base + i ].pos = ftm.get_vertex_position_3d( i ) ;
                                array[ base + i ].nrm = ftm.get_vertex_normal_3d( i ) ;
                                array[ base + i ].tx = ftm.get_vertex_texcoord( 0, i ) ;
                            }
                        }
                    } ) ;
                } );

                auto ib = motor::graphics::index_buffer_t().
                    set_layout_element( motor::graphics::type::tuint ).resize( ftm.indices.size() * max_objects ).
                    update<uint_t>( [&] ( uint_t* array, size_t const ne )
                {
                    motor::concurrent::parallel_for<size_t>( motor::concurrent::range_1d<size_t>(0, max_objects),
                        [&]( motor::concurrent::range_1d<size_t> const & r )
                    {
                        for( size_t o=r.begin(); o<r.end(); ++o )
                        {
                            size_t const vbase = o * ftm.get_num_vertices() ;
                            size_t const ibase = o * ftm.indices.size() ;
                            for( size_t i = 0; i < ftm.indices.size(); ++i ) 
                            {
                                array[ ibase + i ] = ftm.indices[ i ] + uint_t( vbase ) ;
                            }
                        }
                    } ) ;
                } ) ;

                geo_obj = motor::graphics::geometry_object_t( "cubes",
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

            // array
            {
                struct the_data
                {
                    motor::math::vec4f_t pos ;
                    motor::math::vec4f_t col ;
                };

                float_t scale = 20.0f ;
                motor::graphics::data_buffer_t db = motor::graphics::data_buffer_t()
                    .add_layout_element( motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                    .add_layout_element( motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 ) ;

                ar_obj = motor::graphics::array_object_t( "object_data", std::move( db ) ) ;
                    
            }

            {
                // quad object
                {
                    motor::graphics::msl_object_t mslo("scene_obj") ;

                    mslo.add( motor::graphics::msl_api_type::msl_4_0, R"(
                    config test_data_buffer
                    {
                        vertex_shader
                        {
                            uint_t vid : vertex_id ;
                            data_buffer_t u_data ;

                            mat4_t proj : projection ;
                            mat4_t view : view ;
                            mat4_t world : world ;

                            in vec3_t nrm : normal ;
                            in vec2_t tx : texcoord ; 
                            in vec3_t pos : position ;

                            out vec4_t pos : position ;
                            out vec2_t tx : texcoord ;
                            out vec3_t nrm : normal ;
                            out vec4_t col : color ;

                            void main()
                            {
                                int_t idx = vid / 24 ;
                                vec4_t pos_scl = fetch_data( u_data, (idx << 1)+0 ) ;
                                out.col = fetch_data( u_data, (idx<<1)+1 ) ;

                                vec4_t pos = vec4_t( in.pos ' as_vec3( pos_scl.w ), 1.0 ) ;
                                pos = world * pos + vec4_t( pos_scl.xyz ' as_vec3(pos_scl.w*2.0), 0.0 ) ;
                                out.pos = proj * view * pos ;

                                out.tx = in.tx ;
                                out.nrm = normalize( world * vec4_t(in.nrm, 0.0 ) ).xyz ;
                            }
                        }

                        pixel_shader
                        {
                            tex2d_t tex ;
                            vec4_t color ;

                            in vec2_t tx : texcoord ;
                            in vec3_t nrm : normal ;
                            in vec4_t col : color ;

                            out vec4_t color0 : color0 ;
                            out vec4_t color1 : color1 ;
                            out vec4_t color2 : color2 ;

                            void main()
                            {
                                out.color0 = in.col ' texture( tex, in.tx ) ' in.col ;
                                out.color1 = vec4_t( in.nrm, 1.0 ) ;
                                out.color2 = vec4_t( 
                                           as_vec3( dot( normalize( in.nrm ), normalize( vec3_t( 1.0, 1.0, 0.5) ) ) ), 
                                                        1.0 ) ;
                            }
                        }
                    })" ) ;

                        
                    mslo.link_geometry({"cubes"}) ;

                    msl_obj = std::move( mslo ) ;
                }
            
                // add variable set 
                {
                    motor::graphics::variable_set_t vars = motor::graphics::variable_set_t() ;
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
                        auto* var = vars.array_variable( "u_data" ) ;
                        var->set( "object_data" ) ;
                    }

                    msl_obj.add_variable_set( motor::shared( std::move( vars ) ) ) ;
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
        
                            tex2d_t u_tex ;

                            void main()
                            {
                                out.color = rt_texture( u_tex, in.tx ) ; 
                            }
                        }
                    } )" ) ;

                    mslo.link_geometry("quad") ;

                    fb_msl = std::move( mslo ) ;
                }

                // variable sets
                {
                    motor::graphics::variable_set_t vars ;
                        
                    {
                        auto * var = vars.texture_variable( "u_tex" ) ;
                        var->set( "the_scene.0" ) ;
                    }

                    fb_msl.add_variable_set( motor::memory::create_ptr( std::move(vars), "a variable set" ) ) ;
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
            float_t const dt = gd.sec_dt ;

            static float_t v = 0.0f ;
            v += dt * 0.1f;
            if( v > 1.0f ) v = 0.0f ;

            num_objects = std::min( max_objects, motor::math::interpolation<int_t>::linear( 1, max_objects-100, v ) ) ;
                
            // update array object data
            {
                static float_t  angle_ = 0.0f ;
                angle_ += ( ( ((dt))  ) * 2.0f * motor::math::constants<float_t>::pi() ) / 1.0f ;
                if( angle_ > 4.0f * motor::math::constants<float_t>::pi() ) angle_ = 0.0f ;

                float_t s = 5.0f * std::sin(angle_) ;

                struct the_data
                {
                    motor::math::vec4f_t pos ;
                    motor::math::vec4f_t col ;
                };
                
                ar_obj.data_buffer().resize( num_objects ).
                update< the_data >( [&]( the_data * array, size_t const ne )
                {
                    size_t const w = 20 ;
                    size_t const h = 50 ;
                    
                    #if 0 // serial for
                    for( size_t e=0; e<std::min( size_t(_num_objects_rnd), ne ); ++e )
                    {
                        size_t const x = e % w ;
                        size_t const y = (e / w) % h ;
                        size_t const z = (e / w) / h ;

                        motor::math::vec4f_t pos(
                            float_t(x) - float_t(w/2),
                            float_t(z) - float_t(w/2),//10.0f * std::sin( y + angle_ ),
                            float_t( y ) - float_t(w/2),
                            30.0f
                        ) ;

                        array[e].pos = pos ;

                        float_t c = float_t( rand() % 255 ) /255.0f ;
                        array[e].col = motor::math::vec4f_t ( 0.1f, c, (c) , 1.0f ) ;
                    }

                    #else // parallel for

                    typedef motor::concurrent::range_1d<size_t> range_t ;
                    auto const & range = range_t( 0, std::min( size_t(num_objects), ne ) ) ;

                    motor::concurrent::parallel_for<size_t>( range, [&]( range_t const & r )
                    {
                        for( size_t e=r.begin(); e<r.end(); ++e )
                        {
                            size_t const x = e % w ;
                            size_t const y = (e / w) % h ;
                            size_t const z = (e / w) / h ;

                            motor::math::vec4f_t pos(
                                float_t(x) - float_t(w/2),
                                float_t(z) - float_t(w/2),//10.0f * std::sin( y + angle_ ),
                                float_t( y ) - float_t(w/2),
                                30.0f
                            ) ;

                            array[e].pos = pos ;

                            float_t c = float_t( rand() % 255 ) /255.0f ;
                            array[e].col = motor::math::vec4f_t ( 0.1f, c, (c) , 1.0f ) ;
                        }
                    } ) ;

                    #endif

                } ) ;
            }

            // update render object variable sets
            {
                size_t i = 0 ;
                for( auto * vs : msl_obj.borrow_varibale_sets() )
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
                }
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
                    //fe->configure<motor::graphics::msl_object_t>( &msl_obj ) ;
                    fe->configure<motor::graphics::array_object_t>( &ar_obj ) ;
                    //fe->configure<motor::graphics::shader_object_t>( &sh_obj ) ;
                    fe->configure<motor::graphics::msl_object_t>( &msl_obj ) ;
                        
                }

                {
                    fe->configure<motor::graphics::geometry_object_t>( &fb_geo ) ;
                    fe->configure<motor::graphics::framebuffer_object_t>( &fb_obj ) ;
                    fe->configure<motor::graphics::msl_object_t>( &fb_msl ) ;
                }
            }

            // render
            {
                fe->configure<motor::graphics::array_object_t>( &ar_obj ) ;

                fe->use( &fb_obj ) ;
                fe->push( &scene_so ) ;

                {
                    motor::graphics::gen4::backend_t::render_detail_t detail ;
                    detail.start = 0 ;
                    detail.num_elems = num_objects * 36 ;
                    detail.varset = 0 ;
                    fe->render(  &msl_obj, detail ) ;
                }

                fe->pop( motor::graphics::gen4::backend::pop_type::render_state ) ; 
                fe->unuse( motor::graphics::gen4::backend::unuse_type::framebuffer ) ;

                fe->push( &fb_so ) ;
                {
                    motor::graphics::gen4::backend_t::render_detail_t detail ;
                    detail.varset = 0 ;
                    fe->render(  &fb_msl, detail ) ;
                }
                fe->pop( motor::graphics::gen4::backend::pop_type::render_state ) ;
                fe->fence( [=]( void_t ){} ) ;
            }
        }

        virtual bool_t on_tool( this_t::window_id_t const, motor::application::app::tool_data_ref_t ) noexcept { return true ; }
        virtual void_t on_shutdown( void_t ) noexcept {}
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