
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

        motor::graphics::state_object_t scene_so ;
        motor::graphics::geometry_object_t quad_geo ;
        motor::graphics::geometry_object_t points_geo ;

        // capture and bind streamed out geometry
        motor::graphics::streamout_object_t so_obj ;

        // render object : doing stream out
        motor::graphics::msl_object_t msl_so_obj ;

        // render object : rendering original geometry
        motor::graphics::msl_object_t msl_orig_obj ;


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
        }

        virtual void_t on_graphics( motor::application::app::graphics_data_in_t gd ) noexcept 
        {
            if( !graphics_init ) 
            {
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

                    quad_geo = motor::graphics::geometry_object_t( "quad",
                        motor::graphics::primitive_type::triangles, std::move( vb ), std::move( ib ) ) ;
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

                    points_geo = motor::graphics::geometry_object_t( "points",
                        motor::graphics::primitive_type::points, std::move( vb ) ) ;
                }

                // stream out object configuration
                {
                    auto vb = motor::graphics::vertex_buffer_t()
                        .add_layout_element( motor::graphics::vertex_attribute::position, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                        .add_layout_element( motor::graphics::vertex_attribute::color0, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 ) ;


                    so_obj = motor::graphics::streamout_object_t( "compute", std::move( vb ) ).resize( 1000 ) ;
                }

                // render original
                {
                    motor::graphics::msl_object_t mslo("render_original") ;

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

                            // will contain the transform feedback data
                            data_buffer_t u_data ;

                            void main()
                            {
                                int_t idx = vid / 4 ;
                                vec4_t pos = fetch_data( u_data, (idx << 1) + 0 ) ; 
                                vec4_t col = fetch_data( u_data, (idx << 1) + 1 ) ;

                                pos = vec4_t( (pos + in.pos).xyz, 1.0 ) ;
                                pos = pos ' vec4_t( 0.5, 0.5, 1.0, 1.0 ) ;
                                out.color = ( vid < 2) ? in.color : col ;
                                out.pos = pos ;
                            }
                        }

                        pixel_shader
                        {
                            in vec4_t color : color ;
                            out vec4_t color : color ;

                            void main()
                            {
                                out.color = in.color ;
                            }
                        }
                    })" ) ;

                        
                    mslo.link_geometry( {"quad"} ) ;

                    // add variable set 0
                    {
                        motor::graphics::variable_set_t vars ;
                        {
                            auto* var = vars.array_variable_streamout( "u_data" ) ;
                            var->set( "compute" ) ;
                        }
                        mslo.add_variable_set( motor::shared( std::move( vars ) ) ) ;
                    }

                    msl_orig_obj = std::move( mslo ) ;
                }

                // shader configuration
                {
                    motor::graphics::msl_object_t mslo("streamout") ;

                    mslo.add( motor::graphics::msl_api_type::msl_4_0, R"(
                    config stream_out
                    {
                        vertex_shader
                        {
                            in vec4_t pos : position ;
                            in vec4_t color : color ;

                            out vec4_t pos : position ;
                            out vec4_t color : color ;

                            float_t u_ani ;

                            void main()
                            {
                                float_t t = u_ani * 3.14526 * 2.0 ;
                                out.pos = in.pos + vec4_t( 0.02 * cos(t), 0.02 * sin(t), 0.0, 0.0 ) ;
                                out.color = vec4_t( 1.0, 1.0, 0.0, 1.0 ) ;
                            }
                        }

                        // automatically enable streamout when no pixel shader is present.
                    })" ) ;
                        
                    mslo.link_geometry( "points", "compute" ) ;

                    // add variable set
                    {
                        motor::graphics::variable_set_t vars ;
                        {}
                        mslo.add_variable_set( motor::shared( std::move( vars ) ) ) ;
                    }

                    msl_so_obj = std::move( mslo ) ;
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
                fe->configure<motor::graphics::geometry_object_t>( &quad_geo ) ;
                fe->configure<motor::graphics::geometry_object_t>( &points_geo ) ;
                fe->configure<motor::graphics::streamout_object_t>( &so_obj ) ;
                fe->configure<motor::graphics::msl_object_t>( &msl_so_obj ) ;
                fe->configure<motor::graphics::msl_object_t>( &msl_orig_obj ) ;

                // do initial streamout pass 
                // in order to fill the buffer
                {
                    fe->use( &so_obj ) ;
                    fe->render( &msl_so_obj, motor::graphics::gen4::backend::render_detail_t() ) ;
                    fe->unuse( motor::graphics::gen4::backend::unuse_type::streamout ) ;
                }
            }
            
            // do stream out
            {
                fe->use( &so_obj ) ;
                {
                    motor::graphics::gen4::backend_t::render_detail_t detail ;
                    detail.feed_from_streamout = true ;
                    fe->render( &msl_so_obj, detail ) ;
                }
                fe->unuse( motor::graphics::gen4::backend::unuse_type::streamout ) ;
            }

            // matrix variables here

            static float_t max_time = 2.0f ;
            static float_t time = 0.0f ;
            time += rd.sec_dt ;
            if( time > max_time ) time = 0.0f ;
            msl_so_obj.for_each( [&] ( size_t const i, motor::graphics::variable_set_mtr_t vs )
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
                fe->render( &msl_orig_obj, detail ) ;
            }
            #endif
            // render streamed out
            {
                motor::graphics::gen4::backend_t::render_detail_t detail ;
                detail.feed_from_streamout = true ;
                fe->render( &msl_orig_obj, detail ) ;
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