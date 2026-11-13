
#include <motor/platform/global.h>

#include <motor/scene/visitor/camera_collector_visitor.hpp>
#include <motor/scene/visitor/graphics/render_visitor.h>
#include <motor/scene/visitor/light_collector_visitor.hpp>
#include <motor/scene/visitor/graphics/light_pass_render_visitor.h>
#include <motor/scene/visitor/trafo_visitor.h>
#include <motor/scene/visitor/variable_update_visitor.hpp>
#include <motor/scene/visitor/graphics/add_msl_to_set_visitor.hpp>
#include <motor/scene/visitor/graphics/connect_msl_slot_visitor.hpp>
#include <motor/scene/visitor/graphics/debug_graphics_visitor.hpp>

#include <motor/scene/node/logic_group.h>
#include <motor/scene/component/trafo3d_component.h>

#include <motor/gfx/primitive/primitive_render_3d.h>
#include <motor/gfx/camera/generic_camera.h>
#include <motor/gfx/postprocess/hdr_postprocess_pipeline.h>
#include <motor/gfx/manager/msl_manager.h>
#include <motor/gfx/util/light.hpp>

#include <motor/math/utility/fn.hpp>
#include <motor/math/utility/angle.hpp>
#include <motor/math/animation/keyframe_sequence.hpp>

#include <motor/audio/util/frequency_bands.hpp>
#include <motor/audio/util/onset_detector.hpp>
#include <motor/audio/object/capture_object.hpp>
#include <motor/tool/structs.h>
#include <motor/tool/imgui/onset_meter.hpp>
#include <motor/tool/imgui/imgui_property.h>
#include <motor/tool/imgui/imnodes_wire.h>

#include <motor/wire/slot/output_slot.h>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>
#include <motor/format/global.h>

#include <future>

// testing the audio onset detection with some gltf scene import.
namespace this_file
{

enum class msl_id
{
    default_id,
    color_pass_id,
    normal_pass_id,
    light_pass_id,
    depth_pass_id,
    shadow_depth_id,
    shadow_light_id,
    shadow_accum_id,
    rsm_pass_id,
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
    motor::graphics::state_object_mtr_t _light_so = nullptr;

    motor::io::database_mtr_t _db = nullptr;

    motor::gfx::msl_manager_mtr_t _own_mmgr = nullptr;
    size_t _reg0 = size_t( -1 );
    motor::scene::node_mtr_t _root = nullptr;

    motor::gfx::primitive_render_3d_mtr_t _prim_render = nullptr;

private: // audio

    motor::audio::capture_object_t co;
    motor::vector< float_t > captured_samples;
    motor::vector< float_t > captured_frequencies;

    motor::audio::onset_detector kix_detector;
    motor::audio::onset_detector mid_detector;

    motor::tool::imgui_onset_meter low_onset_widget;
    motor::tool::imgui_onset_meter mid_onset_widget;

  private: // post processing and shadow mapping

    motor::gfx::hdr_postprocess_pipeline_mtr_t _pp_pipe = nullptr;
    motor::property::property_sheet_t _pp_sheet;
    bool_t _show_temp_rt = false;
    int_t _temp_rt_index = 0;

  private: // camera

    size_t _cam_id = size_t( -1 );
    motor::vector< std::pair< motor::string_t, motor::gfx::generic_camera_mtr_t > > _cameras;

    struct camera_sequence_item
    {
        motor::math::time_ms_t start;
        motor::math::time_ms_t end;
        motor::gfx::generic_camera_mtr_t cam;
    };

    motor::vector< camera_sequence_item > _cs;

  private: // rendering properties

    bool_t _render_light = true;
    bool_t _render_normals = false;
    bool_t _render_default = true;

    bool_t _draw_debug = false;
    bool_t _draw_leafs = false;
    size_t _max_depth = 0;
    size_t _draw_depth = 0;

    bool_t _draw_sun_ortho = false;

