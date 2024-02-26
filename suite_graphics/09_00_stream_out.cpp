
#include <motor/platform/global.h>
#include <motor/application/window/window_message_listener.h>

#include <motor/graphics/object/render_object.h>
#include <motor/graphics/object/geometry_object.h>
#include <motor/graphics/object/streamout_object.h>

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

        bool_t graphics_init = false ;
        motor::vector< bool_t > rnd_init ;

        motor::math::vec4ui_t fb_dims = motor::math::vec4ui_t( 0, 0, 1920, 1080 ) ;

        motor::graphics::state_object_t scene_so ;
        motor::graphics::geometry_object_t geo_obj ;

        motor::graphics::shader_object_t sh_obj ;
        motor::graphics::shader_object_t sh_so_obj ;

        // capture and bind streamed out geometry
        motor::graphics::streamout_object_t so_obj ;

        // render object : doing stream out
        motor::graphics::render_object_t ro_so_obj ;

        // render object : rendering original geometry
        motor::graphics::render_object_t ro_orig_obj ;


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
                    wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                } ) ;

                rnd_init.emplace_back( false ) ;
            }

            {
                motor::application::window_info_t wi ;
                wi.x = 900 ;
                wi.y = 100 ;
                wi.w = 800 ;
                wi.h = 600 ;
                wi.gen = motor::application::graphics_generation::gen4_gl4 ;
                this_t::send_window_message( this_t::create_window( wi ), [&]( motor::application::app::window_view & wnd )
                {
                    wnd.send_message( motor::application::show_message( { true } ) ) ;
                    wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                } ) ;
                rnd_init.emplace_back( false ) ;
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
                fb_dims = motor::math::vec4ui_t( uint_t(sv.resize_msg.x), uint_t(sv.resize_msg.y), 
                    uint_t(sv.resize_msg.w), uint_t(sv.resize_msg.h) ) ;
            }
        }

        virtual void_t on_graphics( motor::application::app::graphics_data_in_t gd ) noexcept 
        {
            if( !graphics_init ) 
            {
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

                // geometry configuration
                {
                    struct vertex { motor::math::vec4f_t pos ; motor::math::vec4f_t color ; } ;

                    auto vb = motor::graphics::vertex_buffer_t()
                        .add_layout_element( motor::graphics::vertex_attribute::position, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                        .add_layout_element( motor::graphics::vertex_attribute::color0, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                        .resize( 4 ).update<vertex>( [=] ( vertex* array, size_t const ne )
                    {
                        array[ 0 ].pos = motor::math::vec4f_t( -0.5f, -0.5f, 0.0f, 1.0f ) ;
                        array[ 1 ].pos = motor::math::vec4f_t( -0.5f, +0.5f, 0.0f, 1.0f ) ;
                        array[ 2 ].pos = motor::math::vec4f_t( +0.5f, +0.5f, 0.0f, 1.0f ) ;
                        array[ 3 ].pos = motor::math::vec4f_t( +0.5f, -0.5f, 0.0f, 1.0f ) ;

                        array[ 0 ].color = motor::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ) ;
                        array[ 1 ].color = motor::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ) ;
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

                // stream out object configuration
                {
                    auto vb = motor::graphics::vertex_buffer_t()
                        .add_layout_element( motor::graphics::vertex_attribute::position, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                        .add_layout_element( motor::graphics::vertex_attribute::color0, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 ) ;


                    so_obj = motor::graphics::streamout_object_t( "compute", std::move( vb ) ).resize( 1000 ) ;
                }

                // shader configuration
                {
                    motor::graphics::shader_object_t sc( "render_original" ) ;

                    // shaders : ogl 3.1
                    {
                        motor::graphics::shader_set_t ss = motor::graphics::shader_set_t().

                            set_vertex_shader( motor::graphics::shader_t( R"(
                                #version 140
                                in vec4 in_pos ;
                                in vec4 in_color ;
                                out vec4 var_color ;

                                void main()
                                {
                                    var_color = in_color ;
                                    gl_Position = in_pos ;

                                } )" ) ).

                            set_pixel_shader( motor::graphics::shader_t( R"(
                                #version 140
                                #extension GL_ARB_separate_shader_objects : enable
                                #extension GL_ARB_explicit_attrib_location : enable

                                in vec4 var_color ;
                                layout( location = 0 ) out vec4 out_color ;

                                void main()
                                {
                                    out_color = var_color ;
                                } )" ) ) ;

                        sc.insert( motor::graphics::shader_api_type::glsl_4_0, std::move( ss ) ) ;
                    }

                    // shaders : es 3.0
                    {
                        motor::graphics::shader_set_t ss = motor::graphics::shader_set_t().

                            set_vertex_shader( motor::graphics::shader_t( R"(
                                #version 320 es
                                in vec4 in_pos ;
                                in vec4 in_color ;
                                out vec4 var_color ;

                                void main()
                                {
                                    var_color = in_color ;
                                    gl_Position = vec4( in_pos  ) ;

                                } )" ) ).

                            set_pixel_shader( motor::graphics::shader_t( R"(
                                #version 320 es
                                precision mediump int ;
                                precision mediump float ;
                                precision mediump sampler2DArray ;

                                in vec4 var_color ;
                                out vec4 out_color ;

                                void main()
                                {
                                    out_color = var_color ;
                                } )" ) ) ;

                        sc.insert( motor::graphics::shader_api_type::glsles_3_0, std::move( ss ) ) ;
                    }

                    // shaders : hlsl 11(5.0)
                    {
                        motor::graphics::shader_set_t ss = motor::graphics::shader_set_t().

                        set_vertex_shader( motor::graphics::shader_t( R"(
                            cbuffer ConstantBuffer : register( b0 ) 
                            {
                            }

                            struct VS_INPUT
                            {
                                float4 in_pos : POSITION ; 
                                float4 in_color : COLOR ;
                            } ;

                            struct VS_OUTPUT
                            {
                                float4 pos : SV_POSITION ;
                                float4 col : COLOR ;
                            };

                            VS_OUTPUT VS( VS_INPUT input )
                            {
                                VS_OUTPUT output = (VS_OUTPUT)0 ;

                                output.pos = input.in_pos ;
                                output.col = input.in_color ;
                                return output;
                            } )" ) ).

                        set_pixel_shader( motor::graphics::shader_t( R"(

                            cbuffer ConstantBuffer : register( b0 ) 
                            {}

                            struct VS_OUTPUT
                            {
                                float4 pos : SV_POSITION;
                                float4 col : COLOR ;
                            } ;

                            float4 PS( VS_OUTPUT input ) : SV_Target0
                            {
                                return input.col ;
                            } )" ) ) ;

                        sc.insert( motor::graphics::shader_api_type::hlsl_5_0, std::move( ss ) ) ;
                    }

                    // configure more details
                    {
                        sc
                            .add_vertex_input_binding( motor::graphics::vertex_attribute::position, "in_pos" )
                            .add_vertex_input_binding( motor::graphics::vertex_attribute::color0, "in_color" )
                            .add_input_binding( motor::graphics::binding_point::world_matrix, "u_world" )
                            .add_input_binding( motor::graphics::binding_point::view_matrix, "u_view" )
                            .add_input_binding( motor::graphics::binding_point::projection_matrix, "u_proj" ) ;
                    }

                    sh_obj = std::move( sc ) ;
                }

                // shader configuration
                {
                    motor::graphics::shader_object_t sc( "stream_out" ) ;

                    // shaders : ogl 3.1
                    {
                        motor::graphics::shader_set_t ss = motor::graphics::shader_set_t().

                            set_vertex_shader( motor::graphics::shader_t( R"(
                                #version 140
                                in vec4 in_pos ;
                                in vec4 in_color ;
                                out vec4 out_pos ;
                                out vec4 out_col ;

                                uniform float u_ani ;
                                void main()
                                {
                                    float t = u_ani * 3.14526 * 2.0 ;
                                    out_pos = in_pos + vec4( 0.02 *cos(t), 0.02 *sin(t), 0.0, 0.0 ) ;
                                    out_col = vec4( 1.0, 1.0, 0.0, 1.0 ) ;
                                } )" ) ) ;

                        sc.insert( motor::graphics::shader_api_type::glsl_4_0, std::move( ss ) ) ;
                    }

                    // shaders : es 3.0
                    {
                        motor::graphics::shader_set_t ss = motor::graphics::shader_set_t().

                            set_vertex_shader( motor::graphics::shader_t( R"(
                                #version 300 es
                                precision mediump int ;
                                in vec4 in_pos ;
                                in vec4 in_color ;
                                out vec4 out_pos ;
                                out vec4 out_col ;

                                uniform float u_ani ;  
                                void main()
                                {
                                    float t = u_ani * 3.14526 * 2.0 ;
                                    out_pos = in_pos + vec4( 0.02 *cos(t), 0.02 *sin(t), 0.0, 0.0 ) ;
                                    out_col = vec4( 1.0, 1.0, 0.0, 1.0 ) ;
                                } )" ) ) ;

                        sc.insert( motor::graphics::shader_api_type::glsles_3_0, std::move( ss ) ) ;
                    }

                    // shaders : hlsl 11(5.0)
                    {
                        motor::graphics::shader_set_t ss = motor::graphics::shader_set_t().

                            set_vertex_shader( motor::graphics::shader_t( R"(
                                cbuffer ConstantBuffer : register( b0 ) 
                                {
                                    float u_ani ;
                                }

                                struct VS_INPUT
                                {
                                    float4 in_pos : POSITION ; 
                                    float4 in_color : COLOR ;
                                } ;

                                struct VS_OUTPUT
                                {
                                    float4 out_pos : SV_POSITION ;
                                    float4 out_col : COLOR ;
                                };

                                VS_OUTPUT VS( VS_INPUT input )
                                {
                                    VS_OUTPUT output = (VS_OUTPUT)0 ;
                                    float t = u_ani * 3.14526 * 2.0 ;
                                    output.out_pos = input.in_pos + float4( 0.02 *cos(t), 0.02 *sin(t), 0.0, 0.0 ) ;
                                    output.out_col = float4( 1.0, 1.0, 0.0, 1.0 ) ;
                                    return output;
                                } )" ) ) ;

                        sc.insert( motor::graphics::shader_api_type::hlsl_5_0, std::move( ss ) ) ;
                    }

                    // configure more details
                    {
                        sc
                            .add_vertex_input_binding( motor::graphics::vertex_attribute::position, "in_pos" )
                            .add_vertex_input_binding( motor::graphics::vertex_attribute::color0, "in_color" )
                            .add_vertex_output_binding( motor::graphics::vertex_attribute::position, "out_pos" )
                            .add_vertex_output_binding( motor::graphics::vertex_attribute::color0, "out_col" )
                            .set_streamout_mode( motor::graphics::streamout_mode::interleaved ) ;
                    }

                    sh_so_obj = std::move( sc ) ;
                }
            
                // the original geometry render object
                {
                    motor::graphics::render_object_t rc = motor::graphics::render_object_t( "quad" ) ;

                    {
                        rc.link_geometry( "quad", "compute" ) ;
                        rc.link_shader( "render_original" ) ;
                    }

                    // add variable set 0
                    {
                        motor::graphics::variable_set_t vars ;
                        {}
                        rc.add_variable_set( motor::shared( std::move( vars ) ) ) ;
                    }

                    ro_orig_obj = std::move( rc ) ;
                }

                // the stream out render object
                {
                    motor::graphics::render_object_t rc = motor::graphics::render_object_t( "stream_out" ) ;

                    {
                        rc.link_geometry( "quad", "compute" ) ;
                        rc.link_shader( "stream_out" ) ;
                    }

                    // add variable set
                    {
                        motor::graphics::variable_set_t vars ;
                        {}
                        rc.add_variable_set( motor::shared( std::move( vars ) ) ) ;
                    }
                    ro_so_obj = std::move( rc ) ;
                }
            }

            graphics_init = true ;
        }

        virtual void_t on_render( this_t::window_id_t const wid, motor::graphics::gen4::frontend_ptr_t fe,
            motor::application::app::render_data_in_t rd ) noexcept 
        {            
            if( !rnd_init[wid] )
            {
                rnd_init[wid] = true ;

                fe->configure<motor::graphics::state_object_t>( &scene_so ) ;
                fe->configure<motor::graphics::geometry_object_t>( &geo_obj ) ;
                fe->configure<motor::graphics::streamout_object_t>( &so_obj ) ;
                fe->configure<motor::graphics::shader_object_t>( &sh_obj ) ;
                fe->configure<motor::graphics::shader_object_t>( &sh_so_obj ) ;
                fe->configure<motor::graphics::render_object_t>( &ro_so_obj ) ;
                fe->configure<motor::graphics::render_object_t>( &ro_orig_obj ) ;

                // do initial streamout pass 
                // in order to fill the buffer
                {
                    fe->use( &so_obj ) ;
                    fe->render( &ro_so_obj, motor::graphics::gen4::backend::render_detail_t() ) ;
                    fe->unuse( motor::graphics::gen4::backend::unuse_type::streamout ) ;
                }
            }
            
            // do stream out
            {
                fe->use( &so_obj ) ;
                {
                    motor::graphics::gen4::backend_t::render_detail_t detail ;
                    detail.feed_from_streamout = true ;
                    fe->render( &ro_so_obj, detail ) ;
                }
                fe->unuse( motor::graphics::gen4::backend::unuse_type::streamout ) ;
            }

            // matrix variables here

            static float_t max_time = 2.0f ;
            static float_t time = 0.0f ;
            time += rd.sec_dt ;
            if( time > max_time ) time = 0.0f ;
            ro_so_obj.for_each( [&] ( size_t const i, motor::graphics::variable_set_mtr_t vs )
            {
                {
                    auto * var = vs->data_variable<motor::math::float_t>("u_ani") ;
                    var->set( time / max_time ) ;
                }
            } ) ;

            #if 1
            // render original from geometry
            {
                motor::graphics::gen4::backend_t::render_detail_t detail ;
                detail.feed_from_streamout = false ;
                fe->render( &ro_orig_obj, detail ) ;
            }
            #endif
            // render streamed out
            {
                motor::graphics::gen4::backend_t::render_detail_t detail ;
                detail.feed_from_streamout = true ;
                fe->render( &ro_orig_obj, detail ) ;
            }
        }

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