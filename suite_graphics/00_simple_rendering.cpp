#include <motor/platform/global.h>
#include <motor/application/window/window_message_listener.h>

#include <motor/graphics/frontend/gen4/frontend.hpp>
#include <motor/graphics/object/render_object.h>
#include <motor/graphics/object/geometry_object.h>

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

        motor::graphics::state_object_t root_so ;
        motor::graphics::render_object_t render_obj ;
        motor::graphics::shader_object_t shader_obj ;
        motor::graphics::geometry_object_t geo_obj ;
        motor::graphics::image_object_t img_obj ;

        struct vertex { motor::math::vec3f_t pos ; motor::math::vec2f_t tx ; } ;

        virtual void_t on_init( void_t ) noexcept
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

            this_t::send_window_message( this_t::create_window( wi ), [&]( motor::application::app::window_view & wnd )
            {
                wnd.send_message( motor::application::show_message( { true } ) ) ;
                wnd.send_message( motor::application::cursor_message_t( {false} ) ) ;
                wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
            } ) ;
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

        virtual void_t on_update( motor::application::app::update_data_in_t ) noexcept 
        {
            
        }

        virtual void_t on_graphics( motor::application::app::graphics_data_in_t ) noexcept 
        {
            if( graphics_init ) return ;
            graphics_init = true ;

            // vertex/index buffer
            {
                auto vb = motor::graphics::vertex_buffer_t()
                    .add_layout_element( motor::graphics::vertex_attribute::position, motor::graphics::type::tfloat, motor::graphics::type_struct::vec3 )
                    .add_layout_element( motor::graphics::vertex_attribute::texcoord0, motor::graphics::type::tfloat, motor::graphics::type_struct::vec2 )
                    .resize( 4 ).update<vertex>( [=] ( vertex* array, size_t const ne )
                {
                    array[ 0 ].pos = motor::math::vec3f_t( -0.5f, -0.5f, 0.0f ) ;
                    array[ 1 ].pos = motor::math::vec3f_t( -0.5f, +0.5f, 0.0f ) ;
                    array[ 2 ].pos = motor::math::vec3f_t( +0.5f, +0.5f, 0.0f ) ;
                    array[ 3 ].pos = motor::math::vec3f_t( +0.5f, -0.5f, 0.0f ) ;

                    array[ 0 ].tx = motor::math::vec2f_t( -0.0f, -0.0f ) ;
                    array[ 1 ].tx = motor::math::vec2f_t( -0.0f, +1.0f ) ;
                    array[ 2 ].tx = motor::math::vec2f_t( +1.0f, +1.0f ) ;
                    array[ 3 ].tx = motor::math::vec2f_t( +1.0f, -0.0f ) ;
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
                // shader configuration
                {
                    shader_obj = motor::graphics::shader_object_t( "quad" ) ;

                    // shaders : ogl 3.0
                    {
                        motor::graphics::shader_set_t ss = motor::graphics::shader_set_t().

                            set_vertex_shader( motor::graphics::shader_t( R"(
                                #version 140
                                in vec3 in_pos ;
                                in vec2 in_tx ;
                                out vec2 var_tx0 ;
                                //uniform mat4 u_proj ;
                                //uniform mat4 u_view ;
                            
                                void main()
                                {
                                    var_tx0 = in_tx ;
                                    gl_Position = vec4( in_pos.xyz, 1.0 ) ; //u_proj * u_view * vec4( in_pos, 1.0 ) ;
                                } )" ) ).

                            set_pixel_shader( motor::graphics::shader_t( R"(
                                #version 140
                                out vec4 out_color ;
                                in vec2 var_tx0 ;

                                uniform sampler2D u_tex ;
                                uniform vec4 u_color ;
                        
                                void main()
                                {    
                                    out_color = u_color * texture( u_tex, var_tx0 ) ;
                                } )" ) ) ;
                    
                        shader_obj.insert( motor::graphics::shader_api_type::glsl_1_4, std::move(ss) ) ;
                    }

                    // shaders : es 3.0
                    {
                        motor::graphics::shader_set_t ss = motor::graphics::shader_set_t().

                            set_vertex_shader( motor::graphics::shader_t( R"(
                                #version 300 es
                                in vec3 in_pos ;
                                in vec2 in_tx ;
                                out vec2 var_tx0 ;
                                uniform mat4 u_proj ;
                                uniform mat4 u_view ;

                                void main()
                                {
                                    var_tx0 = in_tx ;
                                    gl_Position = u_proj * u_view * vec4( in_pos, 1.0 ) ;
                                } )" ) ).
                        
                            set_pixel_shader( motor::graphics::shader_t( R"(
                                #version 300 es
                                precision mediump float ;
                                out vec4 out_color ;
                                in vec2 var_tx0 ;

                                uniform sampler2D u_tex ;
                                uniform vec4 u_color ;
                        
                                void main()
                                {    
                                    out_color = u_color * texture( u_tex, var_tx0 ) ;
                                } )" ) ) ;

                        shader_obj.insert( motor::graphics::shader_api_type::glsles_3_0, std::move(ss) ) ;
                    }

                    // shaders : hlsl 11(5.0)
                    {
                        motor::graphics::shader_set_t ss = motor::graphics::shader_set_t().

                            set_vertex_shader( motor::graphics::shader_t( R"(
                                cbuffer ConstantBuffer : register( b0 ) 
                                {
                                    matrix u_proj ;
                                    matrix u_view ;
                                }

                                struct VS_OUTPUT
                                {
                                    float4 pos : SV_POSITION;
                                    float2 tx : TEXCOORD0;
                                };

                                VS_OUTPUT VS( float4 in_pos : POSITION, float2 in_tx : TEXCOORD0 )
                                {
                                    VS_OUTPUT output = (VS_OUTPUT)0 ;
                                    //output.Pos = mul( Pos, World ) ;
                                    output.pos = mul( in_pos, u_view );
                                    output.pos = mul( output.pos, u_proj );
                                    output.pos = float4( in_pos.xyz, 1.0 ) ;
                                    output.tx = in_tx ;
                                    return output;
                                } )" ) ).

                            set_pixel_shader( motor::graphics::shader_t( R"(
                                // texture and sampler needs to be on the same slot.
                            
                                Texture2D u_tex : register( t0 );
                                SamplerState smp_u_tex : register( s0 );
                            
                                float4 u_color ;

                                struct VS_OUTPUT
                                {
                                    float4 Pos : SV_POSITION;
                                    float2 tx : TEXCOORD0;
                                };

                                float4 PS( VS_OUTPUT input ) : SV_Target
                                {
                                    return u_tex.Sample( smp_u_tex, input.tx ) * u_color ;
                                } )" ) ) ;

                        shader_obj.insert( motor::graphics::shader_api_type::hlsl_5_0, std::move( ss ) ) ;
                    }

                    // configure more details
                    {
                        shader_obj
                            .add_vertex_input_binding( motor::graphics::vertex_attribute::position, "in_pos" )
                            .add_vertex_input_binding( motor::graphics::vertex_attribute::texcoord0, "in_tx" )
                            .add_input_binding( motor::graphics::binding_point::view_matrix, "u_view" )
                            .add_input_binding( motor::graphics::binding_point::projection_matrix, "u_proj" ) ;
                    }
                }

                // render object
                {
                    render_obj = motor::graphics::render_object_t("render_quad") ;
                    render_obj.link_geometry( "quad" ) ;
                    render_obj.link_shader( "quad" ) ;
                    
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

                        render_obj.add_variable_set( motor::memory::create_ptr( std::move( vars ), "a variable set" ) ) ;
                    }

                    // variable sets
                    // add variable set 1
                    {
                        motor::graphics::variable_set_t vars ;
                        {
                            auto* var = vars.data_variable< motor::math::vec4f_t >( "u_color" ) ;
                            var->set( motor::math::vec4f_t( 0.0f, 1.0f, 0.0f, 1.0f ) ) ;
                        }

                        {
                            auto* var = vars.data_variable< float_t >( "u_time" ) ;
                            var->set( 0.0f ) ;
                        }

                        {
                            auto * var = vars.texture_variable( "u_tex" ) ;
                            var->set( "checker_board" ) ;
                        }

                        render_obj.add_variable_set( motor::memory::create_ptr( std::move(vars), "a variable set" ) ) ;
                    }
                }
            }
            // init render states object
            {
                motor::graphics::state_object_t so = motor::graphics::state_object_t(
                    "root_render_states" ) ;

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
                    rss.view_s.ss.do_activate = false ;
                    rss.view_s.ss.vp = motor::math::vec4ui_t( 0, 0, 500, 500 ) ;
                    so.add_render_state_set( rss ) ;
                }

                root_so = std::move( so ) ; 
            }
        }

        virtual void_t on_render( this_t::window_id_t const, motor::graphics::gen4::frontend_ptr_t fe, 
            motor::application::app::render_data_in_t rd ) noexcept 
        {
            // configure needs to be done only once per window
            {
                fe->configure<motor::graphics::state_object_t>( &root_so ) ;
                fe->configure<motor::graphics::geometry_object_t>( &geo_obj ) ;
                fe->configure<motor::graphics::image_object_t>( &img_obj ) ;
                fe->configure<motor::graphics::shader_object_t>( &shader_obj ) ;
                fe->configure<motor::graphics::render_object_t>( &render_obj ) ;
            }
            
            // render
            {
                fe->push( &root_so ) ;
                {
                    motor::graphics::gen4::backend_t::render_detail_t detail ;
                    detail.start = 0 ;
                    //detail.num_elems = 3 ;
                    detail.varset = 0 ;
                    fe->render( &render_obj, detail ) ;
                }
                fe->pop( motor::graphics::gen4::backend::pop_type::render_state ) ; ;
            }
        }

        virtual void_t on_shutdown( void_t ) noexcept 
        {
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