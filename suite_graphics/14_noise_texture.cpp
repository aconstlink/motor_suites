
#include <motor/platform/global.h>

#include <motor/graphics/frontend/gen4/frontend.hpp>
#include <motor/graphics/object/render_object.h>
#include <motor/graphics/object/geometry_object.h>

#include <motor/noise/method/gradient_noise.h>
#include <motor/noise/method/fbm.hpp>

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

        motor::graphics::state_object_t root_so ;
        motor::graphics::geometry_object_t geo_obj ;
        motor::graphics::image_object_t img_obj ;

        motor::graphics::msl_object_t msl_obj ;

        struct vertex { motor::math::vec3f_t pos ; motor::math::vec2f_t tx ; } ;

        bool_t change_noise = false ;
        bool_t update_image = false ;
        uint_t seed = 127436 ;
        uint_t bit = 8 ;
        uint_t mixes = 3 ;

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
                motor::noise::gradient_noise_t vn( seed, bit, mixes ) ;

                motor::graphics::image_t img = motor::graphics::image_t( motor::graphics::image_t::dims_t( 1024, 1024 ) )
                    .update( [&]( motor::graphics::image_ptr_t, motor::graphics::image_t::dims_in_t dims, void_ptr_t data_in )
                {
                    typedef motor::math::vector4< uint8_t > rgba_t ;
                    auto* data = reinterpret_cast< rgba_t * >( data_in ) ;

                    size_t i = 0 ; 
                    for( size_t y = 0; y < dims.y(); ++y )
                    {
                        for( size_t x = 0; x < dims.x(); ++x )
                        {
                            motor::math::vec2f_t const p( 
                                20.43f*(float_t(x)/float_t(dims.x())), 
                                20.43f*(float_t(y)/float_t(dims.y())) ) ;
                            uint8_t const nv = uint8_t( (vn.noise( p )*0.5f+0.5f) * 255.0f ) ;

                            data[ i++ ] = rgba_t( nv, nv, nv, 255 );
                            //data[ i++ ] = rgba_t(255) ;
                        }
                    }
                } ) ;

                img_obj = motor::graphics::image_object_t( "noise_texture", std::move( img ) )
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
                                out.pos = vec4_t( sign( in.pos ), 1.0 ) ; 
                            }
                        }

                        pixel_shader
                        {
                            tex2d_t u_tex ;

                            in vec2_t tx : texcoord0 ;
                            out vec4_t color : color ;

                            void main()
                            {
                                out.color = texture( u_tex, in.tx )  ;
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
                        var->set( "noise_texture" ) ;
                    }

                    msl_obj.add_variable_set( motor::memory::create_ptr( std::move( vars ), "a variable set" ) ) ;
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
                        var->set( "noise_texture" ) ;
                    }

                    msl_obj.add_variable_set( motor::memory::create_ptr( std::move(vars), "a variable set" ) ) ;
                }
            }

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

        virtual void_t on_graphics( motor::application::app::graphics_data_in_t ) noexcept 
        {
            if( change_noise )
            {
                motor::noise::gradient_noise_t vn( seed, bit, mixes ) ;

                img_obj.image().update( [&]( motor::graphics::image_ptr_t, motor::graphics::image_t::dims_in_t dims, void_ptr_t data_in )
                {
                    typedef motor::math::vector4< uint8_t > rgba_t ;
                    auto* data = reinterpret_cast< rgba_t * >( data_in ) ;

                    size_t i = 0 ; 
                    for( size_t y = 0; y < dims.y(); ++y )
                    {
                        for( size_t x = 0; x < dims.x(); ++x )
                        {
                            motor::math::vec2f_t const p( 
                                20.43f*(float_t(x)/float_t(dims.x())), 
                                20.43f*(float_t(y)/float_t(dims.y())) ) ;
                            uint8_t const nv = uint8_t( (vn.noise( p )*0.5f+0.5f) * 255.0f ) ;

                            data[ i++ ] = rgba_t( nv, nv, nv, 255 );
                            //data[ i++ ] = rgba_t(255) ;
                        }
                    }
                } ) ;

                change_noise = false ;
                update_image = true ;
            }
        }

        virtual void_t on_render( this_t::window_id_t const wid, motor::graphics::gen4::frontend_ptr_t fe,
            motor::application::app::render_data_in_t rd ) noexcept 
        {   
            // configure needs to be done only once per window
            if( rd.first_frame )
            {
                fe->configure<motor::graphics::state_object_t>( &root_so ) ;
                fe->configure<motor::graphics::geometry_object_t>( &geo_obj ) ;
                fe->configure<motor::graphics::image_object_t>( &img_obj ) ;
                fe->configure<motor::graphics::msl_object_t>( &msl_obj ) ;
            }

            if( update_image )
            {
                fe->update( &img_obj ) ;
            }
            
            // render
            {
                fe->push( &root_so ) ;
                {
                    motor::graphics::gen4::backend_t::render_detail_t detail ;
                    detail.start = 0 ;
                    //detail.num_elems = 3 ;
                    detail.varset = 0 ;
                    fe->render( &msl_obj, detail ) ;
                }
                fe->pop( motor::graphics::gen4::backend::pop_type::render_state ) ;
            }
        }

        virtual void_t on_frame_done( void_t ) noexcept 
        {
            update_image = false ;
        }

        virtual bool_t on_tool( this_t::window_id_t const wid, motor::application::app::tool_data_ref_t ) noexcept 
        { 
            if( wid != 0 ) return false ;

            {
                if( ImGui::Begin("Control Window") )
                {
                    int_t v = int_t( seed ) ;
                    if( ImGui::SliderInt( "Seed", &v, 0, 150000 ) ) change_noise = true ;
                    seed = uint_t(v ) ;
                }

                {
                    int_t v = int_t( mixes ) ;
                    if( ImGui::SliderInt( "Mix", &v, 0, 7 ) ) change_noise = true ;
                    mixes = uint_t(v ) ;
                }
                ImGui::End() ;
            }

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
    motor::memory::global::dump_to_std() ;


    return ret ;
}