    motor::collide::aabb3f_t _world_bounds;

  private: // wire

    using os_float_t = motor::wire::output_slot< float_t >;
    using os_trafo_t = motor::wire::output_slot< motor::math::m3d::trafof_t >;

    // output slot for the imported scene animations.
    os_float_t * _time = nullptr;

    // allows to scale the whole scene. it is connected
    // to the root transformation in the imported sceen.
    os_trafo_t * _scale_os = nullptr;

    // store all the wire nodes for easier destruction.
    motor::vector< motor::wire::inode_mtr_t > _node_dump;

    // we need this flag for proper release.
    // if the async chain is done, this flag is tirggered.
    std::atomic< bool_t > _async_done = false;

    motor::wire::inode_mtr_t _start_node = nullptr;

    motor::wire::inode_mtr_t _end_node = nullptr;

    motor::tool::imnodes_wire_t _imn_wire;

  private: // shader variable slots

    motor_typedefs( motor::wire::input_slot< float_t >, float_is );
    motor_typedefs( motor::wire::input_slot< motor::math::vec3f_t >, vec3_is );

    motor_typedefs( motor::wire::output_slot< float_t >, float_os );
    motor_typedefs( motor::wire::output_slot< motor::math::vec3f_t >, vec3_os );

    float_os_mtr_t _intensity = motor::shared( float_os_t( 10.0f ) );
    float_os_mtr_t _shininess = motor::shared( float_os_t( 80.0f ) );
    float_os_mtr_t _specular_strength = motor::shared( float_os_t( 30.0f ) );
    float_os_mtr_t _light_intensity = motor::shared( float_os_t( 2.0f ) );
    float_os_mtr_t _bias = motor::shared( float_os_t( 0.02f ) );

    vec3_os_mtr_t _hemi_top_color =
        motor::shared( vec3_os_t( motor::math::vec3f_t( 0.0f, 0.0f, 10.0f ) ) );

  public:

    my_app( void_t ) noexcept {}
    my_app( my_app const & ) = delete;
    my_app( my_app && ) {}

    virtual ~my_app( void_t ) noexcept 
    {
        motor::release( motor::move( _intensity ) )  ;
        motor::release( motor::move( _shininess ) );
        motor::release( motor::move( _specular_strength ) );
        motor::release( motor::move( _light_intensity ) );
        motor::release( motor::move( _hemi_top_color ) );
        motor::release( motor::move( _bias ) );
    }

    //*******************************************************************************************
    virtual void_t on_audio( motor::audio::frontend_ptr_t fptr, audio_data_in_t ad ) noexcept
    {
        if( ad.first_frame )
        {
            fptr->configure( motor::audio::capture_type::what_u_hear, &co );
        }

        fptr->capture( &co );
        co.copy_samples_to( captured_samples );
        co.copy_frequencies_to( captured_frequencies );

        float_t const bin_width = co.get_bin_width_hz< float_t >();

        if( captured_frequencies.empty() ) return;

        motor::audio::frequency_bands fb( bin_width, captured_frequencies.size() );

        // low / kix ( 40 ; 250 Hz )
        {
            float_t sum = 0.0f;
            fb.for_each( 40.0f, 250.0f,
                [ & ]( size_t const, size_t const g ) { sum += captured_frequencies[ g ]; } );

            if( kix_detector.update( sum, ad.sec_dt ) )
            {
                // detected
            }
        }

        // mid ( 250 ; 2000 Hz )
        {
            float_t sum = 0.0f;
            fb.for_each( 250.0f, 2000.0f,
                [ & ]( size_t const, size_t const g ) { sum += captured_frequencies[ g ]; } );

            if( mid_detector.update( sum, ad.sec_dt ) )
            {
                // detected
            }
        }
    }

