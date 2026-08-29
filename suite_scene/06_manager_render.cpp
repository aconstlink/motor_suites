

#include <motor/profiling/probe_guard.hpp>

#include <motor/platform/global.h>

#include <motor/geometry/mesh/tri_mesh.h>
#include <motor/geometry/mesh/flat_tri_mesh.h>
#include <motor/geometry/3d/cube.h>
#include <motor/geometry/3d/tetra.h>

#include <motor/controls/types/ascii_keyboard.hpp>
#include <motor/controls/types/three_mouse.hpp>

#include <motor/gfx/manager/msl_manager.h>
#include <motor/gfx/camera/generic_camera.h>
#include <motor/math/utility/angle.hpp>

#include <motor/scene/node/logic_group.h>
#include <motor/scene/node/logic_leaf.h>

#include <motor/scene/component/name_component.hpp>
#include <motor/scene/component/graphics/geometry_name_component.hpp>
#include <motor/scene/component/graphics/msl_component.h>
#include <motor/scene/component/graphics/render_settings_component.hpp>
#include <motor/scene/component/trafo3d_component.h>
#include <motor/scene/component/camera_component.h>

#include <motor/scene/visitor/variable_update_visitor.h>
#include <motor/scene/visitor/trafo_visitor.h>
#include <motor/scene/visitor/graphics/render_visitor.h>
#include <motor/scene/visitor/graphics/add_msl_to_set_visitor.hpp>

#include <motor/tool/imgui/node_kit/imgui_node_visitor.h>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>

#include <future>

namespace this_file
{
using namespace motor::core::types;

class my_app : public motor::application::app
{
    motor_this_typedefs( my_app );

    motor::math::vec4ui_t fb_dims = motor::math::vec4ui_t( 0, 0, 1920, 1080 );

    motor::scene::node_mtr_t _root;
    motor::scene::node_mtr_t _selected = nullptr;

    motor::graphics::state_object_mtr_t root_so;

    motor::graphics::geometry_object_t geo_obj1;
    motor::graphics::geometry_object_t geo_obj2;

    motor::gfx::generic_camera_mtr_t _camera;

    motor::gfx::msl_manager_mtr_t _mslm;

    bool_t _msls_are_init = false;
    bool_t _button_pressed = true;

    void_t init_manager_shaders( motor::gfx::msl_manager_mtr_t mgr ) noexcept
    {
        {
            motor::string_t shd = R"(
                config render_config_0
                {
                    vertex_shader
                    {
                        mat4_t proj : projection ;
                        mat4_t view : view ;
                        mat4_t world : world ;

                        in vec3_t pos : position ;
                        in vec3_t nrm : normal ;
                        in vec2_t tx : texcoord ;

                        out vec4_t pos : position ;
                        out vec2_t tx : texcoord ;
                        out vec3_t nrm : normal ;

                        void main()
                        {
                            vec3_t pos = in.pos ;
                            pos.xyz = pos.xyz * 10.0 ;
                            out.tx = in.tx ;
                            out.pos = proj * view * world * vec4_t( pos, 1.0 ) ;
                            out.nrm = normalize( world * vec4_t( in.nrm, 0.0 ) ).xyz ;
                        }
                    }

                    pixel_shader
                    {
                        tex2d_t tex ;
                        vec4_t color ;

                        in vec2_t tx : texcoord ;
                        in vec3_t nrm : normal ;
                        out vec4_t color0 : color0 ;

                        void main()
                        {
                            float_t light = dot( normalize( in.nrm ), normalize( vec3_t( 1.0, 1.0, 0.5) ) ) ;
                            out.color0 = vec4_t( light, light, light, 1.0 ) ;
                            out.color0 = out.color0 ' vec4_t( color.xyz, 1.0 ) ;
                        }
                    }
                })";

            mgr->add( "shader_0", shd );
            // variables are set when the shader is done.
            // @see on_update
        }

        {
            motor::string_t shd = R"(
                config render_config_1
                {
                    vertex_shader
                    {
                        mat4_t proj : projection ;
                        mat4_t view : view ;
                        mat4_t world : world ;

                        in vec3_t pos : position ;
                        in vec3_t nrm : normal ;
                        in vec2_t tx : texcoord ;

                        out vec4_t pos : position ;
                        out vec2_t tx : texcoord ;
                        out vec3_t nrm : normal ;

                        void main()
                        {
                            vec3_t pos = in.pos ;
                            pos.xyz = pos.xyz * 10.0 ;
                            out.tx = in.tx ;
                            out.pos = proj * view * world * vec4_t( pos, 1.0 ) ;
                            out.nrm = normalize( world * vec4_t( in.nrm, 0.0 ) ).xyz ;
                        }
                    }

                    pixel_shader
                    {
                        tex2d_t tex ;
                        vec4_t color ;

                        in vec2_t tx : texcoord ;
                        in vec3_t nrm : normal ;
                        out vec4_t color0 : color0 ;

                        void main()
                        {
                            float_t light = dot( normalize( in.nrm ), normalize( vec3_t( 1.0, 1.0, 0.5) ) ) ;
                            out.color0 = vec4_t( light, light, light, 1.0 ) ;
                            out.color0 = out.color0 ' vec4_t( vec3_t(1.0,1.0,1.0) - color.xyz, 1.0 ) ;
                        }
                    }
                })";

