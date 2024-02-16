#include <motor/platform/global.h>
#include <motor/application/window/window_message_listener.h>

#include <motor/graphics/frontend/gen4/frontend.hpp>

#include <motor/graphics/object/render_object.h>
#include <motor/graphics/object/geometry_object.h>

#include <motor/log/global.h>
#include <motor/memory/global.h>

#include <future>

int main( int argc, char ** argv )
{
    using namespace motor::core::types ;

    motor::application::carrier_mtr_t carrier = motor::platform::global_t::create_carrier() ;

    auto fut_update_loop = std::async( std::launch::async, [&]( void_t )
    {
        motor::application::window_message_listener_mtr_t msgl_out1 = motor::memory::create_ptr<
            motor::application::window_message_listener>( "[out] : message listener 1" ) ;

        motor::application::window_message_listener_mtr_t msgl_out2 = motor::memory::create_ptr<
            motor::application::window_message_listener>( "[out] : message listener 2" ) ;

        {
            motor::application::window_info_t wi ;
            wi.x = 100 ;
            wi.y = 100 ;
            wi.w = 800 ;
            wi.h = 600 ;
            wi.gen = motor::application::graphics_generation::gen4_auto ;

            auto wnd1 = carrier->create_window( wi ) ;

            wi.x = 200 ;
            wi.y = 100 ;
            wi.w = 800 ;
            wi.h = 600 ;
            wi.gen = motor::application::graphics_generation::gen4_gl4 ;

            auto wnd2 = carrier->create_window( wi ) ;

            wnd1->register_out( motor::share( msgl_out1 ) ) ;
            wnd2->register_out( motor::share( msgl_out2 ) ) ;

            wnd1->send_message( motor::application::show_message( { true } ) ) ;
            wnd1->send_message( motor::application::cursor_message_t( {false} ) ) ;
            wnd1->send_message( motor::application::vsync_message_t( { true } ) ) ;

            wnd2->send_message( motor::application::show_message( { true } ) ) ;

            // wait for window creation
            {
                size_t count = 0 ;
                while( count < 2 ) 
                {
                    {
                        motor::application::window_message_listener_t::state_vector_t sv ;
                        if( msgl_out1->swap_and_reset( sv ) )
                        {
                            if( sv.create_changed )
                            {
                                ++count ;
                            }
                        }
                    }

                    {
                        motor::application::window_message_listener_t::state_vector_t sv ;
                        if( msgl_out2->swap_and_reset( sv ) )
                        {
                            if( sv.create_changed )
                            {
                                ++count ;
                            }
                        }
                    }
                }
            }

            {
                motor::math::vec4ui_t fb_dims(0, 0, 800, 800) ;

                motor::graphics::state_object_t scene_so("scene_render_states") ;
                motor::graphics::geometry_object_t geo_obj ;
                motor::graphics::image_object_t img_obj ;
                motor::graphics::msl_object_t msl_obj ;

                motor::graphics::state_object_t fb_so("framebuffer_render_states") ;
                motor::graphics::framebuffer_object_t fb_obj( "the_scene" ) ;

                {
                    motor::graphics::render_state_sets_t rss ;
                    rss.depth_s.do_change = true ;
                    rss.depth_s.ss.do_activate = false ;
                    rss.depth_s.ss.do_depth_write = true ;
                    rss.polygon_s.do_change = true ;
                    rss.polygon_s.ss.do_activate = true ;
                    rss.polygon_s.ss.ff = motor::graphics::front_face::clock_wise ;
                    rss.polygon_s.ss.cm = motor::graphics::cull_mode::back ;
                    rss.clear_s.do_change = true ;
                    rss.clear_s.ss.clear_color = motor::math::vec4f_t(0.5f, 0.9f, 0.5f, 1.0f ) ;
                    rss.clear_s.ss.do_activate = true ;
                    rss.clear_s.ss.do_color_clear = true ;
                    rss.clear_s.ss.do_depth_clear = true ;
                    rss.view_s.do_change = true ;
                    rss.view_s.ss.do_activate = true ;
                    rss.view_s.ss.vp = fb_dims ;

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

                    fb_so.add_render_state_set( rss ) ;
                }

                struct vertex { motor::math::vec2f_t pos ; motor::math::vec2f_t tx ; motor::math::vec4f_t color ; } ;
                
                // vertex/index buffer
                {
                    auto vb = motor::graphics::vertex_buffer_t()
                        .add_layout_element( motor::graphics::vertex_attribute::position, motor::graphics::type::tfloat, motor::graphics::type_struct::vec2 )
                        .add_layout_element( motor::graphics::vertex_attribute::texcoord0, motor::graphics::type::tfloat, motor::graphics::type_struct::vec2 )
                        .add_layout_element( motor::graphics::vertex_attribute::color0, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                        .resize( 4 ).update<vertex>( [=] ( vertex* array, size_t const ne )
                    {
                        array[ 0 ].pos = motor::math::vec2f_t( -0.5f, -0.5f ) ;
                        array[ 1 ].pos = motor::math::vec2f_t( -0.5f, +0.5f ) ;
                        array[ 2 ].pos = motor::math::vec2f_t( +0.5f, +0.5f ) ;
                        array[ 3 ].pos = motor::math::vec2f_t( +0.5f, -0.5f ) ;

                        array[ 0 ].tx = motor::math::vec2f_t( -0.0f, -0.0f ) ;
                        array[ 1 ].tx = motor::math::vec2f_t( -0.0f, +1.0f ) ;
                        array[ 2 ].tx = motor::math::vec2f_t( +1.0f, +1.0f ) ;
                        array[ 3 ].tx = motor::math::vec2f_t( +1.0f, -0.0f ) ;

                        array[ 0 ].color = motor::math::vec4f_t( 1.0f, 0.0f, 1.0f, 1.0f ) ;
                        array[ 1 ].color = motor::math::vec4f_t( 1.0f, 0.0f, 1.0f, 1.0f ) ;
                        array[ 2 ].color = motor::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ) ;
                        array[ 3 ].color = motor::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ) ;
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

                    geo_obj = motor::graphics::geometry_object_t( "quad",
                        motor::graphics::primitive_type::triangles, std::move( vb ), std::move( ib ) ) ;
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

                // shaders
                {
                    // msl object
                    {
                        motor::graphics::msl_object_t mslo("msl_quad") ;

                        mslo.add( motor::graphics::msl_api_type::msl_4_0, R"(
                        config msl_quad
                        {
                            vertex_shader
                            {
                                mat4_t proj : projection ;
                                mat4_t view : view ;

                                in vec3_t pos : position ;
                                in vec2_t tx : texcoord0 ;

                                out vec2_t tx : texcoord0 ;
                                out vec4_t pos : position ;

                                void main()
                                {
                                    //vec3_t p = msl.lib_a.myfunk( vec3_t( 1.0, 1.0, 1.0 ), in.pos, vec3_t( 0.5 ) ) ;
                                    out.tx = in.tx ;
                                    out.pos = vec4_t( in.pos, 1.0 ) ; 
                                }
                            }

                            pixel_shader
                            {
                                tex2d_t u_tex ;

                                in vec2_t tx : texcoord0 ;
                                out vec4_t color : color ;

                                vec4_t u_color ;

                                void main()
                                {
                                    out.color = texture( u_tex, in.tx ) ' u_color  ;
                                }
                            }
                        } )" ) ;

                        mslo.link_geometry("quad") ;

                        msl_obj = std::move( mslo ) ;
                    }

                    // variable sets
                    // add variable set 0
                    {
                        motor::graphics::variable_set_t vars ;
                        {
                            auto* var = vars.data_variable< motor::math::vec4f_t >( "u_color" ) ;
                            var->set( motor::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ) ) ;
                        }

                        {
                            auto* var = vars.data_variable< float_t >( "u_time" ) ;
                            var->set( 0.0f ) ;
                        }

                        {
                            auto * var = vars.texture_variable( "u_tex" ) ;
                            var->set( "checker_board" ) ;
                        }

                        msl_obj.add_variable_set( motor::memory::create_ptr( std::move( vars ), "a variable set" ) ) ;
                    }

                    // variable sets
                    // add variable set 1
                    {
                        motor::graphics::variable_set_t vars ;
                        {
                            auto* var = vars.data_variable< motor::math::vec4f_t >( "u_color" ) ;
                            var->set( motor::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ) ) ;
                        }

                        {
                            auto* var = vars.data_variable< float_t >( "u_time" ) ;
                            var->set( 0.0f ) ;
                        }

                        {
                            auto * var = vars.texture_variable( "u_tex" ) ;
                            var->set( "the_scene.0" ) ;
                        }

                        msl_obj.add_variable_set( motor::memory::create_ptr( std::move(vars), "a variable set" ) ) ;
                    }
                }

                // framebuffer
                {
                    fb_obj.set_target( motor::graphics::color_target_type::rgba_uint_8, 1 )
                        .resize( fb_dims.z(), fb_dims.w() ) ;
                }

                auto my_rnd_funk_init = [&]( motor::graphics::gen4::frontend_ptr_t fe )
                {
                    // init render states object
                    {
                        

                        fe->configure<motor::graphics::state_object_t>( &scene_so ) ;
                        fe->configure<motor::graphics::state_object_t>( &fb_so ) ;
                    }

                    // 
                    {
                        fe->configure<motor::graphics::geometry_object_t>( &geo_obj ) ;
                        fe->configure<motor::graphics::image_object_t>( &img_obj ) ;
                        fe->configure<motor::graphics::framebuffer_object_t>( &fb_obj ) ;
                        fe->configure<motor::graphics::msl_object_t>( &msl_obj ) ;
                    }
                } ;

                if( wnd1->render_frame< motor::graphics::gen4::frontend_t >( my_rnd_funk_init ) )
                {
                    // funk has been rendered.
                }

                if( wnd2->render_frame< motor::graphics::gen4::frontend_t >( my_rnd_funk_init ) )
                {
                    // funk has been rendered.
                }

                while( true ) 
                {
                    std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) ) ;
                
                    {
                        motor::application::window_message_listener_t::state_vector_t sv ;
                        if( msgl_out1->swap_and_reset( sv ) )
                        {
                            if( sv.close_changed )
                            {
                                break ;
                            }
                        }
                    }

                    {
                        motor::application::window_message_listener_t::state_vector_t sv ;
                        if( msgl_out2->swap_and_reset( sv ) )
                        {
                            if( sv.close_changed )
                            {
                                break ;
                            }
                        }
                    }
                    
                    auto my_rnd_funk = [&]( motor::graphics::gen4::frontend_ptr_t fe )
                    {
                        fe->use( &fb_obj ) ;
                        fe->push( &scene_so ) ;
                        {
                            motor::graphics::gen4::backend_t::render_detail_t detail ;
                            detail.start = 0 ;
                            //detail.num_elems = 3 ;
                            detail.varset = 0 ;
                            fe->render(  &msl_obj, detail ) ;
                        }
                        fe->pop( motor::graphics::gen4::backend::pop_type::render_state ) ; 
                        fe->unuse( motor::graphics::gen4::backend::unuse_type::framebuffer ) ;

                        fe->push( &fb_so ) ;
                        {
                            motor::graphics::gen4::backend_t::render_detail_t detail ;
                            detail.varset = 1 ;
                            fe->render(  &msl_obj, detail ) ;
                        }
                        fe->pop( motor::graphics::gen4::backend::pop_type::render_state ) ;
                        fe->fence( [=]( void_t )
                        {
                            //motor::log::global_t::status("fence hit") ;
                        } ) ;
                    } ;


                    if( wnd1->render_frame< motor::graphics::gen4::frontend_t >( my_rnd_funk ) )
                    {
                        // funk has been rendered.
                    }

                    if( wnd2->render_frame< motor::graphics::gen4::frontend_t >( my_rnd_funk ) )
                    {
                        // funk has been rendered.
                    }
                }
            }

            motor::memory::release_ptr( wnd1 ) ;
            motor::memory::release_ptr( wnd2 ) ;
        }

        motor::memory::release_ptr( msgl_out1 ) ;
        motor::memory::release_ptr( msgl_out2 ) ;
    }) ;

    // end the program by closing the carrier
    auto fut_end = std::async( std::launch::async, [&]( void_t )
    {
        fut_update_loop.wait() ;

        carrier->close() ;
    } ) ;

    auto const ret = carrier->exec() ;
    
    motor::memory::release_ptr( carrier ) ;

    motor::log::global::deinit() ;
    motor::memory::global::dump_to_std() ;


    return ret ;
}