    //*******************************************************************************************
    virtual void_t on_init( void_t ) noexcept
    {
        motor::string_t const prefix = "03_wire_animation";
        motor::string_t const loc_prefix = "03_wire_animation";

        _db = motor::shared(
            motor::io::database( motor::io::path_t( DATAPATH ), "./working", "data" ) );

        {
            _time = motor::shared( os_float_t( 0.0f ) );
            _scale_os = motor::shared( os_trafo_t() );

            _start_node = motor::shared(
                motor::wire::funk_node_t( "start", []( motor::wire::funk_node_mtr_t ) {} ) );

            _end_node = motor::shared(
                motor::wire::funk_node_t( "end", []( motor::wire::funk_node_mtr_t ) {} ) );
            ;
        }
        {
            _pp_pipe = motor::shared( motor::gfx::hdr_postprocess_pipeline_t() );
            _pp_pipe->init();
        }

        this_t::init_render_states();
        this_t::init_scene( prefix, motor::io::location_t( "03_wire_animation.scene.gltf" ) );
        this_t::init_msl_manager( prefix, loc_prefix );

        // reconnect nodes
        {
            this_t::release_node_dump();

            auto t = motor::shared( motor::wire::funk_node_t( "update sync",
                [ = ]( motor::wire::funk_node_ptr_t ) { this->_async_done = true; } ) );

            _node_dump.emplace_back( motor::share( t ) );
            _end_node->then( motor::move( t ) );


            _imn_wire.build( _start_node );
        }

        // reload cameras
        {
            this_t::release_cameras();

            motor::scene::camera_collector_visitor_t v(
                [ & ]( motor::string_t node_name, motor::gfx::generic_camera_mtr_t borrowed )
            {
                _cameras.emplace_back( std::make_pair( node_name, motor::share( borrowed ) ) );
                return false; // do not collect
            } );
            motor::scene::node_t::traverser( _root ).apply( &v );

            if( !_cameras.empty() )
                _cam_id = 0;
            else
                motor::log::global_t::error( "scene has no cameras." );
        }

        {
            motor::gfx::primitive_render_3d_t prim_render;
            prim_render.init( "my_debug_render" );
            _prim_render = motor::shared( std::move( prim_render ) );
        }
    }

