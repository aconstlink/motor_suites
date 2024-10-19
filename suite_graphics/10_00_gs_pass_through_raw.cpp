
#include <motor/platform/global.h>
#include <motor/application/window/window_message_listener.h>

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

        motor::graphics::geometry_object_t geo_obj ;
        motor::graphics::shader_object_t sh_obj ;
        motor::graphics::render_object_t rd_obj ;

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
                    wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                } ) ;
            }

            // geometry configuration for post quad
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

            // shader configuration
            {
                motor::graphics::shader_object_t sc( "render_original" ) ;

                // shaders : ogl 3.1
                {
                    motor::graphics::shader_set_t ss = motor::graphics::shader_set_t().

                        set_vertex_shader( motor::graphics::shader_t( R"(
                            #version 400 core
                            in vec4 in_pos ;
                            in vec4 in_color ;

                            uniform mat4 u_proj ;
                            uniform mat4 u_view ;
                            uniform mat4 u_world ;

                            out VS_GS_VERTEX
                            {
                                vec4 color ;
                            } vout ;

                            void main()
                            {
                                (gl_Position) = u_proj * u_view * u_world * in_pos ;
                                vout.color = in_color ;

                            } )" ) ).

                        set_geometry_shader( motor::graphics::shader_t( R"(
                            #version 400 core

                            layout( triangles ) in ;
                            layout( triangle_strip, max_vertices = 3 ) out ;

                            in VS_GS_VERTEX
                            {
                                vec4 color ;
                            } vin[] ;

                            out GS_FS_VERTEX
                            {
                                vec4 color ;
                            } vout ;

                            void main()
                            {
                                for( int i=0; i<gl_in.length(); ++i )
                                {
                                    vout.color = vin[i].color ;
                                    gl_Position = gl_in[i].gl_Position ;
                                    EmitVertex() ;
                                }
                                EndPrimitive() ;
                            } )" )).

                        set_pixel_shader( motor::graphics::shader_t( R"(
                            #version 400 core
    
                            in GS_FS_VERTEX
                            {
                                vec4 color ;
                            } fsin ;

                            layout( location = 0 ) out vec4 out_color ;

                            void main()
                            {
                                out_color = fsin.color ;
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

                            uniform mat4 u_proj ;
                            uniform mat4 u_view ;
                            uniform mat4 u_world ;

                            out VS_GS_VERTEX
                            {
                                vec4 color ;
                            } vout ;

                            void main()
                            {
                                (gl_Position) = u_proj * u_view * u_world * in_pos ;
                                vout.color = in_color ;

                            } )" ) ).

                        set_geometry_shader( motor::graphics::shader_t( R"(
                            #version 320 es

                            layout( triangles ) in ;
                            layout( triangle_strip, max_vertices = 3 ) out ;

                            in VS_GS_VERTEX
                            {
                                vec4 color ;
                            } vin[] ;

                            out GS_FS_VERTEX
                            {
                                vec4 color ;
                            } vout ;

                            void main()
                            {
                                for( int i=0; i<gl_in.length(); ++i )
                                {
                                    vout.color = vin[i].color ;
                                    gl_Position = gl_in[i].gl_Position ;
                                    EmitVertex() ;
                                }
                                EndPrimitive() ;
                            } )" )).

                        set_pixel_shader( motor::graphics::shader_t( R"(
                            #version 320 es

                            precision mediump int ;
                            precision mediump float ;
                            precision mediump sampler2DArray ;

                            in GS_FS_VERTEX
                            {
                                vec4 color ;
                            } fsin ;

                            layout( location = 0 ) out vec4 out_color ;

                            void main()
                            {
                                out_color = fsin.color ;
                            } )" ) ) ;

                    sc.insert( motor::graphics::shader_api_type::glsles_3_0, std::move( ss ) ) ;
                }

                // shaders : hlsl 11(5.0)
                {
                    motor::graphics::shader_set_t ss = motor::graphics::shader_set_t().

                        set_vertex_shader( motor::graphics::shader_t( R"(
                            cbuffer ConstantBuffer : register( b0 ) 
                            {
                                float4x4 u_proj ;
                                float4x4 u_view ;
                                float4x4 u_world ;
                            }

                            struct VS_INPUT
                            {
                                float4 in_pos : POSITION ; 
                                float4 in_color : COLOR ;
                            } ;

                            struct VSGS_DATA
                            {
                                float4 pos : SV_POSITION ;
                                float4 col : COLOR ;
                            };

                            VSGS_DATA VS( VS_INPUT input )
                            {
                                VSGS_DATA output = (VSGS_DATA)0 ;

                                output.pos = mul( input.in_pos, u_world ) ;
                                output.pos = mul( output.pos, u_view ) ;
                                output.pos = mul( output.pos, u_proj ) ;
                                output.col = input.in_color ;
                                return output;
                            } )" ) ).
                        
                        set_geometry_shader( motor::graphics::shader_t( R"(
                            
                            cbuffer ConstantBuffer : register( b0 ) 
                            {}

                            struct VSGS_DATA
                            {
                                float4 pos : SV_POSITION ;
                                float4 col : COLOR ;
                            };
                            
                            struct GSPS_DATA
                            {
                                float4 pos : SV_POSITION;
                                float4 col : COLOR ;
                            } ;

                            [maxvertexcount(3)]
                            void GS( triangle VSGS_DATA input[3], inout TriangleStream<GSPS_DATA> tri_stream ) 
                            {
                                GSPS_DATA output ;
                                for( int i=0; i<3; ++i )
                                {
                                    output.pos = input[i].pos ;
                                    output.col = input[i].col ;
                                    tri_stream.Append( output ) ;    
                                }
                                tri_stream.RestartStrip() ;
                            } )" ) ) .

                        set_pixel_shader( motor::graphics::shader_t( R"(
                            
                            cbuffer ConstantBuffer : register( b0 ) 
                            {}

                            struct GSPS_DATA
                            {
                                float4 pos : SV_POSITION;
                                float4 col : COLOR ;
                            } ;

                            float4 PS( GSPS_DATA input ) : SV_Target0
                            {
                                return input.col ;
                            } )" ) ) ;

                    sc.insert( motor::graphics::shader_api_type::hlsl_5_0, std::move( ss ) ) ;
                }

                // configure more details
                {
                    sc.shader_bindings()
                        .add_vertex_input_binding( motor::graphics::vertex_attribute::position, "in_pos" )
                        .add_vertex_input_binding( motor::graphics::vertex_attribute::color0, "in_color" )
                        .add_input_binding( motor::graphics::binding_point::world_matrix, "u_world" )
                        .add_input_binding( motor::graphics::binding_point::view_matrix, "u_view" )
                        .add_input_binding( motor::graphics::binding_point::projection_matrix, "u_proj" ) ;
                }

                sh_obj = std::move( sc ) ;
            }

            // the quad render object
            {
                motor::graphics::render_object_t rc = motor::graphics::render_object_t( "quad" ) ;

                {
                    rc.link_geometry( "quad" ) ;
                    rc.link_shader( "render_original" ) ;
                }

                // add variable set 0
                {
                    motor::graphics::variable_set_t vars ;
                    {}
                    rc.add_variable_set( motor::shared( std::move( vars ) ) ) ;
                }

                rd_obj = std::move( rc ) ;
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

        virtual void_t on_render( this_t::window_id_t const wid, motor::graphics::gen4::frontend_ptr_t fe,
            motor::application::app::render_data_in_t rd ) noexcept 
        {            
            if( rd.first_frame )
            {
                fe->configure<motor::graphics::geometry_object_t>( &geo_obj ) ;
                fe->configure<motor::graphics::shader_object_t>( &sh_obj ) ;
                fe->configure<motor::graphics::render_object_t>( &rd_obj ) ;
            }

            {
                motor::graphics::gen4::backend_t::render_detail_t detail ;
                fe->render( &rd_obj, detail ) ;
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