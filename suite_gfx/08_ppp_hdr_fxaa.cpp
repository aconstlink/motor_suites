
#include <motor/platform/global.h>

#include <motor/scene/visitor/camera_collector_visitor.hpp>
#include <motor/scene/visitor/graphics/render_visitor.h>
#include <motor/scene/visitor/graphics/light_pass_render_visitor.h>
#include <motor/scene/visitor/trafo_visitor.h>
#include <motor/scene/visitor/variable_update_visitor.h>
#include <motor/scene/visitor/graphics/add_msl_to_set_visitor.hpp>
#include <motor/scene/visitor/graphics/connect_msl_slot_visitor.hpp>

#include <motor/scene/node/logic_group.h>
#include <motor/scene/component/trafo3d_component.h>

#include <motor/gfx/primitive/primitive_render_3d.h>
#include <motor/gfx/camera/generic_camera.h>
#include <motor/gfx/postprocess/hdr_postprocess_pipeline.h>
#include <motor/gfx/manager/msl_manager.h>

#include <motor/math/utility/fn.hpp>
#include <motor/math/utility/angle.hpp>
#include <motor/math/animation/keyframe_sequence.hpp>

#include <motor/tool/imgui/imgui_property.h>

#include <motor/wire/slot/output_slot.h>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>
#include <motor/format/global.h>

#include <future>

// this test look long and complex, but it really also does some things.
// this test shows how the hdr post processing pipeline works
// 1. it loads a gltf scene(only geometry, cameras and animation)
// 2. distributes loaded shaders to the scene graph
// 3. connects animation timing nodes(from the wire layer)
// 4. renders depth pass into post processing pipeline(ppp)
// 5. renders light pass into ppp
namespace this_file
{

enum class msl_id
{
    default_id = 0,
    color_pass_id = 1,
    light_pass_id = 2,
    depth_pass_id = 3,
    shadow_depth_id = 4,
    shadow_accum_id = 5,
    num_ids
};

static motor::scene::msl_set_component::id_t to_id( msl_id const id ) noexcept
{
    return motor::scene::msl_set_component::id_t( id );
}

using namespace motor::core::types;

class my_app : public motor::application::app
{
    motor_this_typedefs( my_app );

    motor::graphics::state_object_mtr_t _final_so = nullptr;

    motor::io::database_mtr_t _db = nullptr;

    motor::gfx::msl_manager_mtr_t _own_mmgr = nullptr;

    motor::scene::node_mtr_t _root = nullptr;

  private: // post processing

    motor::gfx::hdr_postprocess_pipeline_mtr_t _pp_pipe = nullptr;
    motor::property::property_sheet_t _pp_sheet;
    bool_t _show_temp_rt = false;

  private: // camera

    size_t _cam_id = size_t( -1 );
    motor::vector< std::pair< motor::string_t, motor::gfx::generic_camera_mtr_t > > _cameras;

    motor::scene::camera_collector_visitor_t _cc;

    struct camera_sequence_item
    {
        motor::math::time_ms_t start;
        motor::math::time_ms_t end;
        motor::gfx::generic_camera_mtr_t cam;
    };

    motor::vector< camera_sequence_item > _cs;

  private: // wire

    using os_float_t = motor::wire::output_slot< float_t >;
    using os_trafo_t = motor::wire::output_slot< motor::math::m3d::trafof_t >;

    // output slot for the imported scene animations.
    os_float_t * _time = nullptr;

    // allows to scale the whole scene. it is connected
    // to the root transformation in the imported sceen.
    os_trafo_t * _scale_os = nullptr;

    motor::wire::time_node_mtr_t _time_node = nullptr; // from the importer
    motor::wire::inode_mtr_t _merger = nullptr;        // from the importer

    // store all the wire nodes for easier destruction.
    motor::vector< motor::wire::inode_mtr_t > _node_dump;

    // we need this flag for proper release.
    // if the async chain is done, this flag is tirggered.
    bool_t _async_done = false;

  private: // shader variable slots

