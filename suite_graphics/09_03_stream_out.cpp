
#include <motor/platform/global.h>
#include <motor/application/window/window_message_listener.h>

#include <motor/graphics/object/msl_object.h>
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

        motor::graphics::state_object_t scene_so ;
        motor::graphics::geometry_object_t quad_geo ;
        motor::graphics::geometry_object_t points_geo ;

        // capture and bind streamed out geometry
        motor::graphics::streamout_object_t so_obj ;

        // render object : doing stream out
        motor::graphics::msl_object_t msl_so_obj ;

        // render object : rendering original geometry
        motor::graphics::msl_object_t msl_orig_obj ;

        motor::math::vec4f_t particle_bounds = motor::math::vec4f_t( -1000.0f, -1000.0f, 1000.0f, 1000.0f ) ;
        size_t num_particles = 1000 ;

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
            }

            // geometry configuration
            {
                struct vertex { motor::math::vec4f_t pos ; motor::math::vec4f_t color ; } ;

                auto vb = motor::graphics::vertex_buffer_t()
                    .add_layout_element( motor::graphics::vertex_attribute::position, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                    .add_layout_element( motor::graphics::vertex_attribute::color0, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                    .resize( 4 * num_particles ).update<vertex>( [=] ( vertex* array, size_t const ne )
                {
                    for( size_t i=0; i<num_particles; ++i )
                    {
                        size_t const idx = i * 4 ;
                        array[ idx + 0 ].pos = motor::math::vec4f_t( -0.5f, -0.5f, 0.0f, 1.0f ) ;
                        array[ idx + 1 ].pos = motor::math::vec4f_t( -0.5f, +0.5f, 0.0f, 1.0f ) ;
                        array[ idx + 2 ].pos = motor::math::vec4f_t( +0.5f, +0.5f, 0.0f, 1.0f ) ;
                        array[ idx + 3 ].pos = motor::math::vec4f_t( +0.5f, -0.5f, 0.0f, 1.0f ) ;

                        array[ idx + 0 ].color = motor::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ) ;
                        array[ idx + 1 ].color = motor::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ) ;
                        array[ idx + 2 ].color = motor::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ) ;
                        array[ idx + 3 ].color = motor::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ) ;
                    }
                } );

                auto ib = motor::graphics::index_buffer_t().
                    set_layout_element( motor::graphics::type::tuint ).resize( 6 * num_particles ).
                    update<uint_t>( [&] ( uint_t* array, size_t const ne )
                {
                    for( uint_t i=0; i< uint_t(num_particles); ++i )
                    {
                        uint_t const vidx = i * 4 ;
                        uint_t const idx = i * 6 ;

                        array[ idx + 0 ] = vidx + 0 ;
                        array[ idx + 1 ] = vidx + 1 ;
                        array[ idx + 2 ] = vidx + 2 ;

                        array[ idx + 3 ] = vidx + 0 ;
                        array[ idx + 4 ] = vidx + 2 ;
                        array[ idx + 5 ] = vidx + 3 ;
                    }
                } ) ;

                quad_geo = motor::graphics::geometry_object_t( "quads",
                    motor::graphics::primitive_type::triangles, std::move( vb ), std::move( ib ) ) ;
            }

            // geometry configuration
            {
                struct sim_vertex { 
                    motor::math::vec4f_t pos ; motor::math::vec4f_t vel ; 
                    motor::math::vec4f_t accel; motor::math::vec4f_t force ;
                } ;

                auto vb = motor::graphics::vertex_buffer_t()
                    .add_layout_element( motor::graphics::vertex_attribute::position, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                    .add_layout_element( motor::graphics::vertex_attribute::color0, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                    .add_layout_element( motor::graphics::vertex_attribute::color1, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                    .add_layout_element( motor::graphics::vertex_attribute::color2, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                    .resize(num_particles ).update<sim_vertex>( [=] ( sim_vertex* array, size_t const ne )
                {
                    for( size_t i=0; i<num_particles; ++i )
                    {
                        float_t const mass = float_t((i%10)+1)/10.0f ;
                        array[ i ].pos = motor::math::vec4f_t( 0.0f, 0.0f, 0.0f, mass )  + 
                                motor::math::vec4f_t( particle_bounds.x()*2.0f, 0.0f, 0.0f, 0.0f ) ;
                        array[ i ].vel = motor::math::vec4f_t( 0.0f, 0.0f, 0.0f, 1.0f ) ;
                        array[ i ].accel = motor::math::vec4f_t( 0.0f, 0.0f, 0.0f, 1.0f ) ;
                        array[ i ].force = motor::math::vec4f_t( 0.0f, -9.81f, 0.0f, 1.0f ) ;
                    }
                } );

                points_geo = motor::graphics::geometry_object_t( "points",
                    motor::graphics::primitive_type::points, std::move( vb ) ) ;
            }

            // stream out object configuration
            {
                auto vb = motor::graphics::vertex_buffer_t()
                    .add_layout_element( motor::graphics::vertex_attribute::position, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                    .add_layout_element( motor::graphics::vertex_attribute::color0, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                    .add_layout_element( motor::graphics::vertex_attribute::color1, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                    .add_layout_element( motor::graphics::vertex_attribute::color2, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 ) ;

                so_obj = motor::graphics::streamout_object_t( "compute", std::move( vb ) ).resize( num_particles ) ;
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
                            vec4_t pos = fetch_data( u_data, (idx << 2) + 0 ) ; 
                            //vec4_t vel = fetch_data( u_data, (idx << 2) + 1 ) ;

                            pos = vec4_t( pos.xyz ' vec3_t(0.001, 0.001, 0.001), 1.0 ) ;
                            out.color = in.color ;
                            out.pos = ((in.pos'vec4_t(0.01,0.01,0.01,1.0))+ vec4_t(pos.xyz,0.0)) ;
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

                        
                mslo.link_geometry( {"quads"} ) ;

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
                        inout vec4_t pos : position ;
                        inout vec4_t vel : color0 ;
                        inout vec4_t accel : color1 ;
                        inout vec4_t force : color2 ;
                            
                        uint_t vid : vertex_id ;

                        vec4_t u_bounds ;
                        float_t u_dt ;

                        void main()
                        {
                            float_t dt = u_dt ;
                            float_t mass = in.pos.w ;
                            vec3_t acl = in.force.xyz / mass + in.accel.xyz ;
                            vec3_t vel = acl * dt + in.vel.xyz ;
                            vec3_t pos = vel * dt + in.pos.xyz ;

                            if( any( less_than( pos, as_vec3( u_bounds.x ) ) ) || 
                                any( greater_than( pos, as_vec3( u_bounds.z ) ) ) )
                            {
                                pos = vec3_t( float_t(vid%30)*20-300.0, 200.0, 0.0 ) ;
                                pos.y += float_t( (vid / 30) * 15 ) ;

                                vel = as_vec3( 0.0 ) ;
                                acl = as_vec3( 0.0 ) ;
                            }

                            out.pos = vec4_t( pos, in.pos.w ) ;
                            out.vel = vec4_t( vel, 0.0 ) ;
                            out.accel = as_vec4(0.0) ;
                            out.force = in.force ;
                        }
                    }

                    // 1. automatically enable streamout when no pixel shader is present.
                    // 2. the layout elements of the vertex buffer bound to the streamout object used 
                    //      MUST MATCH the number of output variables of this shader.
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
                        
            msl_so_obj.for_each( [&] ( size_t const i, motor::graphics::variable_set_mtr_t vs )
            {
                {
                    auto * var = vs->data_variable<motor::math::float_t>("u_dt") ;
                    var->set( float_t( rd.sec_dt ) ) ;
                }
                {
                    auto * var = vs->data_variable<motor::math::vec4f_t>("u_bounds") ;
                    var->set( particle_bounds ) ;
                }
            } ) ;
            
            // render streamed out
            {
                motor::graphics::gen4::backend_t::render_detail_t detail ;
                detail.feed_from_streamout = false ;
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