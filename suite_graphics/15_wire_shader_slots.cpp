
#include <motor/platform/global.h>
#include <motor/application/window/window_message_listener.h>

#include <motor/tool/imgui/custom_widgets.h>

#include <motor/geometry/mesh/tri_mesh.h>
#include <motor/geometry/mesh/flat_tri_mesh.h>
#include <motor/geometry/3d/cube.h>

#include <motor/gfx/camera/generic_camera.h>

#include <motor/graphics/object/geometry_object.h>
#include <motor/graphics/object/msl_object.h>
#include <motor/graphics/variable/wire_variable_bridge.h>

#include <motor/math/utility/angle.hpp>

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
        motor::graphics::msl_object_mtr_t _msl_obj ;

        motor::io::monitor_mtr_t mon = motor::shared( motor::io::monitor_t(), "DB monitor" ) ;
        motor::io::database db = motor::io::database( motor::io::path_t( DATAPATH ), "./working", "data" ) ;


        motor::vector< motor::graphics::msl_object_ptr_t > reconfigs ;
        motor::vector< motor::graphics::msl_object_ptr_t > renderables ;

        motor::vector< motor::graphics::wire_variable_bridge_mtr_t > _bridges ;

        size_t _cam_id = 0 ;
        // 0 : this is the free moving camera
        // 1 : second camera for testing shader variable bindings
        motor::gfx::generic_camera_mtr_t _cameras[ 2 ] ;

        motor::wire::output_slot< motor::math::mat4f_t > * _proj ;
        motor::wire::output_slot< motor::math::mat4f_t > * _view ;

        //************************************************************************************************
        virtual void_t on_init( void_t ) noexcept
        {
            {
                motor::application::window_info_t wi ;
                wi.x = 100 ;
                wi.y = 100 ;
                wi.w = 800 ;
                wi.h = 600 ;
                wi.gen = motor::application::graphics_generation::gen4_auto ;

                this_t::send_window_message( this_t::create_window( wi ), [&] ( motor::application::app::window_view & wnd )
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

            db.attach( mon ) ;

            // camera
            {
                auto cam = motor::gfx::generic_camera_t( 1.0f, 1.0f, 0.1f, 500.0f ) ;
                cam.perspective_fov( motor::math::angle<float_t>::degree_to_radian( 30.0f ) ) ;
                cam.look_at( motor::math::vec3f_t( 0.0f, 0.0f, -400.0f ),
                    motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ), motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) ) ;

                _cameras[ 0 ] = motor::shared( std::move( cam ) ) ;
            }

            // camera
            {
                auto cam = motor::gfx::generic_camera_t( 1.0f, 1.0f, 1.0f, 100.0f ) ;
                cam.perspective_fov( motor::math::angle<float_t>::degree_to_radian( 90.0f ) ) ;
                cam.look_at( motor::math::vec3f_t( -50.0f, 20.0f, -100.0f ),
                    motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ), motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) ) ;

                _cameras[ 1 ] = motor::shared( std::move( cam ) ) ;
            }

            {
                _proj = motor::shared( motor::wire::output_slot<motor::math::mat4f_t>() ) ;
                _view = motor::shared( motor::wire::output_slot<motor::math::mat4f_t>() ) ;
            }

            // # : make geometry
            {
                struct vertex { motor::math::vec3f_t pos ; motor::math::vec3f_t nrm ; motor::math::vec2f_t tx ; } ;

                // cube
                {
                    motor::geometry::cube_t::input_params ip ;
                    ip.scale = motor::math::vec3f_t( 1.0f ) ;
                    ip.tess = 100 ;

                    motor::geometry::tri_mesh_t tm ;
                    motor::geometry::cube_t::make( &tm, ip ) ;

                    motor::geometry::flat_tri_mesh_t ftm ;
                    tm.flatten( ftm ) ;

                    auto vb = motor::graphics::vertex_buffer_t()
                        .add_layout_element( motor::graphics::vertex_attribute::position, motor::graphics::type::tfloat, motor::graphics::type_struct::vec3 )
                        .add_layout_element( motor::graphics::vertex_attribute::normal, motor::graphics::type::tfloat, motor::graphics::type_struct::vec3 )
                        .add_layout_element( motor::graphics::vertex_attribute::texcoord0, motor::graphics::type::tfloat, motor::graphics::type_struct::vec2 )
                        .resize( ftm.get_num_vertices() ).update<vertex>( [&] ( vertex * array, size_t const ne )
                    {
                        for ( size_t i = 0; i < ne; ++i )
                        {
                            array[ i ].pos = ftm.get_vertex_position_3d( i ) ;
                            array[ i ].nrm = ftm.get_vertex_normal_3d( i ) ;
                            array[ i ].tx = ftm.get_vertex_texcoord( 0, i ) ;
                        }
                    } );

                    auto ib = motor::graphics::index_buffer_t().
                        set_layout_element( motor::graphics::type::tuint ).resize( ftm.indices.size() ).
                        update<uint_t>( [&] ( uint_t * array, size_t const ne )
                    {
                        for ( size_t i = 0; i < ne; ++i ) array[ i ] = ftm.indices[ i ] ;
                    } ) ;

                    geo_obj = motor::graphics::geometry_object_t( "cube",
                        motor::graphics::primitive_type::triangles, std::move( vb ), std::move( ib ) ) ;
                }
            }

            // image configuration
            {
                motor::graphics::image_t img = motor::graphics::image_t( motor::graphics::image_t::dims_t( 100, 100 ) )
                    .update( [&] ( motor::graphics::image_ptr_t, motor::graphics::image_t::dims_in_t dims, void_ptr_t data_in )
                {
                    typedef motor::math::vector4< uint8_t > rgba_t ;
                    auto * data = reinterpret_cast<rgba_t *>( data_in ) ;

                    size_t const w = 5 ;

                    size_t i = 0 ;
                    for ( size_t y = 0; y < dims.y(); ++y )
                    {
                        bool_t const odd = ( y / w ) & 1 ;

                        for ( size_t x = 0; x < dims.x(); ++x )
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

            // shader
            {
                motor::graphics::msl_object_t mslo( "render" ) ;

                motor::string_t shd ;
                db.load( motor::io::location_t( "shaders.15_wire_shader_slots.msl" ) ).wait_for_operation( [&] ( char_cptr_t data, size_t const sib, motor::io::result const )
                {

                    motor::log::global_t::status( "********************************" ) ;
                    motor::log::global_t::status( "loaded shader " + motor::from_std( std::to_string( sib ) ) + " bytes" ) ;

                    shd = motor::string_t( data, sib ) ;
                } ) ;

                mslo.add( motor::graphics::msl_api_type::msl_4_0, shd ) ;

                mslo.link_geometry( "cube" ) ;

                {
                    auto b = motor::shared( motor::graphics::wire_variable_bridge_t( mslo.get_varibale_set(0) ) ) ;
                    b->borrow_inputs()->add( "proj", motor::shared( motor::wire::input_slot< motor::math::mat4f_t >() ) ) ;
                    b->borrow_inputs()->add( "view", motor::shared( motor::wire::input_slot< motor::math::mat4f_t >() ) ) ;
                    b->borrow_inputs()->add( "u_color", motor::shared( motor::wire::input_slot< motor::math::vec4f_t >( motor::math::vec4f_t(1.0f)) ) ) ;
                    b->borrow_inputs()->add( "u_tex", motor::shared( motor::wire::input_slot< motor::graphics::texture_variable_data >( "checker_board" ) ) ) ;
                    b->update_bindings() ;
                    _bridges.emplace_back( motor::move( b ) ) ;
                }
                {
                    auto b = motor::shared( motor::graphics::wire_variable_bridge_t( mslo.get_varibale_set( 1 ) ) ) ;
                    b->borrow_inputs()->add( "proj", motor::shared( motor::wire::input_slot< motor::math::mat4f_t >() ) ) ;
                    b->borrow_inputs()->add( "view", motor::shared( motor::wire::input_slot< motor::math::mat4f_t >() ) ) ;
                    b->borrow_inputs()->add( "u_color", motor::shared( motor::wire::input_slot< motor::math::vec4f_t >( motor::math::vec4f_t( 1.0f ) ) ) ) ;
                    b->borrow_inputs()->add( "u_tex", motor::shared( motor::wire::input_slot< motor::graphics::texture_variable_data >( "checker_board" ) ) ) ;
                    b->update_bindings() ;
                    _bridges.emplace_back( motor::move( b ) ) ;
                }


                _msl_obj = motor::shared( std::move( mslo ) ) ;
            }
        }

        //************************************************************************************************
        virtual void_t on_event( window_id_t const wid,
            motor::application::window_message_listener::state_vector_cref_t sv ) noexcept
        {
            if ( sv.create_changed )
            {
                motor::log::global_t::status( "[my_app] : window created" ) ;
            }
            if ( sv.close_changed )
            {
                motor::log::global_t::status( "[my_app] : window closed" ) ;
                this->close() ;
            }
            if ( sv.resize_changed )
            {
                float_t const w = float_t( sv.resize_msg.w ) ;
                float_t const h = float_t( sv.resize_msg.h ) ;
                for ( size_t i = 0; i < 2; ++i )
                {
                    _cameras[ i ]->set_sensor_dims( w, h ) ;
                    _cameras[ i ]->perspective_fov() ;
                }
            }
        }

        //************************************************************************************************
        virtual void_t on_update( motor::application::app::update_data_in_t ) noexcept
        {
            mon->for_each_and_swap( [&] ( motor::io::location_cref_t loc, motor::io::monitor_t::notify const n )
            {
                motor::log::global_t::status( "[monitor] : Got " + motor::io::monitor_t::to_string( n ) + " for " + loc.as_string() ) ;

                motor::string_t shd ;
                db.load( loc ).wait_for_operation( [&] ( char_cptr_t data, size_t const sib, motor::io::result const )
                {
                    shd = motor::string_t( data, sib ) ;
                } ) ;

                _msl_obj->clear_shaders().add( motor::graphics::msl_api_type::msl_4_0, shd ) ;

                reconfigs.push_back( _msl_obj ) ;
            } ) ;
        }

        //************************************************************************************************
        virtual void_t on_graphics( motor::application::app::graphics_data_in_t ) noexcept 
        {
            if( _msl_obj->has_shader_changed() )
            {
                motor::graphics::shader_bindings_t sb ;
                if( _msl_obj->reset_and_successful( sb ) )
                {
                    motor::string_t name ;

                    for ( auto * b : _bridges )
                    {
                        if ( sb.has_variable_binding( motor::graphics::binding_point::projection_matrix, name ) )
                        {
                            auto s = b->borrow_inputs()->get_or_add( name,
                                motor::shared( motor::wire::input_slot< motor::math::mat4f_t >() ) ) ;

                            if ( !s->connect( motor::share( _proj ) ) )
                            {
                                motor::log::global::error( "slot type mismatch" ) ;
                            }
                        }

                        if ( sb.has_variable_binding( motor::graphics::binding_point::view_matrix, name ) )
                        {
                            auto s = b->borrow_inputs()->get_or_add( name,
                                motor::shared( motor::wire::input_slot< motor::math::mat4f_t >() ) ) ;

                            if ( !s->connect( motor::share( _view ) ) )
                            {
                                motor::log::global::error( "slot type mismatch" ) ;
                            }
                        }
                    }

                    

                    if ( sb.has_variable_binding( motor::graphics::binding_point::world_matrix, name ) )
                    {
                    }
                }

                for ( auto * b : _bridges )
                {
                    b->update_bindings() ;
                }

                renderables.emplace_back( _msl_obj ) ;
            }

            _proj->set_and_exchange( _cameras[ _cam_id ]->get_proj_matrix() ) ;
            _view->set_and_exchange( _cameras[ _cam_id ]->get_view_matrix() ) ;

            for ( auto * b : _bridges )
            {
                b->pull_data() ;
            }
        } 

        //************************************************************************************************
        virtual void_t on_render( this_t::window_id_t const wid, motor::graphics::gen4::frontend_ptr_t fe,
            motor::application::app::render_data_in_t rd ) noexcept
        {
            if ( rd.first_frame )
            {
                fe->configure<motor::graphics::geometry_object_t>( &geo_obj ) ;
                fe->configure<motor::graphics::image_object_t>( &img_obj ) ;
                fe->configure<motor::graphics::msl_object_t>( _msl_obj ) ;
            }

            for ( auto * obj : reconfigs )
            {
                fe->configure<motor::graphics::msl_object_t>( obj ) ;
            }

            for( auto * obj : renderables )
            {
                motor::graphics::gen4::backend_t::render_detail_t detail ;
                fe->render( obj, detail ) ;
            }
        }

        //************************************************************************************************
        virtual void_t on_frame_done( void_t ) noexcept
        {
            reconfigs.clear() ;
        }

        //******************************************************************************************************
        virtual bool_t on_tool( this_t::window_id_t const wid, motor::application::app::tool_data_ref_t ) noexcept 
        { 
            if ( ImGui::Begin( "Camera Window" ) )
            {
                {
                    int used_cam = int_t( _cam_id ) ;
                    ImGui::SliderInt( "Choose Camera", &used_cam, 0, 1 ) ;
                    _cam_id = std::min( size_t( used_cam ), size_t( 2 ) ) ;
                }

                {
                    auto const cam_pos = _cameras[ _cam_id ]->get_position() ;
                    float x = cam_pos.x() ;
                    float y = cam_pos.y() ;
                    ImGui::SliderFloat( "Cur Cam X", &x, -100.0f, 100.0f ) ;
                    ImGui::SliderFloat( "Cur Cam Y", &y, -100.0f, 100.0f ) ;
                    _cameras[ _cam_id ]->translate_to( motor::math::vec3f_t( x, y, cam_pos.z() ) ) ;

                }
            }
            ImGui::End() ;
            
            return true ; 
        }

        //************************************************************************************************
        virtual void_t on_shutdown( void_t ) noexcept 
        {
            reconfigs.clear() ;
            renderables.clear() ;

            for( auto * b : _bridges )
            {
                motor::release( motor::move( b ) ) ;
            }

            motor::release( motor::move( _msl_obj ) ) ;
            motor::release( motor::move( _cameras[0] ) ) ;
            motor::release( motor::move( _cameras[1] ) ) ;
            motor::release( motor::move( _proj ) ) ;
            motor::release( motor::move( _view ) ) ;

            db.detach( motor::move( mon )  ) ;
        } ;
    };
}

int main( int argc, char ** argv )
{
    using namespace motor::core::types ;

    motor::application::carrier_mtr_t carrier = motor::platform::global_t::create_carrier(
        motor::shared( this_file::my_app() ) ) ;

    auto const ret = carrier->exec() ;

    motor::memory::release_ptr( carrier ) ;

    motor::io::global_t::deinit() ;
    motor::concurrent::global::deinit() ;
    motor::log::global::deinit() ;
    motor::memory::global::dump_to_std() ;


    return ret ;
}