    motor_typedefs( motor::wire::input_slot< float_t >, float_is );
    motor_typedefs( motor::wire::input_slot< motor::math::vec3f_t >, vec3_is );

    motor_typedefs( motor::wire::output_slot< float_t >, float_os );
    motor_typedefs( motor::wire::output_slot< motor::math::vec3f_t >, vec3_os );

    float_os_mtr_t _shininess = motor::shared( float_os_t( 80.0f ) );
    float_os_mtr_t _specular_strength = motor::shared( float_os_t( 30.0f ) );
    float_os_mtr_t _light_intensity = motor::shared( float_os_t( 2.0f ) );

    vec3_os_mtr_t _hemi_top_color =
        motor::shared( vec3_os_t( motor::math::vec3f_t( 0.0f, 0.0f, 10.0f ) ) );

  public:

    virtual void_t on_init( void_t ) noexcept
    {
        _db = motor::shared(
            motor::io::database( motor::io::path_t( DATAPATH ), "./working", "data" ) );

        {
            _time = motor::shared( os_float_t( 0.0f ) );
            _scale_os = motor::shared( os_trafo_t() );
        }

#if 1
        {
            motor::application::window_info_t wi;
            wi.x = 100;
            wi.y = 100;
            wi.w = 1920 >> 1;
            wi.h = 1080 >> 1;
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
#if 1
        {
            motor::application::window_info_t wi;
            wi.x = 100 + ( 1920 >> 1 );
            wi.y = 100;
            wi.w = 1920 >> 1;
            wi.h = 1080 >> 1;
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

        // the main state sets comes for the ppp. This is only doing a
        // difference state change for rendering this scene.
        // the ppp provides
        // 1. render states for the depth pass
        // 2. render states for the color/light pass
        {
            motor::graphics::state_object_t so =
                motor::graphics::state_object_t( "final_render_states" );

            {
                motor::graphics::render_state_sets_t rss;
#if 0
                rss.depth_s.do_change = true;
                rss.depth_s.ss.do_activate = true;
                rss.depth_s.ss.do_depth_write = true;
#endif
                rss.polygon_s.do_change = true;
                rss.polygon_s.ss.do_activate = true;
                rss.polygon_s.ss.fm = motor::graphics::fill_mode::fill;
                rss.polygon_s.ss.ff = motor::graphics::front_face::counter_clock_wise;
                rss.polygon_s.ss.cm = motor::graphics::cull_mode::back;
#if 0
                rss.clear_s.do_change = true;
                rss.clear_s.ss.clear_color = motor::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f );
                rss.clear_s.ss.do_activate = true;
                rss.clear_s.ss.do_color_clear = true;
                rss.clear_s.ss.do_depth_clear = true;
#endif
#if 0
                rss.view_s.do_change = true;
                rss.view_s.ss.do_activate = false;
                rss.view_s.ss.vp = motor::math::vec4ui_t( 0, 0, 500, 500 );
#endif
#if 0
                rss.blend_s.do_change = false ;
                rss.blend_s.ss.do_activate = true ;
                rss.blend_s.ss.blend_func = motor::graphics::blend_function::add ;
                rss.blend_s.ss.src_blend_factor = motor::graphics::blend_factor::one ;
                rss.blend_s.ss.dst_blend_factor = motor::graphics::blend_factor::one ;
#endif

                so.add_render_state_set( rss );
            }

            _final_so = motor::shared( motor::graphics::state_object_t( std::move( so ) ) );
        }

        {
            _pp_pipe = motor::shared( motor::gfx::hdr_postprocess_pipeline_t(1920,1080) );
            _pp_pipe->init();
        }

        // init scene tree
        // - import the gltf file
        // - connect animation nodes from the import with
        // app nodes/slots for easy animation update.
        // - collect the cameras
        {
            motor::scene::logic_group_t root;
            root.add_component( motor::shared( motor::scene::name_component_t( "intro scene" ) ) );

            // add imported scene
            {
                motor::scene::node_mtr_t imported_node = nullptr;

                auto group = motor::shared( motor::scene::logic_group_t() );

                // make importer ready
                {
                    motor::format::module_registry_mtr_t mod_reg =
                        motor::format::global::register_default_modules(
                            motor::shared( motor::format::module_registry_t(), "mod registry" ) );

                    // import the gltf asset.
                    {
                        motor::property::property_sheet_t ps;
                        ps.add_property< motor::string_t >( "base_name",
                            motor::property::generic_property< motor::string_t >( "08_ppp_hdr_fxaa" ) );

                        auto item = mod_reg->import_from(
                            motor::io::location_t( "assets.test_scene1.gltf" ), _db,
                            motor::shared( std::move( ps ) ) );

                        auto * ret_item = item.get();

                        if( auto * scene_item =
                                dynamic_cast< motor::format::scene_item_ptr_t >( ret_item );
                            scene_item != nullptr )
                        {
                            imported_node = motor::move( scene_item->root );

                            motor::wire::release( motor::move( _time_node ) );
                            motor::wire::release( motor::move( _merger ) );

                            _time_node = motor::move( scene_item->start_node );
                            _merger = motor::move( scene_item->merger_node );

                            _time_node->borrow_time_is()->connect( motor::share( _time ) );

                            // camera sequence
                            {
                                for( auto & csi : _cs ) motor::release( motor::move( csi.cam ) );

                                _cs.clear();

                                for( auto & i : scene_item->camera_sequence )
                                {
                                    this_t::camera_sequence_item csi;
                                    csi.cam = motor::move( i.cam );
                                    csi.start = i.start;
                                    csi.end = i.end;
                                    _cs.emplace_back( csi );
                                }
                            }
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
                    comp.set_trafo( t );

                    {
                        motor::wire::inputs_t inputs;
                        comp.inputs( inputs );
                        inputs.connect( "trafo", motor::share( _scale_os ) );
                    }

                    imported_node->add_component( motor::shared( std::move( comp ) ) );
                }
                group->add_child( motor::move( imported_node ) );
                root.add_child( motor::move( group ) );
            }

            motor::release( motor::move( _root ) );
            _root = motor::shared( std::move( root ) );
        }

        // reconnect nodes
        {
            this_t::release_node_dump();

            auto t = motor::shared( motor::wire::funk_node_t(
                [ = ]( motor::wire::funk_node_ptr_t ) { this->_async_done = true; } ) );

            _node_dump.emplace_back( motor::share( t ) );
            _merger->then( motor::move( t ) );
        }
        // motor::release( motor::move( _selected_node ) );

        // reload cameras
        {
            this_t::release_cameras();

            motor::scene::node_t::traverser( _root ).apply( &_cc );
            auto cams = _cc.get_cameras();

            if( cams.size() > 0 )
            {
                size_t i = 0;
                _cameras.resize( cams.size() );
                for( auto & c : cams )
                {
                    _cameras[ i ].first = c.first;
                    _cameras[ i++ ].second = motor::share( c.second );
                }
                _cam_id = 0;
            }
            else
            {
                motor::log::global_t::error( "gltf file has no cameras." );
            }
        }

        // manager
        {
            _own_mmgr = motor::shared( motor::gfx::msl_manager_t( motor::share( _db ) ) );
            _own_mmgr->add(
                "color_pass", motor::io::location_t( "08_ppp_hdr_fxaa.shaders.color_pass.msl" ) );
            _own_mmgr->add(
                "light_pass", motor::io::location_t( "08_ppp_hdr_fxaa.shaders.light_pass.msl" ) );
            _own_mmgr->add(
                "depth_pass", motor::io::location_t( "08_ppp_hdr_fxaa.shaders.depth_pass.msl" ) );
        }
    }

    virtual void_t on_event( window_id_t const wid,
        motor::application::window_message_listener::state_vector_cref_t sv ) noexcept
    {
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
        }
    }

    //******************************************************************************************************
    virtual void_t on_update( motor::application::app::update_data_in_t ud ) noexcept
    {
        _own_mmgr->on_update();
        _own_mmgr->for_each_configure_done(
            [ & ]( motor::string_in_t name, motor::graphics::msl_object_mtr_t msl ) //
        {
            if( name == "color_pass" )
            {
                motor::scene::add_msl_to_set_visitor_t v(
                    this_file::to_id( this_file::msl_id::color_pass_id ), motor::share( msl ) );
                motor::scene::node_t::traverser( _root ).apply( &v );
            }
            if( name == "light_pass" )
            {
                {
                    motor::scene::add_msl_to_set_visitor_t v(
                        this_file::to_id( this_file::msl_id::light_pass_id ), motor::share( msl ) );
                    motor::scene::node_t::traverser( _root ).apply( &v );
                }

                {
                    motor::scene::connect_msl_slot_visitor_t v(
                        this_file::to_id( this_file::msl_id::light_pass_id ),
                        [ & ]( motor::wire::inputs_ref_t inputs )
                    {
                        {
                            auto * input = inputs.borrow_or_add(
                                "shininess", motor::shared( this_t::float_is_t( 0.0f ) ) );
                            if( input ) input->connect( motor::share( _shininess ) );
                        }

                        {
                            auto * input = inputs.borrow_or_add(
                                "specular_strength", motor::shared( this_t::float_is_t( 0.0f ) ) );
                            if( input ) input->connect( motor::share( _specular_strength ) );
                        }

                        {
                            auto * input = inputs.borrow_or_add(
                                "light_intensity", motor::shared( this_t::float_is_t( 0.0f ) ) );
                            if( input ) input->connect( motor::share( _light_intensity ) );
                        }

                        {
                            auto * input = inputs.borrow_or_add(
                                "hemi_top_color", motor::shared( this_t::vec3_is_t() ) );
                            if( input ) input->connect( motor::share( _hemi_top_color ) );
                        }
                    } );
                    motor::scene::node_t::traverser( _root ).apply( &v );
                }
            }
            else if( name == "depth_pass" )
            {
                motor::scene::add_msl_to_set_visitor_t v(
                    this_file::to_id( this_file::msl_id::depth_pass_id ), motor::share( msl ) );
                motor::scene::node_t::traverser( _root ).apply( &v );
            }
        } );

        // 4 seconds animation is in sync with the blender export
        {
            static float_t t = 0.0f;
            t += ud.sec_dt;
            t = motor::math::fn< float_t >::mod( t, 4.0 );

            _time->set_and_exchange( t );
        }

        // note, this is async, so the values may not be
        // finished when rendering. For this test, this is ok.
        // _time_node is connected to the imported scene and
        // is responsible for the animations.
        {
            _async_done = false;
            motor::concurrent::global_t::schedule(
                _time_node->get_task(), motor::concurrent::schedule_type::pool );
        }

// sync of pool tasks need to be done by the user itself.
// this is just an example. Do syncronization by any means
// you find necessarry.
#if 1
        {
            while( !_async_done );
        }
#endif

        // absolutely required. this visitor bakes local
        // transformations, i.e. if animations occure or if
        // the local trafor is just commited to the usable
        // transformation by the trafo visitor
        {
            motor::scene::variable_update_visitor_t v;
            motor::scene::node_t::traverser( _root ).apply( &v );
        }

        {
            motor::scene::trafo_visitor_t v;
            motor::scene::node_t::traverser( _root ).apply( &v );
        }
    }

    virtual void_t on_graphics( motor::application::app::graphics_data_in_t gd ) noexcept {}

    virtual void_t on_render( this_t::window_id_t const wid,
        motor::graphics::gen4::frontend_ptr_t fe,
        motor::application::app::render_data_in_t rd ) noexcept
    {
        motor::log::global_t::status(
            !_async_done, "[08_ppp] : async not done yet but rendering." );

        if( rd.first_frame )
        {
            _pp_pipe->init_render( fe );
            fe->configure< motor::graphics::state_object_t >( _final_so );
        }

        if( rd.last_frame )
        {
            _pp_pipe->release_render( fe );
            fe->release< motor::graphics::state_object_t >( _final_so );
            return;
        }

        _own_mmgr->on_render( fe );

        // render into framebuffer
        {
            // activate fb 0
            fe->use( _pp_pipe->borrow_hdr_fb( 0 ) );

            // z-prepass
            {
                fe->push( _pp_pipe->borrow_zpre_states() );
                this_t::on_render_depth_pass( fe );
                fe->pop( motor::graphics::gen4::backend::pop_type::render_state );
            }

            // render color
            {
                fe->push( _pp_pipe->borrow_hdr_states() );
                this_t::on_render_scene( fe );
                fe->pop( motor::graphics::gen4::backend::pop_type::render_state );
            }
            fe->unuse( motor::graphics::gen4::backend::unuse_type::framebuffer );
        }

        // present framebuffer
        {
            _pp_pipe->render( fe, _show_temp_rt );
        }
    }

    //************************************************************************************
    virtual void_t on_render_depth_pass( motor::graphics::gen4::frontend_ptr_t fe ) noexcept
    {
        if( _cam_id != size_t( -1 ) )
        {
            fe->push( _final_so );
            {
                motor::gfx::generic_camera_mtr_t cam = _cameras[ _cam_id ].second;
                motor::scene::render_visitor_t vis(
                    this_file::to_id( this_file::msl_id::depth_pass_id ), fe, cam );

                motor::scene::node_t::traverser( _root ).apply( &vis );
            }

            fe->pop( motor::graphics::gen4::backend::pop_type::render_state );
        }
    }

    //************************************************************************************
    virtual void_t on_render_scene( motor::graphics::gen4::frontend_ptr_t fe ) noexcept
    {
        if( _cam_id != size_t( -1 ) )
        {
            fe->push( _final_so );
#if 1
            {
                motor::gfx::generic_camera_mtr_t cam = _cameras[ _cam_id ].second;
                motor::scene::render_visitor_t vis(
                    this_file::to_id( this_file::msl_id::default_id ), fe, cam );
                motor::scene::node_t::traverser( _root ).apply( &vis );
            }
#else
            {

                motor::gfx::generic_camera_mtr_t cam = _cameras[ _cam_id ].second;
                motor::scene::light_pass_render_visitor_t vis(
                    this_file::to_id( this_file::msl_id::light_pass_id ), fe, cam,
                    motor::scene::light_pass_render_visitor_t::light{
                        motor::scene::light_pass_render_visitor_t::light_type::directional_light,
                        motor::math::vec3f_t( -1.0f, -1.0f, 0.0f ),
                        motor::math::mat4f_t::make_identity(),
                        motor::math::mat4f_t::make_identity(), "shadow_depth_framebuffer.depth" } );

                motor::scene::node_t::traverser( _root ).apply( &vis );
            }
#endif
            fe->pop( motor::graphics::gen4::backend::pop_type::render_state );
        }
    }

    //************************************************************************************
    virtual void_t on_frame_done( void_t ) noexcept
    {
        _own_mmgr->on_frame_done();
    }

    //******************************************************************************************************
    bool_t on_tool(
        this_t::window_id_t const wid, motor::application::app::tool_data_ref_t td ) noexcept
    {
        // SECTION: cameras
        {
            auto cams = _cc.get_cameras();

            if( cams.size() > 0 )
            {
                size_t i = 0;
                static ImGuiComboFlags flags = 0;
                motor::vector< char const * > items( cams.size() );
                for( auto const & e : cams )
                {
                    items[ i++ ] = e.first.c_str();
                }

                motor::string_t combo_name = "Scene Camera#";

                int item_selected_idx = _cam_id != size_t( -1 ) ? int_t( _cam_id ) : 0;
                const char * combo_preview_value = items[ item_selected_idx ];
                if( ImGui::BeginCombo( combo_name.c_str(), combo_preview_value, flags ) )
                {
                    for( int n = 0; n < items.size(); n++ )
                    {
                        const bool is_selected = ( item_selected_idx == n );
                        if( ImGui::Selectable( items[ n ], is_selected ) ) item_selected_idx = n;

                        // Set the initial focus when opening the combo (scrolling +
                        // keyboard navigation focus)
                        if( is_selected ) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();

                    _cam_id = size_t( item_selected_idx );
                }
            }
        }

        if( ImGui::Begin( "Post Process" ) )
        {
            bool_t has_changed = false;
            for( auto ps : _pp_pipe->property_sheets() )
            {
                has_changed |= motor::tool::imgui_property::handle( ps.first, *ps.second );
            }
            if( has_changed ) _pp_pipe->update_properies();

            {
                if( ImGui::Checkbox( "Show temp render target ##scene_manager", &_show_temp_rt ) )
                {
                    //_pp_pipe->set_map_to_screen_texture_temp( "scene.00.shadow_framebuffer.depth"
                    //);
                    _pp_pipe->set_map_to_screen_texture_temp(
                        "gfx.postprocess.fb.full.hdr.0.0"
                        //"scene.00.shadow_accum_framebuffer.0"
                        //"gfx.postprocess.hdr.framebuffer.0.depth" 
                        );
                }
            }
        }
        ImGui::End();

        if( ImGui::Begin( "Light Variables" ) )
        {
            {
                float_t v = _shininess->get_value();
                if( ImGui::SliderFloat( "Shininess", &v, 0.0f, 100.0f ) )
                {
                    _shininess->set_and_exchange( v );
                }
            }

            {
                float_t v = _specular_strength->get_value();
                if( ImGui::SliderFloat( "specular_strength", &v, 0.0f, 100.0f ) )
                {
                    _specular_strength->set_and_exchange( v );
                }
            }

            {
                float_t v = _light_intensity->get_value();
                if( ImGui::SliderFloat( "light_intensity", &v, 0.1f, 10.0f ) )
                {
                    _light_intensity->set_and_exchange( v );
                }
            }

            {

                float_t v[ 3 ] = { _hemi_top_color->get_value().x(),
                    _hemi_top_color->get_value().y(), _hemi_top_color->get_value().z() };

                if( ImGui::SliderFloat3( "upper hemnisphere color", v, 0.1f, 10.0f ) )
                {
                    _hemi_top_color->set_and_exchange( motor::math::vec3f_t( v ) );
                }
            }
        }
        ImGui::End();
        return true;
    }

    //******************************************************************************************************
    void_t on_shutdown( void_t ) noexcept
    {
        while( !_async_done );
        release_all_objects();
    }

  private:

    void_t release_cameras( void_t ) noexcept
    {
        _cc.release();
        for( auto & cam : _cameras )
        {
            motor::release( motor::move( cam.second ) );
        }
        _cameras.clear();

        for( auto & csi : _cs ) motor::release( motor::move( csi.cam ) );
        _cs.clear();
    }

    void_t release_node_dump( void_t ) noexcept
    {
        for( auto * ptr : _node_dump )
        {
            ptr->disconnect();
            motor::release( motor::move( ptr ) );
        }
        _node_dump.clear();
    }

    void_t release_all_objects( void_t ) noexcept
    {
        motor::release( motor::move( _pp_pipe ) );
        motor::release( motor::move( _db ) );

        motor::wire::release( motor::move( _time ) );
        motor::wire::release( motor::move( _scale_os ) );
        motor::wire::release( motor::move( _time_node ) );
        motor::wire::release( motor::move( _merger ) );

        motor::release( motor::move( _final_so ) );

        motor::release( motor::move( _root ) );

        this_t::release_cameras();
        this_t::release_node_dump();

        motor::release( motor::move( _own_mmgr ) );

        motor::release( motor::move( _shininess ) );
        motor::release( motor::move( _specular_strength ) );
        motor::release( motor::move( _light_intensity ) );
        motor::release( motor::move( _hemi_top_color ) );
    }
};
} // namespace this_file

int main( int argc, char ** argv )
{
    using namespace motor::core::types;

    motor::application::carrier_mtr_t carrier =
        motor::platform::global_t::create_carrier( motor::shared( this_file::my_app() ) );

    auto const ret = carrier->exec();

    motor::memory::release_ptr( carrier );

    motor::io::global::deinit();
    motor::concurrent::global::deinit();
    motor::log::global::deinit();
    motor::memory::global::dump_to_std();

    return ret;
}
