
#include <motor/platform/global.h>
#include <motor/application/window/window_message_listener.h>

#include <motor/graphics/object/render_object.h>
#include <motor/graphics/object/geometry_object.h>

#include <motor/format/global.h>
#include <motor/format/future_items.hpp>

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

        motor::io::database db = motor::io::database( motor::io::path_t( DATAPATH ), "./working", "data" ) ;

        motor::math::vec4ui_t fb_dims = motor::math::vec4ui_t( 0, 0, 1920, 1080 ) ;

        motor::graphics::state_object_t scene_so ;
        motor::graphics::geometry_object_t geo_obj ;
        motor::graphics::image_object_t img_obj ;
        motor::graphics::shader_object_t sh_obj ;
        motor::graphics::msl_object_t msl_obj ;

        int_t max_textures = 3 ;
        int_t used_texture = 0 ;

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

            #if 0
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
            #endif
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

            // geometry configuration for post quad
            {
                struct vertex { motor::math::vec2f_t pos ; } ;

                auto vb = motor::graphics::vertex_buffer_t()
                    .add_layout_element( motor::graphics::vertex_attribute::position, motor::graphics::type::tfloat, motor::graphics::type_struct::vec2 )
                    .resize( 4 ).update<vertex>( [=] ( vertex* array, size_t const ne )
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

                geo_obj = motor::graphics::geometry_object_t( "quad",
                    motor::graphics::primitive_type::triangles, std::move( vb ), std::move( ib ) ) ;
            }

            // image configuration
            {
                motor::format::module_registry_mtr_t mod_reg = motor::format::global::register_default_modules( 
                    motor::shared( motor::format::module_registry_t() ) ) ;

                motor::format::future_item_t items[4] = 
                {
                    mod_reg->import_from( motor::io::location_t( "images.1.png" ), &db ),
                    mod_reg->import_from( motor::io::location_t( "images.2.png" ), &db ),
                    mod_reg->import_from( motor::io::location_t( "images.3.png" ), &db ),
                    mod_reg->import_from( motor::io::location_t( "images.4.png" ), &db )
                } ;

                // taking all slices
                motor::graphics::image_t img ;

                // load each slice into the image
                for( size_t i=0; i<4; ++i )
                {
                    if( auto * ii = dynamic_cast<motor::format::image_item_mtr_t>( items[i].get() ); ii != nullptr )
                    {
                        img.append( *ii->img ) ;
                        motor::memory::release_ptr( ii->img ) ;
                        motor::memory::release_ptr( ii ) ;
                    }
                    else
                    {
                        motor::memory::release_ptr( ii ) ;
                        std::exit( 1 ) ;
                    }
                }

                img_obj = motor::graphics::image_object_t( 
                    "image_array", std::move( img ) )
                    .set_type( motor::graphics::texture_type::texture_2d_array )
                    .set_wrap( motor::graphics::texture_wrap_mode::wrap_s, motor::graphics::texture_wrap_type::repeat )
                    .set_wrap( motor::graphics::texture_wrap_mode::wrap_t, motor::graphics::texture_wrap_type::repeat )
                    .set_filter( motor::graphics::texture_filter_mode::min_filter, motor::graphics::texture_filter_type::nearest )
                    .set_filter( motor::graphics::texture_filter_mode::mag_filter, motor::graphics::texture_filter_type::nearest );

                motor::memory::release_ptr( mod_reg ) ;
            }

            // render texture array
            {
                motor::graphics::msl_object_t mslo("scene_obj") ;

                mslo.add( motor::graphics::msl_api_type::msl_4_0, R"(
                config test_texture_array
                {
                    vertex_shader
                    {
                        int_t quad ; // in [0,1] left or right quad

                        in vec3_t pos : position ;

                        out vec4_t pos : position ;
                        out vec2_t tx : texcoord ;

                        void main()
                        {
                            vec2_t offset = { vec2_t(-0.5, 0.0), vec2_t(0.5,0.0) } ;
                            out.pos = vec4_t( in.pos.xy ' as_vec2(0.85) + offset[quad], 0.0, 1.0 ) ;
                            out.tx = sign( in.pos.xy ) ' as_vec2(0.5) + as_vec2( 0.5 ) ;
                        }
                    }

                    pixel_shader
                    {
                        int_t quad ; // in [0,1] left or right quad
                        int_t tx_id ; // in [0,3] choosing the sampler in u_tex

                        tex2d_array_t u_tex ;

                        in vec2_t tx : texcoord ;
                        out vec4_t color : color0 ;

                        void main()
                        {
                            vec2_t uv = fract( in.tx * 2.0 ) ;
                            int_t quadrant = int_t( dot( floor( in.tx * 2.0 ), vec2_t(1,2) ) ) ;
                            int_t idx = quad * tx_id + quadrant * ( 1 - quad ) ;
                            out.color = texture( u_tex, vec3_t(uv, float_t(idx) ) ) ;
                        }
                    }

                })" ) ;

                        
                mslo.link_geometry({"quad"}) ;

                msl_obj = std::move( mslo ) ;
            }
            

            // place variable sets
            {
                // add variable set 0
                {
                    motor::graphics::variable_set_t vars ;
                    
                    {
                        auto* var = vars.texture_variable( "u_tex" ) ;
                        var->set( "image_array" ) ;
                    }

                    {
                        auto * var = vars.data_variable<int32_t>("quad" ) ;
                        var->set( 0 ) ;
                    }

                    {
                        auto * var = vars.data_variable<int32_t>("tx_id" ) ;
                        var->set( 0 ) ;
                    }

                    msl_obj.add_variable_set( motor::shared( std::move( vars ) ) ) ;
                }

                {
                    motor::graphics::variable_set_t vars ;
                    
                    {
                        auto* var = vars.texture_variable( "u_tex" ) ;
                        var->set( "image_array" ) ;
                    }

                    {
                        auto * var = vars.data_variable<int32_t>("quad" ) ;
                        var->set( 1 ) ;
                    }

                    {
                        auto * var = vars.data_variable<int32_t>("tx_id" ) ;
                        var->set( 0 ) ;
                    }

                    msl_obj.add_variable_set( motor::shared( std::move( vars ) ) ) ;
                }
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

        virtual void_t on_render( this_t::window_id_t const wid, motor::graphics::gen4::frontend_ptr_t fe,
            motor::application::app::render_data_in_t rd ) noexcept 
        {            
            if( rd.first_frame )
            {
                fe->configure<motor::graphics::state_object_t>( &scene_so ) ;
                fe->configure<motor::graphics::geometry_object_t>( &geo_obj ) ;
                fe->configure<motor::graphics::image_object_t>( &img_obj ) ;
                fe->configure<motor::graphics::shader_object_t>( &sh_obj ) ;
                fe->configure<motor::graphics::msl_object_t>( &msl_obj ) ;
            }

            msl_obj.for_each( [&] ( size_t const i, motor::graphics::variable_set_mtr_t vs )
            {
                {
                    auto* var = vs->data_variable< int32_t >( "tx_id" ) ;
                    var->set( used_texture ) ;
                }
            } ) ;

            {
                // left quad
                {
                    motor::graphics::gen4::backend_t::render_detail_t detail ;
                    detail.varset = 0 ;
                    fe->render( &msl_obj, detail ) ;
                }
                // right quad
                {
                    motor::graphics::gen4::backend_t::render_detail_t detail ;
                    detail.varset = 1 ;
                    fe->render( &msl_obj, detail ) ;
                }
            }
        }

        virtual bool_t on_tool( this_t::window_id_t const, motor::application::app::tool_data_ref_t ) noexcept
        {
            ImGui::Begin( "Test Control" ) ;

            if( ImGui::SliderInt( "Use Texture", &used_texture, 0, max_textures ) ){} 

            ImGui::End() ;

            return true ;
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
    
    motor::io::global::deinit() ;
    motor::concurrent::global::deinit() ;
    motor::log::global::deinit() ;
    motor::memory::global::dump_to_std() ;


    return ret ;
}