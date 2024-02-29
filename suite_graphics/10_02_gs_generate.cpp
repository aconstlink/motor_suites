
#include <motor/platform/global.h>
#include <motor/application/window/window_message_listener.h>

#include <motor/tool/imgui/custom_widgets.h>

#include <motor/graphics/object/render_object.h>
#include <motor/graphics/object/geometry_object.h>
#include <motor/graphics/object/msl_object.h>

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
        motor::graphics::msl_object_t msl_obj ;


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

                geo_obj = motor::graphics::geometry_object_t( "points",
                    motor::graphics::primitive_type::points, std::move( vb ), std::move( ib ) ) ;

            }

            // filter points shader
            {
                motor::graphics::msl_object_t mslo("filter_points") ;

                mslo.add( motor::graphics::msl_api_type::msl_4_0, R"(
                config render_original
                {
                    vertex_shader
                    {
                        inout vec4_t pos : position ;
                        inout vec4_t color : color ;

                        mat4_t u_proj : projection ;
                        mat4_t u_view : view ;
                        mat4_t u_world : world ;
                            
                        void main()
                        {
                            out.pos = vec4_t( in.pos.xyz, 1.0 ) ;
                            out.pos = out.pos ;
                            out.color = in.color ;
                        }
                    }

                    geometry_shader
                    {
                        in points ;
                        out triangles[ max_verts = 6 ] ;

                        inout vec4_t pos : position ;
                        inout vec4_t color : color ;

                        void main()
                        {
                            for( int i=0; i<in.length(); i++ )
                            {
                                {
                                    out.pos = vec4_t( in[i].pos.xy + vec2_t( -0.25, -0.25 ), 0.0, in[i].pos.w ) ;
                                    out.color = in[i].color ;
                                    emit_vertex() ;
                                }
                                {
                                    out.pos =  vec4_t( in[i].pos.xy + vec2_t( -0.25, 0.25 ), 0.0, in[i].pos.w ) ;
                                    out.color = in[i].color ;
                                    emit_vertex() ;
                                }
                                {
                                    out.pos =  vec4_t( in[i].pos.xy + vec2_t( +0.25, +0.25 ), 0.0, in[i].pos.w ) ;
                                    out.color = in[i].color ;
                                    emit_vertex() ;
                                }
                                end_primitive() ;

                                {
                                    out.pos =  vec4_t( in[i].pos.xy + vec2_t( -0.25, -0.25 ), 0.0, in[i].pos.w ) ;
                                    out.color = in[i].color ;
                                    emit_vertex() ;
                                }
                                {
                                    out.pos =  vec4_t( in[i].pos.xy + vec2_t( 0.25, 0.25 ), 0.0, in[i].pos.w ) ;
                                    out.color = in[i].color ;
                                    emit_vertex() ;
                                }
                                {
                                    out.pos =  vec4_t( in[i].pos.xy + vec2_t( 0.25, -0.25 ), 0.0, in[i].pos.w ) ;
                                    out.color = in[i].color ;
                                    emit_vertex() ;
                                }
                                end_primitive() ;
                            }
                        }
                    }

                    pixel_shader
                    {
                        inout vec4_t color : color ;

                        void main()
                        {
                            out.color = in.color ;
                        }
                    }
                })" ) ;

                        
                mslo.link_geometry( "points" ) ;

                // add variable set
                {
                    motor::graphics::variable_set_t vars ;
                    {}
                    mslo.add_variable_set( motor::shared( std::move( vars ) ) ) ;
                }

                msl_obj = std::move( mslo ) ;
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
                fe->configure<motor::graphics::msl_object_t>( &msl_obj ) ;
            }
            
            {
                motor::graphics::gen4::backend_t::render_detail_t detail ;
                detail.feed_from_streamout = false ;
                fe->render( &msl_obj, detail ) ;
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
    
    motor::concurrent::global::deinit() ;
    motor::log::global::deinit() ;
    motor::memory::global::dump_to_std() ;


    return ret ;
}