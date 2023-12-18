
#include "main.h"

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/gfx/camera/pinhole_camera.h>
#include <natus/graphics/variable/variable_set.hpp>
#include <natus/graphics/shader/nsl_bridge.hpp>

#include <natus/format/global.h>
#include <natus/format/nsl/nsl_module.h>
#include <natus/format/future_items.hpp>

#include <natus/io/database.h>
#include <natus/nsl/database.hpp>
#include <natus/nsl/dependency_resolver.hpp>
#include <natus/nsl/generator_structs.hpp>

#include <natus/math/vector/vector3.hpp>
#include <natus/math/vector/vector4.hpp>
#include <natus/math/matrix/matrix4.hpp>

#include <thread>

//
// This project tests 
// - importing nsl files (format)
// - parsing and generating glsl code (nsl)
// - using the generated code for rendering (graphics)
//

namespace this_file
{
    using namespace motor::core::types ;

    class test_app : public motor::application::app
    {
        motor_this_typedefs( test_app ) ;

    private:

        app::window_async_t _wid_async ;
        
        motor::graphics::state_object_res_t _root_render_states ;
        motor::graphics::render_object_res_t _rc = motor::graphics::render_object_t() ;
        motor::graphics::geometry_object_res_t _gconfig = motor::graphics::geometry_object_t() ;
        motor::graphics::image_object_res_t _imgconfig = motor::graphics::image_object_t() ;

        struct vertex { motor::math::vec3f_t pos ; motor::math::vec2f_t tx ; } ;
        
        typedef std::chrono::high_resolution_clock __clock_t ;
        __clock_t::time_point _tp = __clock_t::now() ;

        motor::gfx::pinhole_camera_t _camera_0 ;

        motor::io::database_res_t _db ;
        motor::nsl::database_res_t _ndb ;

    public:

        test_app( void_t ) 
        {
            motor::application::app::window_info_t wi ;
            _wid_async = this_t::create_window( "A Render Window", wi, { 
                motor::graphics::backend_type::d3d11, 
                motor::graphics::backend_type::es3, 
                motor::graphics::backend_type::gl4 } ) ;
            _db = motor::io::database_t( motor::io::path_t( DATAPATH ), "./working", "data" ) ;
            _ndb = motor::nsl::database_t() ;
        }
        test_app( this_cref_t ) = delete ;
        test_app( this_rref_t rhv ) : app( std::move( rhv ) ) 
        {
            _wid_async = std::move( rhv._wid_async ) ;
            _camera_0 = std::move( rhv._camera_0 ) ;
            _gconfig = std::move( rhv._gconfig ) ;
            _rc = std::move( rhv._rc) ;
            _db = std::move( rhv._db ) ;
            _ndb = std::move( rhv._ndb ) ;
            _imgconfig = std::move( rhv._imgconfig ) ;
        }
        virtual ~test_app( void_t ) 
        {}

    private:

        virtual motor::application::result on_init( void_t ) noexcept
        { 
            {
                _camera_0.set_dims( 2.0f, 2.0f, 1.0f, 1000.0f ) ;
                _camera_0.orthographic() ;
            }

            // root render states
            {
                motor::graphics::state_object_t so = motor::graphics::state_object_t(
                    "root_render_states" ) ;


                {
                    motor::graphics::render_state_sets_t rss ;
                    rss.depth_s.do_change = true ;
                    rss.depth_s.ss.do_activate = false ;
                    rss.depth_s.ss.do_depth_write = false ;
                    rss.polygon_s.do_change = true ;
                    rss.polygon_s.ss.do_activate = true ;
                    rss.polygon_s.ss.ff = motor::graphics::front_face::clock_wise ;
                    rss.polygon_s.ss.cm = motor::graphics::cull_mode::back ;
                    so.add_render_state_set( rss ) ;
                }

                _root_render_states = std::move( so ) ;
                _wid_async.async().configure( _root_render_states ) ;
            }

            // geometry configuration
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

                _gconfig = motor::graphics::geometry_object_t( "quad",
                    motor::graphics::primitive_type::triangles, std::move( vb ), std::move( ib ) ) ;

                _wid_async.async().configure( _gconfig ) ;
            }