    //*******************************************************************************************
    void_t init_msl_manager( motor::string_in_t prefix, motor::string_in_t loc_prefix ) noexcept
    {
        // manager
        {
            _own_mmgr = motor::shared( motor::gfx::msl_manager_t( motor::share( _db ) ) );
            _own_mmgr->add(
                "color_pass", motor::io::location_t( loc_prefix + ".shaders.color_pass.msl" ) );
            _own_mmgr->add(
                "light_pass", motor::io::location_t( loc_prefix + ".shaders.light_pass.msl" ) );
            _own_mmgr->add(
                "depth_pass", motor::io::location_t( loc_prefix + ".shaders.depth_pass.msl" ) );
            _own_mmgr->add( "normal_vis_pass",
                motor::io::location_t( loc_prefix + ".shaders.normal_vis_pass.msl" ) );
            _own_mmgr->add( "shadow_depth_pass",
                motor::io::location_t( loc_prefix + ".shaders.shadow_depth_pass.msl" ) );
            _own_mmgr->add(
                "rsm_pass", motor::io::location_t( loc_prefix + ".shaders.rsm_pass.msl" ) );
            _own_mmgr->add( "shadow_light_pass",
                motor::io::location_t( loc_prefix + ".shaders.shadow_light_pass.msl" ) );

            _reg0 = _own_mmgr->register_funk(
                [ this ]( motor::string_in_t msl_name, motor::graphics::msl_object_mtr_t msl )
            {
                if( msl_name == "color_pass" )
                {
                    {
                        motor::scene::add_msl_to_set_visitor_t v(
                            this_file::to_id( this_file::msl_id::color_pass_id ),
                            motor::share( msl ) );
                        motor::scene::node_t::traverser( _root ).apply( &v );
                    }

                    {
                        motor::scene::connect_msl_slot_visitor_t v(
                            this_file::to_id( this_file::msl_id::color_pass_id ),
                            [ & ]( motor::wire::inputs_ref_t inputs )
                        {
                            {
                                auto * input = inputs.borrow_or_add(
                                    "intensity", motor::shared( this_t::float_is_t( 10.0f ) ) );
                                if( input ) input->connect( motor::share( _intensity ) );
                            }
                        } );
                        motor::scene::node_t::traverser( _root ).apply( &v );
                    }
                }
                else if( msl_name == "normal_vis_pass" )
                {
                    motor::scene::add_msl_to_set_visitor_t v(
                        this_file::to_id( this_file::msl_id::normal_pass_id ),
                        motor::share( msl ) );
                    motor::scene::node_t::traverser( _root ).apply( &v );
                }
                else if( msl_name == "light_pass" )
                {
                    {
                        motor::scene::add_msl_to_set_visitor_t v(
                            this_file::to_id( this_file::msl_id::light_pass_id ),
                            motor::share( msl ) );
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
                                auto * input = inputs.borrow_or_add( "specular_strength",
                                    motor::shared( this_t::float_is_t( 0.0f ) ) );
                                if( input ) input->connect( motor::share( _specular_strength ) );
                            }

                            {
                                auto * input = inputs.borrow_or_add( "light_intensity",
                                    motor::shared( this_t::float_is_t( 0.0f ) ) );
                                if( input ) input->connect( motor::share( _light_intensity ) );
                            }

                            {
                                auto * input = inputs.borrow_or_add(
                                    "bias", motor::shared( this_t::float_is_t( 0.02f ) ) );
                                if( input ) input->connect( motor::share( _bias ) );
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
                else if( msl_name == "shadow_light_pass" )
                {
                    {
                        motor::scene::add_msl_to_set_visitor_t v(
                            this_file::to_id( this_file::msl_id::shadow_light_id ),
                            motor::share( msl ) );
                        motor::scene::node_t::traverser( _root ).apply( &v );
                    }

                    {
                        motor::scene::connect_msl_slot_visitor_t v(
                            this_file::to_id( this_file::msl_id::shadow_light_id ),
                            [ & ]( motor::wire::inputs_ref_t inputs )
                        {
                            {
                                auto * input = inputs.borrow_or_add(
                                    "shininess", motor::shared( this_t::float_is_t( 0.0f ) ) );
                                if( input ) input->connect( motor::share( _shininess ) );
                            }

                            {
                                auto * input = inputs.borrow_or_add( "specular_strength",
                                    motor::shared( this_t::float_is_t( 0.0f ) ) );
                                if( input ) input->connect( motor::share( _specular_strength ) );
                            }

                            {
                                auto * input = inputs.borrow_or_add( "light_intensity",
                                    motor::shared( this_t::float_is_t( 0.0f ) ) );
                                if( input ) input->connect( motor::share( _light_intensity ) );
                            }

                            {
                                auto * input = inputs.borrow_or_add(
                                    "bias", motor::shared( this_t::float_is_t( 0.02f ) ) );
                                if( input ) input->connect( motor::share( _bias ) );
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
                else if( msl_name == "shadow_depth_pass" )
                {
                    motor::scene::add_msl_to_set_visitor_t v(
                        this_file::to_id( this_file::msl_id::shadow_depth_id ),
                        motor::share( msl ) );
                    motor::scene::node_t::traverser( _root ).apply( &v );
                }
                else if( msl_name == "rsm_pass" )
                {
                    motor::scene::add_msl_to_set_visitor_t v(
                        this_file::to_id( this_file::msl_id::rsm_pass_id ), motor::share( msl ) );
                    motor::scene::node_t::traverser( _root ).apply( &v );
                }
                else if( msl_name == "depth_pass" )
                {
                    motor::scene::add_msl_to_set_visitor_t v(
                        this_file::to_id( this_file::msl_id::depth_pass_id ), motor::share( msl ) );
                    motor::scene::node_t::traverser( _root ).apply( &v );
                }
            } );
        }
    }

    //*******************************************************************************************
    void_t init_render_states( void_t )
    {
#if 0
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

            _light_so = motor::shared( motor::graphics::state_object_t( std::move( so ) ) );
        }
    }

    //******************************************************************************************************
    void_t init_scene( motor::string_in_t prefix, motor::io::location_cref_t loc ) noexcept
    {
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
                            motor::property::generic_property< motor::string_t >( prefix ) );

                        auto item =
                            mod_reg->import_from( loc, _db, motor::shared( std::move( ps ) ) );

                        auto * ret_item = item.get();

                        if( auto * scene_item =
                                dynamic_cast< motor::format::scene_item_ptr_t >( ret_item );
                            scene_item != nullptr )
                        {
                            imported_node = motor::move( scene_item->root );

                            auto gltf_time = motor::move( scene_item->start_node );
                            auto gltf_end = motor::move( scene_item->merger_node );

                            gltf_time->borrow_time_is()->connect( motor::share( _time ) );

                            _start_node->then( motor::move( gltf_time ) )
                                ->then( motor::move( gltf_end ) )
                                ->then( motor::share( _end_node ) );

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
    }

    //******************************************************************************************************
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

            _pp_pipe->on_resize_window( uint_t( sv.resize_msg.w ), uint_t( sv.resize_msg.h ) );
        }
    }

    //******************************************************************************************************
    virtual void_t on_update( motor::application::app::update_data_in_t ud ) noexcept
    {
        _own_mmgr->on_update();

        // 4 seconds animation is in sync with the blender export
        {
            static float_t t = 0.0f;
            t += ud.sec_dt;
            t = motor::math::fn< float_t >::mod( t, 15.0 );

            _time->set_and_exchange( t );
        }

        // note, this is async, so the values may not be
        // finished when rendering. For this test, this is ok.
        // _time_node is connected to the imported scene and
        // is responsible for the animations.
        {
            _async_done = false;
            motor::concurrent::global_t::schedule(
                _start_node->get_task(), motor::concurrent::schedule_type::pool );
        }

// sync of pool tasks need to be done by the user itself.
// this is just an example. Do syncronization by any means
// you find necessarry.
#if 1
        {
            while( !_async_done );
        }
#endif

        {
            motor::scene::trafo_visitor_t v;
            motor::scene::node_t::traverser( _root ).apply( &v );
            _world_bounds = v.get_world_box();
        }
    }

    //******************************************************************************************************
    virtual void_t on_graphics( motor::application::app::graphics_data_in_t gd ) noexcept
    {
        // absolutely required. this visitor bakes local
        // transformations, i.e. if animations occure or if
        // the local trafo is just commited to the usable
        // transformation by the trafo visitor
        // must be called in on graphics at the moment.
        {
            motor::scene::variable_update_visitor_t v;
            motor::scene::node_t::traverser( _root ).apply( &v );
        }

        auto * cam = _cameras[ _cam_id ].second;

// test
#if 0
        if( _draw_debug )
        {
            _prim_render->draw_arrow( motor::math::vec3f_t( 0.0f ),
                motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ), 10.0f,
                motor::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ) );

            _prim_render->draw_wire_circle( cam->get_camera_matrix(),
                motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ), 10.0f,
                motor::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ), 20 );
        }
#endif

        // use the debug_graphics_visitor_t and render the scene
        {
            if( _draw_debug )
            {
                motor::scene::debug_graphics_visitor_t::draw_context dc = {
                    cam, _prim_render, _draw_depth, _draw_leafs };

                motor::scene::debug_graphics_visitor_t v( dc );
                motor::scene::node_t::traverser( _root ).apply( &v );

                _max_depth = v.get_max_depth();
            }

            // render sun_cam frustum
            if( _draw_debug )
            {
            }

            _prim_render->set_view_proj( cam->get_view_matrix(), cam->get_proj_matrix() );
            _prim_render->prepare_for_rendering();
        }
    }

