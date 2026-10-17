

#include <motor/profiling/probe_guard.hpp>

#include <motor/tool/imgui/node_kit/imgui_node_visitor.h>
#include <motor/tool/imgui/imgui_property.h>

#include <motor/platform/global.h>

#include <motor/format/global.h>

#include <motor/geometry/mesh/tri_mesh.h>
#include <motor/geometry/mesh/flat_tri_mesh.h>
#include <motor/geometry/3d/cube.h>

#include <motor/controls/types/ascii_keyboard.hpp>
#include <motor/controls/types/three_mouse.hpp>

#include <motor/gfx/manager/msl_manager.h>
#include <motor/gfx/camera/generic_camera.h>
#include <motor/math/utility/angle.hpp>

#include <motor/scene/node/logic_group.h>
#include <motor/scene/node/logic_leaf.h>

#include <motor/scene/component/name_component.hpp>
#include <motor/scene/component/graphics/msl_component.h>
#include <motor/scene/component/graphics/msl_set_component.hpp>
#include <motor/scene/component/graphics/render_settings_component.hpp>
#include <motor/scene/component/trafo3d_component.h>
#include <motor/scene/component/camera_component.h>

#include <motor/scene/visitor/search_by_name_visitor.hpp>
#include <motor/scene/visitor/trafo_visitor.h>
#include <motor/scene/visitor/light_collector_visitor.hpp>
#include <motor/scene/visitor/camera_collector_visitor.hpp>
#include <motor/scene/visitor/graphics/render_visitor.h>
#include <motor/scene/visitor/variable_update_visitor.hpp>
#include <motor/scene/visitor/graphics/light_pass_render_visitor.h>
#include <motor/scene/visitor/graphics/add_msl_to_set_visitor.hpp>

#include <motor/wire/variables/trafo_variables.hpp>
#include <motor/wire/variables/vector_variables.hpp>

#include <motor/math/interpolation/interpolate.hpp>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>

#include <future>

namespace this_file
{

enum class passes
{
    base_color_pass = 0,
    light_pass = 1
};
static size_t to_idx( passes const p ) noexcept
{
    return size_t( p );
}

using namespace motor::core::types;

class my_app : public motor::application::app
{
    motor_this_typedefs( my_app );

    motor::math::vec4ui_t fb_dims = motor::math::vec4ui_t( 0, 0, 1920, 1080 );

    motor::scene::node_mtr_t _root;
    motor::scene::node_mtr_t _selected = nullptr;
    motor::property::property_sheet_t _props;

    motor::graphics::state_object_mtr_t root_so;
    motor::graphics::state_object_mtr_t light_so;

    using os_float_t = motor::wire::output_slot< float_t >;
    using os_trafo_t = motor::wire::output_slot< motor::math::m3d::trafof_t >;

    os_float_t * _time = motor::shared( os_float_t( 0.0f ) );
    os_trafo_t * _scale_os = motor::shared( os_trafo_t() );

    motor::wire::time_node_mtr_t _time_node; // from the importer
    motor::wire::inode_mtr_t _merger;        // from the importer

    motor::gfx::generic_camera_mtr_t _camera;

    motor::gfx::msl_manager_mtr_t _mslm;

    int_t _max_lights_render = 0;
    uint_t _num_lights_found = 0;
    bool_t _use_num_ligts_found = 0;

    void_t init_manager_shaders( motor::gfx::msl_manager_mtr_t mgr ) noexcept
    {
        {
            motor::string_t shd = R"(
                config light_pass
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

                        vec3_t light_pos : light_position ;
                        vec3_t light_dir : light_direction ;

                        bool_t is_point_light(true) ;

                        out vec3_t light_dir ;

                        void main()
                        {
                            
                            vec3_t pos = in.pos ;
                            //pos.xyz = pos.xyz * 10.0 ;
                            out.tx = in.tx ;

                            out.pos = proj * view * world * vec4_t( pos, 1.0 ) ;
                            out.nrm = normalize( world * vec4_t( in.nrm, 0.0 ) ).xyz ;

                            vec3_t wrl_pos = (world * vec4_t( pos, 1.0 )).xyz;

                            // points towards light
                            if( is_point_light ) out.light_dir = (light_pos - wrl_pos) ;
                            // points towards light
                            else out.light_dir = -light_dir ;
                        }
                    }

                    pixel_shader
                    {
                        tex2d_t tex ;
                        vec4_t color ;

                        in vec2_t tx : texcoord ;
                        in vec3_t nrm : normal ;
                        out vec4_t color0 : color0 ;

                        in vec3_t light_dir ;

                        void main()
                        {
                            float_t light = dot( normalize( in.nrm ), normalize( in.light_dir) ) ;
                            out.color0 = vec4_t( light, light, light, light ) ;
                            //out.color0 = vec4_t( 0.0, 1.0, 0.0, 1.0 ) ;
                            //out.color0 = out.color0 ' vec4_t( color.xyz, 1.0 ) ;
                        }
                    }
                })";