            mgr->add( "shader_1", shd );
            // variables are set when the shader is done.
            // @see on_update
        }
    }

    void_t release_manager_shaders( void_t ) noexcept {}

    //******************************************************************************************************
    virtual void_t on_init( void_t ) noexcept
    {
        MOTOR_PROBE( "application", "on_init" );

        // #1 : init window
        {
            motor::application::window_info_t wi;
            wi.x = 100;
            wi.y = 100;
            wi.w = 800;
            wi.h = 600;
            wi.gen = motor::application::graphics_generation::gen4_auto;

            this_t::send_window_message( this_t::create_window( wi ),
                [ & ]( motor::application::app::window_view & wnd )
            {
                wnd.send_message( motor::application::show_message( { true } ) );
                wnd.send_message( motor::application::cursor_message_t( { true } ) );
                wnd.send_message( motor::application::vsync_message_t( { true } ) );
            } );
        }

        // camera
        {
            auto cam = motor::gfx::generic_camera_t( 1.0f, 1.0f, 1.0f, 100.0f );
            cam.perspective_fov( motor::math::angle< float_t >::degree_to_radian( 45.0f ) );
            cam.look_at( motor::math::vec3f_t( 0.0f, 50.0f, 80.0f ),
                motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ),
                motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) );

            _camera = motor::shared( std::move( cam ) );
        }

        // # : make geometry
        {
            struct vertex
            {
                motor::math::vec3f_t pos;
                motor::math::vec3f_t nrm;
                motor::math::vec2f_t tx;
            };

            // cube
            {
                motor::geometry::cube_t::input_params ip;
                ip.scale = motor::math::vec3f_t( 1.0f );
                ip.tess = 100;

                motor::geometry::tri_mesh_t tm;
                motor::geometry::cube_t::make( &tm, ip );

                motor::geometry::flat_tri_mesh_t ftm;
                tm.flatten( ftm );

                auto vb =
                    motor::graphics::vertex_buffer_t()
                        .add_layout_element( motor::graphics::vertex_attribute::position,
                            motor::graphics::type::tfloat, motor::graphics::type_struct::vec3 )
                        .add_layout_element( motor::graphics::vertex_attribute::normal,
                            motor::graphics::type::tfloat, motor::graphics::type_struct::vec3 )
                        .add_layout_element( motor::graphics::vertex_attribute::texcoord0,
                            motor::graphics::type::tfloat, motor::graphics::type_struct::vec2 )
                        .resize( ftm.get_num_vertices() )
                        .update< vertex >( [ & ]( vertex * array, size_t const ne )
                {
                    for( size_t i = 0; i < ne; ++i )
                    {
                        array[ i ].pos = ftm.get_vertex_position_3d( i );
                        array[ i ].nrm = ftm.get_vertex_normal_3d( i );
                        array[ i ].tx = ftm.get_vertex_texcoord( 0, i );
                    }
                } );

                auto ib = motor::graphics::index_buffer_t()
                              .set_layout_element( motor::graphics::type::tuint )
                              .resize( ftm.indices.size() )
                              .update< uint_t >( [ & ]( uint_t * array, size_t const ne )
                {
                    for( size_t i = 0; i < ne; ++i ) array[ i ] = ftm.indices[ i ];
                } );

                geo_obj1 = motor::graphics::geometry_object_t( "cube",
                    motor::graphics::primitive_type::triangles, std::move( vb ), std::move( ib ) );
            }
        }

        // # : make geometry
        {
            struct vertex
            {
                motor::math::vec3f_t pos;
                motor::math::vec3f_t nrm;
                motor::math::vec2f_t tx;
            };

            // tetra
            {
                motor::geometry::tetra_t::input_params ip;
                ip.scale = motor::math::vec3f_t( 1.0f );

                motor::geometry::polygon_mesh pm;
                motor::geometry::tetra::make( &pm, ip );

                motor::geometry::flat_tri_mesh_t ftm;
                pm.flatten( ftm );

                auto vb =
                    motor::graphics::vertex_buffer_t()
                        .add_layout_element( motor::graphics::vertex_attribute::position,
                            motor::graphics::type::tfloat, motor::graphics::type_struct::vec3 )
                        .add_layout_element( motor::graphics::vertex_attribute::normal,
                            motor::graphics::type::tfloat, motor::graphics::type_struct::vec3 )
                        .add_layout_element( motor::graphics::vertex_attribute::texcoord0,
                            motor::graphics::type::tfloat, motor::graphics::type_struct::vec2 )
                        .resize( ftm.get_num_vertices() )
                        .update< vertex >( [ & ]( vertex * array, size_t const ne )
                {
                    for( size_t i = 0; i < ne; ++i )
                    {
                        array[ i ].pos = ftm.get_vertex_position_3d( i );
                        array[ i ].nrm = ftm.get_vertex_normal_3d( i );
                        array[ i ].tx = ftm.get_vertex_texcoord( 0, i );
                    }
                } );

                auto ib = motor::graphics::index_buffer_t()
                              .set_layout_element( motor::graphics::type::tuint )
                              .resize( ftm.indices.size() )
                              .update< uint_t >( [ & ]( uint_t * array, size_t const ne )
                {
                    for( size_t i = 0; i < ne; ++i ) array[ i ] = ftm.indices[ i ];
                } );

                geo_obj2 = motor::graphics::geometry_object_t( "tetra",
                    motor::graphics::primitive_type::triangles, std::move( vb ), std::move( ib ) );
            }
        }

        // manager stuff
        {
            motor::io::database_t db =
                motor::io::database_t( motor::io::path_t( DATAPATH ), "./working", "data" );
            motor::gfx::msl_manager_t mgr( motor::shared( std::move( db ) ) );

            ///this_t::init_manager_shaders( &mgr );

            _mslm = motor::shared( std::move( mgr ) );
        }

        {
            motor::graphics::state_object_t so =
                motor::graphics::state_object_t( "root_render_states" );

            {
                motor::graphics::render_state_sets_t rss;
                rss.depth_s.do_change = true;
                rss.depth_s.ss.do_activate = false;
                rss.depth_s.ss.do_depth_write = true;
                rss.polygon_s.do_change = true;
                rss.polygon_s.ss.do_activate = true;
                rss.polygon_s.ss.ff = motor::graphics::front_face::clock_wise;
                rss.polygon_s.ss.cm = motor::graphics::cull_mode::back;
                rss.clear_s.do_change = true;
                rss.clear_s.ss.clear_color = motor::math::vec4f_t( 0.5f, 0.9f, 0.5f, 1.0f );
                rss.clear_s.ss.do_activate = true;
                rss.clear_s.ss.do_color_clear = true;
                rss.clear_s.ss.do_depth_clear = true;
                rss.view_s.do_change = true;
                rss.view_s.ss.do_activate = false;
                rss.view_s.ss.vp = motor::math::vec4ui_t( 0, 0, 500, 500 );
                so.add_render_state_set( rss );
            }

            root_so = motor::shared( motor::graphics::state_object_t( std::move( so ) ) );
        }

        // #3 : init scene tree
        {
            motor::scene::logic_group_t root;
            root.add_component( motor::shared( motor::scene::name_component_t( "my root name" ) ) );

            // add camera
            {
                auto cam_comp = motor::scene::camera_component_t(
                    motor::shared( motor::gfx::generic_camera_t( 800.0f, 600.0f, 1.0f, 100.0f ) ) );
                root.add_component( motor::shared( std::move( cam_comp ) ) );
            }

            {
                auto t = motor::shared( motor::scene::logic_group_t() );
                {
                    motor::scene::trafo3d_component_t tc(
                        motor::math::m3d::trafof_t( motor::math::vec3f_t( 1.0f, 1.0f, 1.0f ),
                            motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ),
                            motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) ) );

                    t->add_component(
                        motor::shared( motor::scene::name_component_t( "trafo node 1" ) ) );
                    t->add_component( motor::shared( std::move( tc ) ) );
                }

                {
                    auto rs = motor::shared( motor::scene::logic_group_t() );

                    // add render settings
                    {
                        motor::scene::render_settings_component_t rsc( motor::share( root_so ) );
                        rs->add_component( motor::shared( std::move( rsc ) ) );
                    }

                    // render object 1
                    {
                        auto rn = motor::scene::logic_leaf_t();
                        {
                            motor::scene::trafo3d_component_t tc( motor::math::m3d::trafof_t(
                                motor::math::vec3f_t( 1.0f, 1.0f, 1.0f ),
                                motor::math::vec3f_t( .0f, 0.0f, 0.0f ),
                                motor::math::vec3f_t( -10.0f, 0.0f, 0.0f ) ) );

                            rn.add_component( motor::shared( std::move( tc ) ) );

                            rn.add_component( motor::shared(
                                motor::scene::name_component_t( "Render Object 0" ) ) );
                        }

                        // add geometry name ref so the run-time can link that
                        // geometry to the new msl in the future.
                        {
                            motor::scene::geometry_name_component_t gn( "cube" );
                            rn.add_component( motor::shared( std::move( gn ) ) );
                        }

                        // add empty msl set component so the visitor can
                        // just add a msl component to the set
                        {
                            auto mslset_comp = motor::scene::msl_set_component_t();
                            rn.add_component( motor::shared( std::move( mslset_comp ) ) );
                        }

                        rs->add_child( motor::shared( std::move( rn ) ) );
                    }

                    // render object 2
                    {
                        auto rn = motor::scene::logic_leaf_t();
                        {
                            rn.add_component( motor::shared(
                                motor::scene::name_component_t( "Render Object 1" ) ) );

                            motor::scene::trafo3d_component_t tc( motor::math::m3d::trafof_t(
                                motor::math::vec3f_t( 1.0f, 1.0f, 1.0f ),
                                motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ),
                                motor::math::vec3f_t( 10.0f, 0.0f, 0.0f ) ) );

                            rn.add_component( motor::shared( std::move( tc ) ) );
                        }

                        // add geometry name ref so the run-time can link that
                        // geometry to the new msl in the future.
                        {
                            motor::scene::geometry_name_component_t gn( "tetra" );
                            rn.add_component( motor::shared( std::move( gn ) ) );
                        }

                        // add empty msl set component so the visitor can
                        // just add a msl component to the set
                        {
                            auto mslset_comp = motor::scene::msl_set_component_t();
                            rn.add_component( motor::shared( std::move( mslset_comp ) ) );
                        }
                        rs->add_child( motor::shared( std::move( rn ) ) );
                    }

                    // render object 3
                    {
                        auto rn = motor::scene::logic_leaf_t();
                        {
                            rn.add_component( motor::shared(
                                motor::scene::name_component_t( "Render Object 2" ) ) );

                            motor::scene::trafo3d_component_t tc( motor::math::m3d::trafof_t(
                                motor::math::vec3f_t( 1.0f, 1.0f, 1.0f ),
                                motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ),
                                motor::math::vec3f_t( 10.0f, 0.0f, -50.0f ) ) );

                            rn.add_component( motor::shared( std::move( tc ) ) );
                        }

                        // add geometry name ref so the run-time can link that
                        // geometry to the new msl in the future.
                        {
                            motor::scene::geometry_name_component_t gn( "tetra" );
                            rn.add_component( motor::shared( std::move( gn ) ) );
                        }

                        // add empty msl set component so the visitor can
                        // just add a msl component to the set
                        {
                            auto mslset_comp = motor::scene::msl_set_component_t();
                            rn.add_component( motor::shared( std::move( mslset_comp ) ) );
                        }
                        rs->add_child( motor::shared( std::move( rn ) ) );
                    }

                    t->add_child( motor::move( rs ) );
                }

                root.add_child( motor::move( t ) );
            }

            _root = motor::shared( std::move( root ) );
        }
    }

    //******************************************************************************************************
    virtual void_t on_event( window_id_t const wid,
        motor::application::window_message_listener::state_vector_cref_t sv ) noexcept
    {
        MOTOR_PROBE( "application", "on_event" );

        if( sv.create_changed )
        {
            motor::log::global_t::status( "[my_app] : window created" );
        }
        if( sv.close_changed )
        {
            motor::log::global_t::status( "[my_app] : window closed" );
            this->close();
        }
        if( sv.resize_changed )
        {
            float_t const w = float_t( sv.resize_msg.w );
            float_t const h = float_t( sv.resize_msg.h );
            _camera->set_sensor_dims( w, h );
            _camera->perspective_fov();
        }
    }

    //******************************************************************************************************
    virtual void_t on_render( this_t::window_id_t const wid,
        motor::graphics::gen4::frontend_ptr_t fe,
        motor::application::app::render_data_in_t rd ) noexcept
    {
        _mslm->on_render( fe );

        if( _msls_are_init && _button_pressed )
        {
            _mslm->on_render_release( fe );
            _button_pressed = false;
            _msls_are_init = false;
        }

        if( !_msls_are_init && _button_pressed )
        {
            this_t::init_manager_shaders( _mslm ) ;
            _mslm->on_render_init( fe ) ;
            _msls_are_init = true ;
            _button_pressed = false;
        }

        // configure needs to be done only once per window
        if( rd.first_frame )
        {
            fe->configure< motor::graphics::state_object_t >( root_so );
            fe->configure< motor::graphics::geometry_object_t >( &geo_obj1 );
            fe->configure< motor::graphics::geometry_object_t >( &geo_obj2 );
        }

        {
            motor::scene::render_visitor_t vis( 0, fe, _camera );
            motor::scene::node_t::traverser( _root ).apply( &vis );
        }

#if 0
        {
            motor::scene::render_visitor_t vis( 1, fe, _camera );
            motor::scene::node_t::traverser( _root ).apply( &vis );
        }
#endif
    }

    //******************************************************************************************************
    virtual void_t on_frame_done( void_t ) noexcept
    {
        _mslm->on_frame_done();
    }

    //******************************************************************************************************
    virtual void_t on_update( motor::application::app::update_data_in_t ) noexcept
    {
        MOTOR_PROBE( "application", "on_update" );

        // push method: call completion listeners
        _mslm->on_update();

        // pull method: ask completion and react
        _mslm->for_each_configure_done(
            [ & ]( motor::string_in_t msl_name, motor::graphics::msl_object_mtr_t msl ) //
        {
            if( msl_name == "shader_0" )
            {
                motor::scene::add_msl_to_set_visitor_t v( 0, motor::share( msl ),
                    [ & ]( motor::string_in_t node_name, motor::graphics::variable_set_mtr_t vs )
                {
                    if( node_name == "Render Object 0" )
                    {
                        auto * var = vs->data_variable< motor::math::vec4f_t >( "color" );
                        var->set( motor::math::vec4f_t( 0.0f, 0.0f, 1.0f, 1.0f ) );
                    }
                    else if( node_name == "Render Object 1" )
                    {
                        auto * var = vs->data_variable< motor::math::vec4f_t >( "color" );
                        var->set( motor::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ) );
                    }

                    // apply on all others
                    else
                    {
                        auto * var = vs->data_variable< motor::math::vec4f_t >( "color" );
                        var->set( motor::math::vec4f_t( 0.0f, 1.0f, 0.0f, 1.0f ) );
                    }
                } );
                motor::scene::node_t::traverser( _root ).apply( &v );
            }

            // added but not rendered.
            if( msl_name == "shader_1" )
            {
                motor::scene::add_msl_to_set_visitor_t v( 1, motor::share( msl ),
                    [ & ]( motor::string_in_t node_name, motor::graphics::variable_set_mtr_t vs )
                {
                    auto * var = vs->data_variable< motor::math::vec4f_t >( "color" );
                    var->set( motor::math::vec4f_t( 0.0f, 1.0f, 1.0f, 1.0f ) );
                } );
                motor::scene::node_t::traverser( _root ).apply( &v );
            }
        } );

        // must use this in order to update trafo components.
        {
            motor::scene::variable_update_visitor_t v;
            motor::scene::node_t::traverser( _root ).apply( &v );
        }
        {
            motor::scene::trafo_visitor_t v;
            motor::scene::node_t::traverser( _root ).apply( &v );
        }
    }

    //******************************************************************************************************
    virtual bool_t on_tool(
        this_t::window_id_t const wid, motor::application::app::tool_data_ref_t ) noexcept
    {
        MOTOR_PROBE( "application", "on_tool" );

#if 0
            {
                if( ImGui::Begin("Scene Graph Window") )
                {
                    motor::tool::imgui_node_visitor_t v( motor::move( _selected ) ) ;
                    motor::scene::node_t::traverser( _root ).apply( &v ) ;
                    _selected = v.get_selected() ;
                }
                ImGui::End() ;
            }
#endif

        if( _msls_are_init )
        {
            if( ImGui::Button( "Release msls" ) )
            {

                // motor::scene::add_msl_to_set_visitor_t v();
                // motor::scene::node_t::traverser( _root ).apply( &v );
                _button_pressed = true;
            }
        }
        else
        {
            _button_pressed = true;
        }
        return true;
    }

    virtual void_t on_shutdown( void_t ) noexcept
    {
        motor::memory::release_ptr( motor::move( _selected ) );
        motor::memory::release_ptr( _root );
        motor::memory::release_ptr( root_so );
        motor::memory::release_ptr( _camera );

        motor::release( motor::move( _mslm ) );
    }
};
} // namespace this_file

int main( int argc, char ** argv )
{
    return motor::platform::global_t::create_and_exec< this_file::my_app >();
}