    //******************************************************************************************************
    virtual void_t on_first_frame( this_t::window_id_t const wid,
        motor::graphics::gen4::frontend_ptr_t fe,
        motor::application::app::render_data_in_t ) noexcept
    {
        _pp_pipe->init_render( fe );
        fe->configure< motor::graphics::state_object_t >( _final_so );
        fe->configure< motor::graphics::state_object_t >( _light_so );

        _prim_render->configure( fe );
    }

    //******************************************************************************************************
    virtual void_t on_last_frame( this_t::window_id_t const wid,
        motor::graphics::gen4::frontend_ptr_t fe,
        motor::application::app::render_data_in_t ) noexcept
    {
        _pp_pipe->release_render( fe );
        fe->release< motor::graphics::state_object_t >( _final_so );
        fe->release< motor::graphics::state_object_t >( _light_so );

        _prim_render->release( fe );
    }

    //******************************************************************************************************
    virtual void_t on_render( this_t::window_id_t const wid,
        motor::graphics::gen4::frontend_ptr_t fe,
        motor::application::app::render_data_in_t ) noexcept
    {
        _own_mmgr->on_render( fe );

        // render into final framebuffer
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

        _prim_render->prepare_for_rendering( fe );
        _prim_render->render( fe );
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
        size_t render_id = size_t( -1 );

        if( _cam_id != size_t( -1 ) )
        {
            fe->push( _final_so );

            if( _render_normals )
            {
                motor::scene::render_visitor_t rnd_vis(
                    this_file::to_id( this_file::msl_id::normal_pass_id ), ++render_id, fe,
                    _cameras[ _cam_id ].second );
                motor::scene::node_t::traverser( _root ).apply( &rnd_vis );
            }
            if( _render_default )
            {
                motor::scene::render_visitor_t rnd_vis(
                    this_file::to_id( this_file::msl_id::color_pass_id ), ++render_id, fe,
                    _cameras[ _cam_id ].second );
                motor::scene::node_t::traverser( _root ).apply( &rnd_vis );
            }

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
            if( _cameras.size() > 0 )
            {
                size_t i = 0;
                static ImGuiComboFlags flags = 0;
                motor::vector< char const * > items( _cameras.size() );
                for( auto const & e : _cameras )
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
                ImGui::Separator();
                auto * cam_ptr = _cameras[ _cam_id ].second;
                if( cam_ptr->is_orthographic() )
                    ImGui::Text( "Type : orthographic " );
                else if( cam_ptr->is_orthographic() )
                    ImGui::Text( "Type : perspecitve " );

                auto const nf = cam_ptr->get_near_far();
                ImGui::Text( "[Near; Far] : [ %f;%f]", nf.x(), nf.y() );
            }
        }

        if( ImGui::Begin( "Rendering" ) )
        {

            {
                ImGui::Checkbox( "Render Debug", &_draw_debug );
                ImGui::Checkbox( "Render Leafs", &_draw_leafs );
                {
                    int_t v = int_t( _draw_depth );
                    ImGui::SliderInt( "Render Level", &v, 0, int_t( _max_depth ) );
                    _draw_depth = size_t( v );
                }
                ImGui::Separator();
            }

            ImGui::Checkbox( "Render Light", &_render_light );
            ImGui::Checkbox( "Draw Sun Light Ortho", &_draw_sun_ortho );

            if( ImGui::Checkbox( "Render Normals", &_render_normals ) )
            {
                if( _render_normals )
                {
                    _render_default = false;
                }
            }

            if( ImGui::Checkbox( "Render Default", &_render_default ) )
            {
                if( _render_default )
                {
                    _render_normals = false;
                }
            }

            {
                static char const * const labels[] = { "Shadow depth", "RSM positions (color 0)",
                    "RSM normals (color 1)", "HDR color 0", "HDR color 1", "HDR color 2",
                    "HDR color 3", "HDR depth" };
                static char const * const textures[] = {
                    "02_camera_fit_shadow_mapping.shadow_depth_framebuffer.depth",
                    "02_camera_fit_shadow_mapping.shadow_depth_framebuffer.0",
                    "02_camera_fit_shadow_mapping.shadow_depth_framebuffer.1",
                    "gfx.postprocess.hdr.framebuffer.0.0", "gfx.postprocess.hdr.framebuffer.0.1",
                    "gfx.postprocess.hdr.framebuffer.0.2", "gfx.postprocess.hdr.framebuffer.0.3",
                    "gfx.postprocess.hdr.framebuffer.0.depth" };
                static_assert( IM_ARRAYSIZE( labels ) == IM_ARRAYSIZE( textures ) );

                ImGui::Separator();

                ImGui::Checkbox( "Temp Render Target", &_show_temp_rt );

                if( ImGui::ListBox(
                        "Render target", &_temp_rt_index, labels, IM_ARRAYSIZE( labels ), 5 ) )
                {
                }

                if( _show_temp_rt )
                {
                    _pp_pipe->set_map_to_screen_texture_temp( textures[ _temp_rt_index ] );
                }
            }
        }
        ImGui::End();

        if( ImGui::Begin( "Post Process" ) )
        {
            bool_t has_changed = false;
            for( auto ps : _pp_pipe->property_sheets() )
            {
                has_changed |= motor::tool::imgui_property::handle( ps.first, *ps.second );
            }
            if( has_changed ) _pp_pipe->update_properies();

            {
                ImGui::Checkbox( "Show temp render target ##scene_manager", &_show_temp_rt );
            }
        }
        ImGui::End();

        if( ImGui::Begin( "Variables" ) )
        {
            {
                float_t v = _intensity->get_value();
                if( ImGui::SliderFloat( "Intensity", &v, 0.0f, 100.0f ) )
                {
                    _intensity->set_and_exchange( v );
                }
            }
        }
        ImGui::End();

        if( ImGui::Begin( "Detection" ) )
        {
            {
                ImGui::SameLine();
                low_onset_widget.draw( "40\n250\nHz", kix_detector );
            }
            {
                ImGui::SameLine();
                mid_onset_widget.draw( "250\n2000\nHz", mid_detector );
            }
            {
                float_t peak = 0.0000000001f;
                for( float_t const value : captured_frequencies )
                {
                    peak = std::max( peak, value );
                }
                ImGui::PlotHistogram( "Squared amplitude", captured_frequencies.data(),
                    int_t( captured_frequencies.size() ) >> 2, 0, nullptr, 0.0f, peak * 1.05f,
                    ImVec2( ImGui::GetContentRegionAvail().x, 50.0f ) );
            }
            {
                float_t peak = 0.00001f;
                for( float_t const sample : captured_samples )
                {
                    peak = std::max( peak, std::abs( sample ) );
                }
                float_t const scale = peak * 1.05f;
                ImGui::PlotLines( "Samples", captured_samples.data(),
                    int_t( captured_samples.size() ), 0, nullptr, -scale, scale,
                    ImVec2( ImGui::GetContentRegionAvail().x, 50.0f ) );
            }
        }
        ImGui::End();

        // #1 : nodes graph window
        if( ImGui::Begin( "node graph visualization" ) )
        {
            _imn_wire.begin();
            _imn_wire.visualize( 0 );
            _imn_wire.end( true );
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
        motor::wire::release( motor::move( _start_node ) );
        motor::wire::release( motor::move( _end_node ) );

        motor::release( motor::move( _final_so ) );
        motor::release( motor::move( _light_so ) );

        motor::release( motor::move( _root ) );

        this_t::release_cameras();
        this_t::release_node_dump();

        motor::release( motor::move( _own_mmgr ) );

        

        motor::release( motor::move( _prim_render ) );
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