            mgr->add( "light_pass", shd );
            // variables are set when the shader is done.
            // @see on_update
        }

        {
            motor::string_t shd = R"(
                config base_color_pass
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
                            //pos.xyz = pos.xyz * 10.0 ;
                            out.tx = in.tx ;
                            out.pos = proj * view * world * vec4_t( pos, 1.0 ) ;
                            out.nrm = normalize( world * vec4_t( in.nrm, 0.0 ) ).xyz ;
                        }
                    }

                    pixel_shader
                    {
                        tex2d_t tex ;
                        vec4_t base_color ;

                        in vec2_t tx : texcoord ;
                        in vec3_t nrm : normal ;
                        out vec4_t color0 : color0 ;

                        void main()
                        {
                            out.color0 = vec4_t( base_color.xyz, 1.0 ) ;
                        }
                    }
                })";

            // mgr->add( "base_color_pass", shd );
            //  variables are set when the shader is done.
            //  @see on_update
        }
    }

    //******************************************************************************************************
    virtual void_t on_init( void_t ) noexcept
    {
        motor::io::database_mtr_t db = motor::shared(
            motor::io::database_t( motor::io::path_t( DATAPATH ), "./working", "data" ) );

        MOTOR_PROBE( "application", "on_init" );

// #1 : init window
#if 1
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
#endif
// #2 : init window
#if 1
        {
            motor::application::window_info_t wi;
            wi.x = 500;
            wi.y = 100;
            wi.w = 800;
            wi.h = 600;
            wi.gen = motor::application::graphics_generation::gen4_gl4;

            this_t::send_window_message( this_t::create_window( wi ),
                [ & ]( motor::application::app::window_view & wnd )
            {
                wnd.send_message( motor::application::show_message( { true } ) );
                wnd.send_message( motor::application::cursor_message_t( { true } ) );
                wnd.send_message( motor::application::vsync_message_t( { true } ) );
            } );
        }
#endif

        // camera
        {
            auto cam = motor::gfx::generic_camera_t( 1.0f, 1.0f, 1.0f, 1000.0f );
            cam.perspective_fov( motor::math::angle< float_t >::degree_to_radian( 45.0f ) );
            cam.look_at( motor::math::vec3f_t( 0.0f, 0.0f, 80.0f ),
                motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ),
                motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) );

            _camera = motor::shared( std::move( cam ) );
        }

        // manager stuff
        {
            motor::gfx::msl_manager_t mgr( motor::share( db ) );

            /// this_t::init_manager_shaders( &mgr );

            _mslm = motor::shared( std::move( mgr ) );
        }

        {
            motor::graphics::state_object_t so =
                motor::graphics::state_object_t( "root_render_states" );

            {
                motor::graphics::render_state_sets_t rss;
                rss.depth_s.do_change = true;
                rss.depth_s.ss.do_activate = true;
                rss.depth_s.ss.do_depth_write = true;
                rss.polygon_s.do_change = true;
                rss.polygon_s.ss.do_activate = true;
                rss.polygon_s.ss.fm = motor::graphics::fill_mode::fill;
                rss.polygon_s.ss.ff = motor::graphics::front_face::counter_clock_wise;
                rss.polygon_s.ss.cm = motor::graphics::cull_mode::back;
                rss.clear_s.do_change = true;
                rss.clear_s.ss.clear_color = motor::math::vec4f_t( 0.5f, 0.9f, 0.5f, 1.0f );
                rss.clear_s.ss.do_activate = true;
                rss.clear_s.ss.do_color_clear = false;
                rss.clear_s.ss.do_depth_clear = true;
                rss.view_s.do_change = true;
                rss.view_s.ss.do_activate = false;
                rss.view_s.ss.vp = motor::math::vec4ui_t( 0, 0, 500, 500 );
                so.add_render_state_set( rss );
            }

            root_so = motor::shared( motor::graphics::state_object_t( std::move( so ) ) );
        }

        {
            motor::graphics::state_object_t so =
                motor::graphics::state_object_t( "light_render_states" );

            {
                motor::graphics::render_state_sets_t rss;
                rss.depth_s.do_change = true;
                rss.depth_s.ss.do_activate = true;
                rss.depth_s.ss.do_depth_write = false;
                rss.depth_s.ss.compare_funk = motor::graphics::depth_compare::less_equal;

                rss.blend_s.do_change = true;
                rss.blend_s.ss.do_activate = true;
                rss.blend_s.ss.blend_func = motor::graphics::blend_function::add;
                rss.blend_s.ss.dst_blend_factor = motor::graphics::blend_factor::one;
                rss.blend_s.ss.src_blend_factor = motor::graphics::blend_factor::src_alpha;

#if 0
                // this is done in the frame restart of the
                // rendering backends. Here we can not do it
                // because we are using mutiple lights and therefore
                // render the scene scene multiple times.
                rss.clear_s.do_change = true;
                rss.clear_s.ss.clear_color = motor::math::vec4f_t( 0.5f, 0.9f, 0.5f, 1.0f );
                rss.clear_s.ss.do_activate = true;
                rss.clear_s.ss.do_color_clear = true;
                rss.clear_s.ss.do_depth_clear = true;
#endif

#if 0
                // do not need to set the viewport here. It is
                // done through the backends.
                rss.view_s.do_change = true;
                rss.view_s.ss.do_activate = false;
                rss.view_s.ss.vp = motor::math::vec4ui_t( 0, 0, 500, 500 );
#endif

                so.add_render_state_set( rss );
            }

            light_so = motor::shared( motor::graphics::state_object_t( std::move( so ) ) );
        }

