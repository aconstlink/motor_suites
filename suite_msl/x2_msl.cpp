
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
    using namespace natus::core::types ;

    class test_app : public natus::application::app
    {
        natus_this_typedefs( test_app ) ;

    private:

        app::window_async_t _wid_async ;
        
        natus::graphics::state_object_res_t _root_render_states ;
        natus::graphics::render_object_res_t _rc = natus::graphics::render_object_t() ;
        natus::graphics::geometry_object_res_t _gconfig = natus::graphics::geometry_object_t() ;
        natus::graphics::image_object_res_t _imgconfig = natus::graphics::image_object_t() ;

        struct vertex { natus::math::vec3f_t pos ; natus::math::vec2f_t tx ; } ;
        
        typedef ::std::chrono::high_resolution_clock __clock_t ;
        __clock_t::time_point _tp = __clock_t::now() ;

        natus::gfx::pinhole_camera_t _camera_0 ;

        natus::io::database_res_t _db ;
        natus::nsl::database_res_t _ndb ;

    public:

        test_app( void_t ) 
        {
            natus::application::app::window_info_t wi ;
            _wid_async = this_t::create_window( "A Render Window", wi, { 
                natus::graphics::backend_type::d3d11, 
                natus::graphics::backend_type::es3, 
                natus::graphics::backend_type::gl4 } ) ;
            _db = natus::io::database_t( natus::io::path_t( DATAPATH ), "./working", "data" ) ;
            _ndb = natus::nsl::database_t() ;
        }
        test_app( this_cref_t ) = delete ;
        test_app( this_rref_t rhv ) : app( ::std::move( rhv ) ) 
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

        virtual natus::application::result on_init( void_t ) noexcept
        { 
            {
                _camera_0.set_dims( 2.0f, 2.0f, 1.0f, 1000.0f ) ;
                _camera_0.orthographic() ;
            }

            // root render states
            {
                natus::graphics::state_object_t so = natus::graphics::state_object_t(
                    "root_render_states" ) ;


                {
                    natus::graphics::render_state_sets_t rss ;
                    rss.depth_s.do_change = true ;
                    rss.depth_s.ss.do_activate = false ;
                    rss.depth_s.ss.do_depth_write = false ;
                    rss.polygon_s.do_change = true ;
                    rss.polygon_s.ss.do_activate = true ;
                    rss.polygon_s.ss.ff = natus::graphics::front_face::clock_wise ;
                    rss.polygon_s.ss.cm = natus::graphics::cull_mode::back ;
                    so.add_render_state_set( rss ) ;
                }

                _root_render_states = std::move( so ) ;
                _wid_async.async().configure( _root_render_states ) ;
            }

            // geometry configuration
            {
                auto vb = natus::graphics::vertex_buffer_t()
                    .add_layout_element( natus::graphics::vertex_attribute::position, natus::graphics::type::tfloat, natus::graphics::type_struct::vec3 )
                    .add_layout_element( natus::graphics::vertex_attribute::texcoord0, natus::graphics::type::tfloat, natus::graphics::type_struct::vec2 )
                    .resize( 4 ).update<vertex>( [=] ( vertex* array, size_t const ne )
                {
                    array[ 0 ].pos = natus::math::vec3f_t( -0.5f, -0.5f, 0.0f ) ;
                    array[ 1 ].pos = natus::math::vec3f_t( -0.5f, +0.5f, 0.0f ) ;
                    array[ 2 ].pos = natus::math::vec3f_t( +0.5f, +0.5f, 0.0f ) ;
                    array[ 3 ].pos = natus::math::vec3f_t( +0.5f, -0.5f, 0.0f ) ;

                    array[ 0 ].tx = natus::math::vec2f_t( -0.0f, -0.0f ) ;
                    array[ 1 ].tx = natus::math::vec2f_t( -0.0f, +1.0f ) ;
                    array[ 2 ].tx = natus::math::vec2f_t( +1.0f, +1.0f ) ;
                    array[ 3 ].tx = natus::math::vec2f_t( +1.0f, -0.0f ) ;
                } );

                auto ib = natus::graphics::index_buffer_t().
                    set_layout_element( natus::graphics::type::tuint ).resize( 6 ).
                    update<uint_t>( [] ( uint_t* array, size_t const ne )
                {
                    array[ 0 ] = 0 ;
                    array[ 1 ] = 1 ;
                    array[ 2 ] = 2 ;

                    array[ 3 ] = 0 ;
                    array[ 4 ] = 2 ;
                    array[ 5 ] = 3 ;
                } ) ;

                _gconfig = natus::graphics::geometry_object_t( "quad",
                    natus::graphics::primitive_type::triangles, ::std::move( vb ), ::std::move( ib ) ) ;

                _wid_async.async().configure( _gconfig ) ;
            }

            // image configuration
            {
                natus::format::module_registry_res_t mod_reg = natus::format::global_t::registry() ;
                auto fitem = mod_reg->import_from( natus::io::location_t( "images.checker.png" ), _db ) ;
                natus::format::image_item_res_t ii = fitem.get() ;
                if( ii.is_valid() )
                {
                    natus::graphics::image_t img = *ii->img ;

                    _imgconfig = natus::graphics::image_object_t( "loaded_image", std::move( img ) )
                        .set_wrap( natus::graphics::texture_wrap_mode::wrap_s, natus::graphics::texture_wrap_type::repeat )
                        .set_wrap( natus::graphics::texture_wrap_mode::wrap_t, natus::graphics::texture_wrap_type::repeat )
                        .set_filter( natus::graphics::texture_filter_mode::min_filter, natus::graphics::texture_filter_type::nearest )
                        .set_filter( natus::graphics::texture_filter_mode::mag_filter, natus::graphics::texture_filter_type::nearest );
                }
                

                _wid_async.async().configure( _imgconfig ) ;
            }

            {
                natus::format::module_registry_res_t mod_reg = natus::format::global_t::registry() ;
                auto fitem1 = mod_reg->import_from( natus::io::location_t( "shaders.mylib.nsl" ), _db ) ;
                auto fitem2 = mod_reg->import_from( natus::io::location_t( "shaders.myshader.nsl" ), _db ) ;

                // do the lib
                {
                    natus::format::nsl_item_res_t ii = fitem1.get() ;
                    if( ii.is_valid() ) _ndb->insert( std::move( std::move( ii->doc ) ) ) ;
                }

                // do the config
                {
                    natus::format::nsl_item_res_t ii = fitem2.get() ;
                    if( ii.is_valid() ) _ndb->insert( std::move( std::move( ii->doc ) ) ) ;
                }
               
                {
                    natus::nsl::generatable_t res = natus::nsl::dependency_resolver_t().resolve( 
                        _ndb, natus::nsl::symbol( "myconfig" ) ) ;

                    if( res.missing.size() != 0 )
                    {
                        natus::log::global_t::warning( "We have missing symbols." ) ;
                        for( auto const& s : res.missing )
                        {
                            natus::log::global_t::status( s.expand() ) ;
                        }
                    }

                    auto const sc = natus::graphics::nsl_bridge_t().create( 
                        natus::nsl::generator_t( std::move( res ) ).generate() ).set_name("quad") ;

                    _wid_async.async().configure( sc ) ;
                }
            }

            natus::graphics::render_object_t rc = natus::graphics::render_object_t( "quad" ) ;

            {
                rc.link_geometry( "quad" ) ;
                rc.link_shader( "quad" ) ;
            }

            // add variable set 0
            {
                natus::graphics::variable_set_res_t vars = natus::graphics::variable_set_t() ;
                {
                    auto* var = vars->data_variable< natus::math::vec4f_t >( "some_color" ) ;
                    var->set( natus::math::vec4f_t( 0.0f, 1.0f, 0.5f, 1.0f ) ) ;
                }
                {
                    auto * var = vars->texture_variable( "some_texture" ) ;
                    var->set( "loaded_image" ) ;
                }

                rc.add_variable_set( ::std::move( vars ) ) ;
            }
            
            _rc = std::move( rc ) ;
            _wid_async.async().configure( _rc ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_update( natus::application::app_t::update_data_in_t ) noexcept 
        { 
            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t ) noexcept 
        { 
            _wid_async.async().push( _root_render_states ) ;

            // per frame update of variables
            _rc->for_each( [&] ( size_t const i, natus::graphics::variable_set_res_t const& vs )
            {
                {
                    auto* var = vs->data_variable< natus::math::mat4f_t >( "view" ) ;
                    var->set( _camera_0.mat_view() ) ;
                }

                {
                    auto* var = vs->data_variable< natus::math::mat4f_t >( "proj" ) ;
                    var->set( _camera_0.mat_proj() ) ;
                }
            } ) ;

            {
                natus::graphics::backend_t::render_detail_t detail ;
                detail.start = 0 ;
                _wid_async.async().render( _rc, detail ) ;
            }

            _wid_async.async().pop( natus::graphics::backend::pop_type::render_state ) ;
            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_shutdown( void_t ) noexcept 
        { return natus::application::result::ok ; }
    };
    natus_res_typedef( test_app ) ;
}

int main( int argc, char ** argv )
{
    return natus::application::global_t::create_and_exec_application( 
        this_file::test_app_res_t( this_file::test_app_t() ) ) ;
}