            // image configuration
            {
                motor::format::module_registry_res_t mod_reg = motor::format::global_t::registry() ;
                auto fitem = mod_reg->import_from( motor::io::location_t( "images.checker.png" ), _db ) ;
                motor::format::image_item_res_t ii = fitem.get() ;
                if( ii.is_valid() )
                {
                    motor::graphics::image_t img = *ii->img ;

                    _imgconfig = motor::graphics::image_object_t( "loaded_image", std::move( img ) )
                        .set_wrap( motor::graphics::texture_wrap_mode::wrap_s, motor::graphics::texture_wrap_type::repeat )
                        .set_wrap( motor::graphics::texture_wrap_mode::wrap_t, motor::graphics::texture_wrap_type::repeat )
                        .set_filter( motor::graphics::texture_filter_mode::min_filter, motor::graphics::texture_filter_type::nearest )
                        .set_filter( motor::graphics::texture_filter_mode::mag_filter, motor::graphics::texture_filter_type::nearest );
                }
                

                _wid_async.async().configure( _imgconfig ) ;
            }

            {
                motor::format::module_registry_res_t mod_reg = motor::format::global_t::registry() ;
                auto fitem1 = mod_reg->import_from( motor::io::location_t( "shaders.mylib.nsl" ), _db ) ;
                auto fitem2 = mod_reg->import_from( motor::io::location_t( "shaders.myshader.nsl" ), _db ) ;

                // do the lib
                {
                    motor::format::nsl_item_res_t ii = fitem1.get() ;
                    if( ii.is_valid() ) _ndb->insert( std::move( std::move( ii->doc ) ) ) ;
                }

                // do the config
                {
                    motor::format::nsl_item_res_t ii = fitem2.get() ;
                    if( ii.is_valid() ) _ndb->insert( std::move( std::move( ii->doc ) ) ) ;
                }
               
                {
                    motor::nsl::generatable_t res = motor::nsl::dependency_resolver_t().resolve( 
                        _ndb, motor::nsl::symbol( "myconfig" ) ) ;

                    if( res.missing.size() != 0 )
                    {
                        motor::log::global_t::warning( "We have missing symbols." ) ;
                        for( auto const& s : res.missing )
                        {
                            motor::log::global_t::status( s.expand() ) ;
                        }
                    }

                    auto const sc = motor::graphics::nsl_bridge_t().create( 
                        motor::nsl::generator_t( std::move( res ) ).generate() ).set_name("quad") ;

                    _wid_async.async().configure( sc ) ;
                }
            }

            motor::graphics::render_object_t rc = motor::graphics::render_object_t( "quad" ) ;

            {
                rc.link_geometry( "quad" ) ;
                rc.link_shader( "quad" ) ;
            }

            // add variable set 0
            {
                motor::graphics::variable_set_res_t vars = motor::graphics::variable_set_t() ;
                {
                    auto* var = vars->data_variable< motor::math::vec4f_t >( "some_color" ) ;
                    var->set( motor::math::vec4f_t( 0.0f, 1.0f, 0.5f, 1.0f ) ) ;
                }
                {
                    auto * var = vars->texture_variable( "some_texture" ) ;
                    var->set( "loaded_image" ) ;
                }

                rc.add_variable_set( std::move( vars ) ) ;
            }
            
            _rc = std::move( rc ) ;
            _wid_async.async().configure( _rc ) ;

            return motor::application::result::ok ; 
        }

        virtual motor::application::result on_update( motor::application::app_t::update_data_in_t ) noexcept 
        { 
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) ) ;

            return motor::application::result::ok ; 
        }

        virtual motor::application::result on_graphics( motor::application::app_t::render_data_in_t ) noexcept 
        { 
            _wid_async.async().push( _root_render_states ) ;

            // per frame update of variables
            _rc->for_each( [&] ( size_t const i, motor::graphics::variable_set_res_t const& vs )
            {
                {
                    auto* var = vs->data_variable< motor::math::mat4f_t >( "view" ) ;
                    var->set( _camera_0.mat_view() ) ;
                }

                {
                    auto* var = vs->data_variable< motor::math::mat4f_t >( "proj" ) ;
                    var->set( _camera_0.mat_proj() ) ;
                }
            } ) ;

            {
                motor::graphics::backend_t::render_detail_t detail ;
                detail.start = 0 ;
                _wid_async.async().render( _rc, detail ) ;
            }

            _wid_async.async().pop( motor::graphics::backend::pop_type::render_state ) ;
            return motor::application::result::ok ; 
        }

        virtual motor::application::result on_shutdown( void_t ) noexcept 
        { return motor::application::result::ok ; }
    };
    motor_res_typedef( test_app ) ;
}

int main( int argc, char ** argv )
{
    return motor::application::global_t::create_and_exec_application( 
        this_file::test_app_res_t( this_file::test_app_t() ) ) ;
}