// time noe
#if 0
            {
                _time_node = motor::shared( motor::wire::time_node_t() ) ;
                _time_node->borrow_time_is()->set_value( 0.0f ) ;
            }
#endif

        // #3 : init scene tree
        {
            motor::scene::logic_group_t root;
            root.add_component( motor::shared( motor::scene::name_component_t( "my root name" ) ) );

            // add imported scene
            {
                motor::scene::node_mtr_t imported_node = nullptr;

                auto rs = motor::shared( motor::scene::logic_group_t() );
                {
                    auto rsc = motor::scene::render_settings_component_t( motor::share( root_so ) );

                    rs->add_component( motor::shared( std::move( rsc ) ) );

                    rs->add_component(
                        motor::shared( motor::scene::name_component_t( "Render Settings" ) ) );
                }

                // make importer ready
                {
                    motor::format::module_registry_mtr_t mod_reg =
                        motor::format::global::register_default_modules(
                            motor::shared( motor::format::module_registry_t(), "mod registry" ) );

                    // import the gltf asset.
                    {
                        auto item = mod_reg->import_from(
                            motor::io::location_t(
                                //"gltf.LightsPunctualLamp.LightsPunctualLamp.gltf"
                                "gltf.LightsBlender.Lights.gltf"

                                ),
                            db );

                        auto * ret_item = item.get();

                        // test scene with visitor
                        if( auto * scene_item =
                                dynamic_cast< motor::format::scene_item_ptr_t >( ret_item );
                            scene_item != nullptr )
                        {
                            imported_node = motor::move( scene_item->root );
                            _time_node = motor::move( scene_item->start_node );
                            _merger = motor::move( scene_item->merger_node );

                            _time_node->borrow_time_is()->connect( motor::share( _time ) );
                        }
                        else
                        {
                            motor::log::global_t::critical( "Failed to load gltf file." );
                            std::exit( 1 );
                        }

                        ret_item->release();

                        motor::release( motor::move( ret_item ) );
                    }

                    motor::release( motor::move( mod_reg ) );
                }

                // test and scale whole imported tree with
                // only one trafo component.
                {
                    motor::math::m3d::trafof_t t;
                    t.set_scale( motor::math::vec3f_t( 1.0f ) );

                    motor::scene::trafo3d_component_t comp;
                    comp.set_trafo_local( t );

                    {
                        motor::wire::inputs_t inputs;
                        comp.inputs( inputs );
                        inputs.connect( "trafo", motor::share( _scale_os ) );
                    }

                    imported_node->add_component( motor::shared( std::move( comp ) ) );
                }
                rs->add_child( motor::move( imported_node ) );
                root.add_child( motor::move( rs ) );
            }

            _root = motor::shared( std::move( root ) );
        }

        {
            motor::scene::camera_collector_visitor_t cc;
            motor::scene::node_t::traverser( _root ).apply( &cc );

            auto cams = cc.get_cameras();
            if( cams.size() > 0 )
            {
                motor::release( motor::move( _camera ) ) ;
                _camera = motor::share( cams[0].second ) ;
            }
        }

        this_t::init_manager_shaders( _mslm );
        motor::release( motor::move( db ) );
    }

    //******************************************************************************************************
    virtual void_t on_frame_done( void_t ) noexcept
    {
        _mslm->on_frame_done();
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
    virtual void_t on_graphics( motor::application::app::graphics_data_in_t d ) noexcept
    {
        {
            static float_t t_ = 0.0f;
            t_ += d.sec_dt;

            float_t t = t_;
            t = motor::math::fn< float_t >::mod( t, 12.0f );
            // t = (motor::math::fn<float_t>::mod( t, 8.0f ) / 4.0f)-1.0f ;
            // t = 1.0f - std::abs( t ) ;

            _time->set_and_exchange( t );
        }

        {
            motor::concurrent::global_t::schedule(
                _time_node->get_task(), motor::concurrent::schedule_type::pool );
        }
    }

    //******************************************************************************************************
    virtual void_t on_first_frame( this_t::window_id_t const wid,
        motor::graphics::gen4::frontend_ptr_t fe,
        motor::application::app::render_data_in_t ) noexcept
    {
        fe->configure< motor::graphics::state_object_t >( root_so );
        fe->configure< motor::graphics::state_object_t >( light_so );

        _mslm->on_render_init( fe );
    }

    //******************************************************************************************************
    virtual void_t on_last_frame( this_t::window_id_t const wid,
        motor::graphics::gen4::frontend_ptr_t fe,
        motor::application::app::render_data_in_t ) noexcept
    {
        fe->release< motor::graphics::state_object_t >( root_so );
        fe->release< motor::graphics::state_object_t >( light_so );

        _mslm->on_render_release( fe );
    }

    //******************************************************************************************************
    virtual void_t on_render( this_t::window_id_t const wid,
        motor::graphics::gen4::frontend_ptr_t fe,
        motor::application::app::render_data_in_t ) noexcept
    {
        _mslm->on_render( fe );

        size_t render_id = size_t( -1 );

        fe->push( root_so );
#if 1
        {
            motor::scene::render_visitor_t vis(
                this_file::to_idx( this_file::passes::base_color_pass ), ++render_id, fe, _camera );
            motor::scene::node_t::traverser( _root ).apply( &vis );
        }
#endif
#if 1
        fe->push( light_so );

        {
            motor::scene::light_collector_visitor_t vis;
            motor::scene::node_t::traverser( _root ).apply( &vis );

            _num_lights_found = uint_t( vis.get_num_lights() );

            vis.for_each_light( [ & ]( size_t const i, motor::scene::light_component_mtr_t light )
            {
                if( i < _max_lights_render )
                {
                    light->for_each_light( [ & ]( motor::gfx::light_mtr_t l )
                    {
                        motor::scene::light_pass_render_visitor_t vis_light(
                            this_file::to_idx( this_file::passes::light_pass ), ++render_id, fe,
                            _camera, l );
                        motor::scene::node_t::traverser( _root ).apply( &vis_light );
                    } );
                }
            } );
        }

        fe->pop( motor::graphics::gen4::backend::pop_type::render_state );
#endif

        fe->pop( motor::graphics::gen4::backend::pop_type::render_state );
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
            if( msl_name == "light_pass" )
            {
                motor::scene::add_msl_to_set_visitor_t v(
                    this_file::to_idx( this_file::passes::light_pass ), motor::share( msl ),
                    [ & ]( motor::string_in_t node_name, motor::graphics::variable_set_mtr_t vs ) {

                } );
                motor::scene::node_t::traverser( _root ).apply( &v );
            }

            // added but not rendered.
            if( msl_name == "base_color_pass" )
            {
                motor::scene::add_msl_to_set_visitor_t v(
                    this_file::to_idx( this_file::passes::base_color_pass ), motor::share( msl ),
                    [ & ]( motor::string_in_t node_name, motor::graphics::variable_set_mtr_t vs )
                {
                    if( node_name == "Render Object 0" )
                    {
                        auto * var = vs->data_variable< motor::math::vec4f_t >( "color" );
                        var->set( motor::math::vec4f_t( 0.0f, 0.0f, 0.5f, 1.0f ) );
                    }
                    else if( node_name == "Render Object 1" )
                    {
                        auto * var = vs->data_variable< motor::math::vec4f_t >( "color" );
                        var->set( motor::math::vec4f_t( 0.5f, 0.0f, 0.0f, 1.0f ) );
                    }

                    // apply on all others
                    else
                    {
                        auto * var = vs->data_variable< motor::math::vec4f_t >( "color" );
                        var->set( motor::math::vec4f_t( 0.0f, 0.5f, 0.0f, 1.0f ) );
                    }
                } );
                motor::scene::node_t::traverser( _root ).apply( &v );
            }
        } );

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

        {
            if( ImGui::Begin( "Lights" ) )
            {
                ImGuiSliderFlags sf = 0;

                ImGui::Text( "Num Lights found : %d", _num_lights_found );
                if( ImGui::Checkbox( "Use lights found", &_use_num_ligts_found ) )
                {
                }

                if( _use_num_ligts_found ) 
                {
                    sf = ImGuiSliderFlags_NoInput;
                    _max_lights_render = _num_lights_found ;
                }

                ImGui::SliderInt(
                    "max lights", &_max_lights_render, 0, _num_lights_found, "%d", sf );
            }
            ImGui::End();

            if( ImGui::Begin( "Camera Window" ) )
            {
                // choose camera from scene
                {
                    motor::scene::camera_collector_visitor_t cc;
                    motor::scene::node_t::traverser( _root ).apply( &cc );

                    auto cams = cc.get_cameras();

                    if( cams.size() > 0 )
                    {
                        size_t i = size_t( -1 );
                        static ImGuiComboFlags flags = 0;
                        motor::vector< char const * > items( cams.size() );
                        for( auto const & e : cams )
                        {
                            items[ ++i ] = e.first.c_str();
                        }

                        static int item_selected_idx = 0;
                        const char * combo_preview_value = items[ item_selected_idx ];
                        if( ImGui::BeginCombo( "Scene Camera", combo_preview_value, flags ) )
                        {
                            for( int n = 0; n < items.size(); n++ )
                            {
                                const bool is_selected = ( item_selected_idx == n );
                                if( ImGui::Selectable( items[ n ], is_selected ) )
                                    item_selected_idx = n;

                                // Set the initial focus when opening the combo (scrolling +
                                // keyboard navigation focus)
                                if( is_selected ) ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();

                            if( item_selected_idx != 0 )
                            {
                                motor::release( motor::move( _camera ) );
                                _camera = motor::move( cams[ item_selected_idx - 1 ].second );
                            }
                        }
                    }
                    else
                    {
                        ImGui::Text( "No cameras in the scene" );
                    }

                    for( auto e : cams )
                    {
                        motor::release( motor::move( e.second ) );
                    }
                }
            }
            ImGui::End();
        }

        return true;
    }

    virtual void_t on_shutdown( void_t ) noexcept
    {
        _props.clear();

        // release your own slots with wire::release!
        // motor::release will not resolve x-connections.
        motor::wire::release( motor::move( _time ) );
        motor::wire::release( motor::move( _scale_os ) );

        motor::release( motor::move( _root ) );
        motor::release( motor::move( root_so ) );
        motor::release( motor::move( light_so ) );

        motor::release( motor::move( _selected ) );
        motor::release( motor::move( _camera ) );

        motor::release( motor::move( _time_node ) );
        motor::release( motor::move( _merger ) );
        motor::release( motor::move( _mslm ) );
    }
};
} // namespace this_file

int main( int argc, char ** argv )
{
    return motor::platform::global_t::create_and_exec< this_file::my_app >();
}
