
#include <motor/platform/global.h>
#include <motor/application/window/window_message_listener.h>

#include <motor/tool/imgui/custom_widgets.h>

#include <motor/graphics/object/render_object.h>
#include <motor/graphics/object/geometry_object.h>
#include <motor/graphics/object/msl_object.h>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>
#include <motor/profiling/global.h>

#include <future>

namespace this_file
{
    using namespace motor::core::types ;

    class my_app : public motor::application::app
    {
        motor_this_typedefs( my_app ) ;

        motor::graphics::streamout_object_t so_obj ;
        motor::graphics::geometry_object_t geo_pts_obj ;
        motor::graphics::msl_object_t msl_filter_obj ;
        motor::graphics::msl_object_t msl_obj ;

        motor::math::vec3f_t dir = motor::math::vec3f_t( -1.0f, 0.0f, 0.0f ) ;

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
                    wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                } ) ;
            }

            // geometry configuration : points
            {
                size_t const w = 100 ;
                size_t const h = 100 ;

                float_t const sx = 1.0f/w ; //1.0f ;// float_t( w ) ;
                float_t const sy = 1.0f/h ; // 1.0f ;// float_t( h ) ;

                motor::math::vec2f_t const start = motor::math::vec2f_t( -float_t( 1.0f ), -float_t( 1.0f ) ) *0.5f;

                struct vertex { motor::math::vec4f_t pos ; motor::math::vec4f_t color ; } ;

                auto vb = motor::graphics::vertex_buffer_t()
                    .add_layout_element( motor::graphics::vertex_attribute::position, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                    .add_layout_element( motor::graphics::vertex_attribute::color0, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                    .resize( w * h ).update<vertex>( [=] ( vertex* array, size_t const ne )
                {
                    for( size_t yi =0; yi<h; ++yi )
                    {
                        for( size_t xi =0; xi<w; ++xi )
                        {
                            size_t const idx = yi * w + xi ;

                            motor::math::vec2f_t const p = start + motor::math::vec2f_t( sx, sy ) * 
                                motor::math::vec2f_t( float_t(xi), float_t(yi) ) ;

                            array[ idx ].pos = motor::math::vec4f_t( p, 0.0f, sx*sy*0.5f ) ;
                            array[ idx ].color = motor::math::vec4f_t( -1.0f, 1.0f, 1.0f, 1.0f ) ;
                        }
                    }
                    
                } );

                geo_pts_obj = motor::graphics::geometry_object_t( "points",
                    motor::graphics::primitive_type::points, std::move( vb ) ) ;

                // stream out object configuration
                {
                    auto vb2 = motor::graphics::vertex_buffer_t()
                        .add_layout_element( motor::graphics::vertex_attribute::position, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                        .add_layout_element( motor::graphics::vertex_attribute::color0, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 ) ;

                    so_obj = motor::graphics::streamout_object_t( "filtered_points", std::move( vb2 ) ).resize( w * h ) ;
                }
            }

            // filter points shader
            {
                motor::graphics::msl_object_t mslo("filter_points") ;

                mslo.add( motor::graphics::msl_api_type::msl_4_0, R"(
                config filter_stream
                {
                    vertex_shader
                    {
                        inout vec4_t pos : position ;
                        inout vec4_t color : color ;
                            
                        uint_t vid : vertex_id ;

                        void main()
                        {
                            out.pos = in.pos ; 
                            out.color = in.color ; 
                        }
                    }

                    geometry_shader
                    {
                        in points ;
                        out points[ max_verts = 1 ] ;

                        inout vec4_t pos : position ;
                        inout vec4_t color : color ;

                        uint_t pid : primitive_id ;

                        vec3_t plane ;

                        void main()
                        {
                            for( int i=0; i<in.length(); i++ )
                            {
                                out.pos = in[i].pos ;
                                out.color = in[i].color ;
                                    
                                // recolor every 2nd vertex
                                //if( pid % 2 == 0 ) { out.color = vec4_t( 0.0, 1.0, 1.0, 1.0 ) ; }

                                float_t weight = in[i].pos.w ;
                                float_t pdp = dot( plane, in[i].pos.xyz ) ;
                        
                                // on the line
                                if( (pdp > -0.001) && (pdp < 0.001) ) 
                                { 
                                    out.pos.w = in[i].pos.w ;//0.005 ;
                                    out.color = vec4_t( 0.0, 0.0, 1.0, 1.0 ) ' in[i].color ;
                                    emit_vertex() ; 
                                }

                                // "under the line"
                                else if( pdp < 0.0 ) 
                                { 
                                    out.pos.w = in[i].pos.w;//0.0025 ;
                                    out.color = vec4_t( 1.0, 0.0, 0.0, 1.0 ) ' in[i].color ;
                                    emit_vertex() ; 
                                }

                                // "above" the line
                                else if( pdp > 0.0 ) 
                                { 
                                    out.pos.w = in[i].pos.w ;//0.005 ;
                                    out.color = vec4_t( 0.0, 1.0, 0.0, 1.0 ) ' in[i].color ;
                                    //emit_vertex() ; 
                                }
                            }
                            end_primitive() ;
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

                msl_filter_obj = std::move( mslo ) ;
            }

            // render shader
            {
                motor::graphics::msl_object_t mslo("render") ;

                mslo.add( motor::graphics::msl_api_type::msl_4_0, R"(
                config render_original
                {
                    vertex_shader
                    {
                        in vec4_t pos : position ;
                        in vec4_t color : color ;

                        out vec4_t pos : position ;
                        out vec4_t color : color ;
        
                        uint_t vid : vertex_id ;
                            
                        data_buffer_t u_filtered ;

                        out float_t scale ;

                        void main()
                        {
                            int_t idx = vid ;
                            vec4_t pos = fetch_data( u_filtered, (idx << 1) + 0 ) ; 
                            vec4_t color = fetch_data( u_filtered, (idx << 1) + 1 ) ;

                            out.scale = 0.0025;//pos.w * 2.0 ;

                            out.pos = vec4_t( pos.xyz, 1.0 ) ;
                            //out.pos = out.pos ;
                            out.color = color ' in.color;
                        }
                    }

                    geometry_shader
                    {
                        in points ;
                        out triangles[ max_verts = 6 ] ;

                        inout vec4_t pos : position ;
                        inout vec4_t color : color ;

                        in float_t scale ;

                        void main()
                        {
                            for( int i=0; i<in.length(); i++ )
                            {
                                {
                                    out.pos = vec4_t( in[i].pos.xy + vec2_t( -in[i].scale, -in[i].scale ), 0.0, 1.0 ) ;
                                    out.color = in[i].color ;
                                    emit_vertex() ;
                                }
                                {
                                    out.pos =  vec4_t( in[i].pos.xy + vec2_t( -in[i].scale, in[i].scale ), 0.0, 1.0 ) ;
                                    out.color = in[i].color ;
                                    emit_vertex() ;
                                }
                                {
                                    out.pos =  vec4_t( in[i].pos.xy + vec2_t( +in[i].scale, +in[i].scale ), 0.0, 1.0 ) ;
                                    out.color = in[i].color ;
                                    emit_vertex() ;
                                }
                                end_primitive() ;

                                {
                                    out.pos =  vec4_t( in[i].pos.xy + vec2_t( -in[i].scale, -in[i].scale ), 0.0, 1.0 ) ;
                                    out.color = in[i].color ;
                                    emit_vertex() ;
                                }
                                {
                                    out.pos =  vec4_t( in[i].pos.xy + vec2_t( in[i].scale, in[i].scale ), 0.0, 1.0 ) ;
                                    out.color = in[i].color ;
                                    emit_vertex() ;
                                }
                                {
                                    out.pos =  vec4_t( in[i].pos.xy + vec2_t( in[i].scale, -in[i].scale ), 0.0, 1.0 ) ;
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

                // must set soo name so number of captured primitives can be 
                // retrieved from the streamout object.
                mslo.link_geometry( "points", "filtered_points" ) ;

                // add variable set
                {
                    motor::graphics::variable_set_t vars ;
                    {
                        auto* var = vars.array_variable_streamout( "u_filtered" ) ;
                        var->set( "filtered_points" ) ;
                    }
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
            if ( wid == 2 ) return ;

            if( rd.first_frame )
            {
                fe->configure<motor::graphics::geometry_object_t>( &geo_pts_obj ) ;
                fe->configure<motor::graphics::streamout_object_t>( &so_obj ) ;
                fe->configure<motor::graphics::msl_object_t>( &msl_filter_obj ) ;
                fe->configure<motor::graphics::msl_object_t>( &msl_obj ) ;

                // do initial streamout pass 
                // in order to fill the buffer
                {
                    fe->use( &so_obj ) ;
                    fe->render( &msl_filter_obj, motor::graphics::gen4::backend::render_detail_t() ) ;
                    fe->unuse( motor::graphics::gen4::backend::unuse_type::streamout ) ;
                }
            }

            {
                fe->use( &so_obj ) ;
                motor::graphics::gen4::backend::render_detail_t detail ;
                detail.feed_from_streamout = false ;
                fe->render( &msl_filter_obj, detail ) ;
                fe->unuse( motor::graphics::gen4::backend::unuse_type::streamout ) ;
            }

            // draw the cutting plane
            {
                auto dir2 = dir.xy().normalized().ortho() ;
                motor::math::mat2f_t basis ;
                basis.set_column( 0, motor::math::vec2f_t( dir ) ) ;
                basis.set_column( 1, dir2 ) ;

                motor::math::vec2f_t p0 = basis * motor::math::vec2f_t(0.0f, -250.0f ) ;
                motor::math::vec2f_t p1 = basis * motor::math::vec2f_t(0.0f, +250.0f ) ;
                // @todo need primitive renderer
                //_ae.get_prim_render()->draw_line( 0, p0, p1, motor::math::vec4f_t( 0.0f, 1.0f, 0.0f, 1.0f ) ) ;
            }

            msl_filter_obj.for_each( [&] ( size_t const i, motor::graphics::variable_set_mtr_t vs )
            {
                auto * var = vs->data_variable<motor::math::vec3f_t>("plane") ;
                var->set( dir ) ;
            } ) ;
            
            {
                motor::graphics::gen4::backend_t::render_detail_t detail ;
                detail.feed_from_streamout = false ;
                detail.use_streamout_count = true ;
                fe->render( &msl_obj, detail ) ;
            }
        }

        virtual bool_t on_tool( this_t::window_id_t const wid, motor::application::app::tool_data_ref_t ) noexcept 
        {
            //if ( wid != 2 ) return false ;

            ImGui::Begin( "Test Control" ) ;

            {
                motor::math::vec2f_t d = dir.xy() ;
                if( motor::tool::custom_imgui_widgets::direction( "dir", d ) )
                {
                    dir = motor::math::vec3f_t( d, 0.0f ) ;
                }
            }

            ImGui::End() ;
            
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
    motor::profiling::global::deinit() ;
    motor::memory::global::dump_to_std() ;


    return ret ;
}