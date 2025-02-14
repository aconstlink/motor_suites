
#include <motor/platform/global.h>
#include <motor/application/window/window_message_listener.h>

#include <motor/tool/imgui/custom_widgets.h>

#include <motor/geometry/mesh/tri_mesh.h>
#include <motor/geometry/mesh/flat_tri_mesh.h>
#include <motor/geometry/3d/cube.h>

#include <motor/graphics/object/render_object.h>
#include <motor/graphics/object/geometry_object.h>
#include <motor/graphics/object/msl_object.h>

#include <motor/math/interpolation/interpolate.hpp>

#include <motor/io/database.h>
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

        motor::graphics::image_object_t img_obj ;
        motor::graphics::geometry_object_t geo_obj ;
        motor::graphics::msl_object_mtr_t msl_obj ;
        motor::graphics::msl_object_mtr_t msl_lib_obj ;

        motor::io::monitor_mtr_t mon = motor::memory::create_ptr( motor::io::monitor_t() ) ;
        motor::io::database db = motor::io::database( motor::io::path_t( DATAPATH ), "./working", "data" ) ;

        motor::vector< motor::graphics::msl_object_ptr_t > reconfigs ;

        //***********************************************************************************
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

            db.attach( mon ) ;

            {
                struct vertex { motor::math::vec3f_t pos ; motor::math::vec2f_t tx ; } ;

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
                
                // load shader libs first
                {
                    motor::graphics::msl_object_t mslo("") ;

                    motor::string_t shd ;

                    motor::vector< motor::io::location_t > locations = { 
                        motor::io::location_t( "shaders.13_msl_buildins_1.mylib.msl" ) } ;

                    for( auto const & l : locations )
                    {
                        auto const res = db.load( l ).wait_for_operation( [&] ( char_cptr_t data, size_t const sib, motor::io::result const ) 
                        { 
                            mslo.add( motor::graphics::msl_api_type::msl_4_0, motor::string_t( data, sib ) ) ;
                        } ) ;

                        if( !res ) 
                        {
                            motor::log::global_t::status( "loading shader failed" ) ;
                        }
                    }

                    msl_lib_obj = motor::shared( std::move( mslo ) ) ;
                    reconfigs.emplace_back( msl_lib_obj ) ;
                }

                // render shader
                {
                    motor::graphics::msl_object_t mslo("my_render_msl") ;

                    motor::string_t shd ;
                    auto const res =  db.load( motor::io::location_t( "shaders.13_msl_buildins_1.render.msl" ) ).wait_for_operation( [&] ( char_cptr_t data, size_t const sib, motor::io::result const ) 
                    { 
                        shd = motor::string_t( data, sib ) ;
                    } ) ;

                    if( !res ) 
                    {
                        motor::log::global_t::status( "loading shader failed" ) ;
                    }

                    mslo.add( motor::graphics::msl_api_type::msl_4_0, shd ) ;
                        
                    mslo.link_geometry( "quad" ) ;

                    // variable sets
                    {
                        motor::graphics::variable_set_t vars ;
                        {
                            auto* var = vars.data_variable< motor::math::vec4f_t >( "u_color" ) ;
                            var->set( motor::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ) ) ;
                        }

                        {
                            auto * var = vars.texture_variable( "u_tex" ) ;
                            var->set( "checker_board" ) ;
                        }

                        mslo.add_variable_set( motor::memory::create_ptr( std::move( vars ), "a variable set" ) ) ;
                    }

                    msl_obj = motor::shared( std::move( mslo ) ) ;
                    reconfigs.emplace_back( msl_obj ) ;
                }
            }
        }

        //***********************************************************************************
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

        //***********************************************************************************
        virtual void_t on_update( motor::application::app::update_data_in_t ) noexcept 
        {
            mon->for_each_and_swap( [&]( motor::io::location_cref_t loc, motor::io::monitor_t::notify const n )
            {
                motor::log::global_t::status( "[monitor] : Got " + motor::io::monitor_t::to_string(n) + " for " + loc.as_string() ) ;

                motor::string_t shd ;
                db.load( loc ).wait_for_operation( [&] ( char_cptr_t data, size_t const sib, motor::io::result const ) 
                { 
                    shd = motor::string_t( data, sib ) ;
                } ) ;

                msl_lib_obj->clear_shaders().add( motor::graphics::msl_api_type::msl_4_0, shd ) ;

                reconfigs.push_back( msl_lib_obj ) ;
            }) ;
        }

        bool_t animate = false ;
        float_t my_mult = 10.0f ;
        float_t my_layer = 10.0f ;

        //***********************************************************************************
        virtual void_t on_graphics( motor::application::app::graphics_data_in_t gdi ) noexcept 
        {
            static float_t t = 0.0f ;
            t += gdi.sec_dt * 0.25f;
            if( t > 1.0f ) t = 0.0f ;

            float_t const t2 = 1.0f - motor::math::fn<float_t>::abs( t * 2.0f - 1.0f ) ;

            if( animate )
                my_mult = motor::math::interpolation<float_t>::linear( 1.0f, 20.0f, t2 ) ;

            msl_obj->for_each( [&]( size_t const, motor::graphics::variable_set_mtr_t vs )
            {
                {
                    auto * var = vs->data_variable<float_t>( "u_mult" ) ;
                    var->set( my_mult ) ;
                }

                {
                    auto * var = vs->data_variable<float_t>( "u_layer" ) ;
                    var->set( my_layer ) ;
                }
            } ) ;
        } ;

        //***********************************************************************************
        virtual void_t on_render( this_t::window_id_t const wid, motor::graphics::gen4::frontend_ptr_t fe,
            motor::application::app::render_data_in_t rd ) noexcept 
        {            
            if( rd.first_frame )
            {
                fe->configure<motor::graphics::geometry_object_t>( &geo_obj ) ;
                fe->configure<motor::graphics::image_object_t>( &img_obj ) ;
                //fe->configure<motor::graphics::msl_object_t>( &msl_obj ) ;
            }
            
            for( auto * obj : reconfigs )
            {
                fe->configure<motor::graphics::msl_object_t>( obj ) ;
            }

            {
                motor::graphics::gen4::backend_t::render_detail_t detail ;
                fe->render( msl_obj, detail ) ;
            }
        }

        //***********************************************************************************
        virtual void_t on_frame_done( void_t ) noexcept 
        {
            reconfigs.clear() ;
        } 

        //***********************************************************************************
        virtual bool_t on_tool( this_t::window_id_t const wid, motor::application::app::tool_data_ref_t ) noexcept 
        { 
            if( wid != 0 ) return false ;

            if( ImGui::Begin( "Controls" ) )
            {
                ImGui::Checkbox( "Animate", &animate ) ;
                ImGui::SliderFloat( "Noise Multiplication", &my_mult, 1.0f, 20.0f ) ;
                ImGui::SliderFloat( "Noise Layer", &my_layer, 1.0f, 20.0f ) ;
            }
            ImGui::End() ;
            return true ; 
        }

        //******************************************************************************************************
        virtual void_t on_shutdown( void_t ) noexcept
        {
            motor::release( motor::move( msl_obj ) ) ;
            motor::release( motor::move( msl_lib_obj ) ) ;
        }
    };
}

int main( int argc, char ** argv )
{
    return motor::platform::global_t::create_and_exec< this_file::my_app >() ;
}