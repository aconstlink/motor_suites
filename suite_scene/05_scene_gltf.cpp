



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

#include <motor/gfx/camera/generic_camera.h>
#include <motor/math/utility/angle.hpp>

#include <motor/scene/node/logic_group.h>
#include <motor/scene/node/logic_leaf.h>

#include <motor/scene/component/name_component.hpp>
#include <motor/scene/component/msl_component.h>
#include <motor/scene/component/render_settings_component.h>
#include <motor/scene/component/trafo3d_component.h>
#include <motor/scene/component/camera_component.h>

#include <motor/scene/visitor/trafo_visitor.h>
#include <motor/scene/visitor/render_visitor.h>
#include <motor/scene/visitor/variable_update_visitor.h>

#include <motor/wire/variables/trafo_variables.hpp>
#include <motor/wire/variables/vector_variables.hpp>

#include <motor/math/interpolation/interpolate.hpp>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>

#include <future>

namespace this_file
{    
    using namespace motor::core::types ;    

    class collect_cameras : public motor::scene::default_visitor
    {
        motor_this_typedefs( collect_cameras ) ;

    private:

        using camera_entry_t = std::pair< motor::string_t, motor::gfx::generic_camera_mtr_t > ;
        using cameras_t = motor::vector< camera_entry_t > ;

        cameras_t _cameras ;

        virtual motor::scene::result visit( motor::scene::node_ptr_t nptr ) noexcept 
        {
            motor::string_t name = "" ;
            {
                motor::scene::name_component_mtr_t comp ;
                if( nptr->has_component_and_borrow( comp ) )
                {
                    name = comp->get_name() ;
                }
            }
            {
                motor::scene::camera_component_mtr_t comp;
                if (nptr->has_component_and_borrow(comp)) 
                {
                    this_t::camera_entry_t e(name, comp->get_camera() );
                    _cameras.emplace_back( std::move(e) ) ;
                }
            }

            return motor::scene::result::ok ;
        }

        virtual motor::scene::result post_visit( motor::scene::node_ptr_t nptr, motor::scene::result const res ) noexcept 
        {
            return motor::scene::result::ok ;
        }

    public:

        cameras_t get_cameras( void_t ) noexcept
        {
            return _cameras ;
        }

    };

    class my_app : public motor::application::app
    {
        motor_this_typedefs( my_app ) ;

        motor::math::vec4ui_t fb_dims = motor::math::vec4ui_t( 0, 0, 1920, 1080 ) ;

        motor::scene::node_mtr_t _root ;
        motor::scene::node_mtr_t _selected = nullptr ;
        motor::property::property_sheet_t _props ;

        motor::graphics::state_object_mtr_t root_so ;

        using os_float_t = motor::wire::output_slot< float_t > ;
        using os_trafo_t = motor::wire::output_slot< motor::math::m3d::trafof_t > ;

        os_float_t * _time = motor::shared( os_float_t( 0.0f ) ) ;
        os_trafo_t * _scale_os = motor::shared( os_trafo_t() ) ;

        motor::wire::time_node_mtr_t _time_node ; // from the importer
        motor::wire::inode_mtr_t _merger ; // from the importer

        size_t _cam_id = 0 ;

        // 0 : this is the free moving camera
        // 1 : second camera for testing shader variable bindings
        motor::gfx::generic_camera_mtr_t _cameras[2] ;

        motor::gfx::generic_camera_mtr_t _selected_cam = nullptr ;


        //******************************************************************************************************
        virtual void_t on_init( void_t ) noexcept
        {
            MOTOR_PROBE( "application", "on_init" ) ;

            // #1 : init window
            #if 1
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
                    wnd.send_message( motor::application::cursor_message_t( {true} ) ) ;
                    wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                } ) ;
            }
            #endif
            // #2 : init window
            #if 1
            {
                motor::application::window_info_t wi ;
                wi.x = 500 ;
                wi.y = 100 ;
                wi.w = 800 ;
                wi.h = 600 ;
                wi.gen = motor::application::graphics_generation::gen4_gl4 ;

                this_t::send_window_message( this_t::create_window( wi ), [&] ( motor::application::app::window_view & wnd )
                {
                    wnd.send_message( motor::application::show_message( { true } ) ) ;
                    wnd.send_message( motor::application::cursor_message_t( { true } ) ) ;
                    wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                } ) ;
            }            
            #endif
            // camera
            {
                auto cam = motor::gfx::generic_camera_t( 1.0f, 1.0f, 0.1f, 1000.0f ) ;
                #if 1
                cam.perspective_fov( motor::math::angle<float_t>::degree_to_radian( 45.0f ) ) ;
                cam.look_at( motor::math::vec3f_t( 0.0f, 0.0f, 50.0f ),
                    motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ), motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) ) ;
                #else
                cam.orthographic() ;
                cam.look_at( motor::math::vec3f_t( 0.0f, 0.0f, -100.0f ),
                    motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ), motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) ) ;
                #endif
                _cameras[0] = motor::shared( std::move( cam ) ) ;
            }

            // camera
            {
                auto cam = motor::gfx::generic_camera_t( 1.0f, 1.0f, 1.0f, 100.0f ) ;
                cam.perspective_fov( motor::math::angle<float_t>::degree_to_radian( 45.0f ) ) ;
                cam.look_at( motor::math::vec3f_t( -50.0f, 00.0f, 0.0f ),
                    motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ), motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) ) ;

                _cameras[1] = motor::shared( std::move( cam ) ) ;
            }            
            
            {
                motor::graphics::state_object_t so = motor::graphics::state_object_t(
                    "root_render_states" ) ;

                {
                    motor::graphics::render_state_sets_t rss ;
                    rss.depth_s.do_change = true ;
                    rss.depth_s.ss.do_activate = true ;
                    rss.depth_s.ss.do_depth_write = true ;
                    rss.polygon_s.do_change = true ;
                    rss.polygon_s.ss.do_activate = true ;
                    rss.polygon_s.ss.fm = motor::graphics::fill_mode::fill ;
                    rss.polygon_s.ss.ff = motor::graphics::front_face::counter_clock_wise ;
                    rss.polygon_s.ss.cm = motor::graphics::cull_mode::back ;
                    rss.clear_s.do_change = true ;
                    rss.clear_s.ss.clear_color = motor::math::vec4f_t( 0.5f, 0.9f, 0.5f, 1.0f ) ;
                    rss.clear_s.ss.do_activate = true ;
                    rss.clear_s.ss.do_color_clear = true ;
                    rss.clear_s.ss.do_depth_clear = true ;
                    rss.view_s.do_change = true ;
                    rss.view_s.ss.do_activate = false ;
                    rss.view_s.ss.vp = motor::math::vec4ui_t( 0, 0, 500, 500 ) ;
                    so.add_render_state_set( rss ) ;
                }

                root_so = motor::shared( motor::graphics::state_object_t( std::move( so ) ) ) ;
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
                motor::scene::logic_group_t root ;
                root.add_component( motor::shared( motor::scene::name_component_t( "my root name" ) ) ) ;
                
                // add imported scene
                {
                    motor::scene::node_mtr_t imported_node = nullptr ;

                    auto rs = motor::shared( motor::scene::logic_group_t(  ) ) ;
                    {
                        auto rsc = motor::scene::render_settings_component_t( motor::share( root_so ) ) ;

                        rs->add_component( motor::shared( std::move( rsc) ) ) ;

                        rs->add_component(motor::shared(
                          motor::scene::name_component_t("Render Settings")));
                    }

                    // make importer ready
                    {
                        motor::format::module_registry_mtr_t mod_reg = motor::format::global::register_default_modules( 
                        motor::shared( motor::format::module_registry_t(), "mod registry"  ) ) ;

                        motor::io::database_t db = motor::io::database_t( motor::io::path_t( DATAPATH ), "./working", "data" ) ;

                        // import the gltf asset.
                        {
                            //auto item = mod_reg->import_from( motor::io::location_t( "gltf.2CylinderEngine.glTF.2CylinderEngine.gltf" ), &db ) ;
                            //auto item = mod_reg->import_from( motor::io::location_t( "gltf.AntiqueCamera.glTF.AntiqueCamera.gltf" ), &db ) ;
                            //auto item = mod_reg->import_from( motor::io::location_t( "gltf.box.glTF.Box.gltf" ), &db ) ;
                            //auto item = mod_reg->import_from( motor::io::location_t( "gltf.simple_camera.simple_camera.gltf" ), &db ) ;
                            //auto item = mod_reg->import_from( motor::io::location_t( "gltf.scaled_cube.scaled_cube.gltf" ), &db ) ;
                            //auto item = mod_reg->import_from( motor::io::location_t( "gltf.BoomBox.BoomBox.gltf" ), &db ) ;
                            
                            //auto item = mod_reg->import_from( motor::io::location_t( "gltf.ABeautifulGame.ABeautifulGame.gltf" ), &db ) ;
                            //auto item = mod_reg->import_from( motor::io::location_t( "gltf.AnimatedTriangle.AnimatedTriangle.gltf" ), &db ) ;
                            //auto item = mod_reg->import_from( motor::io::location_t( "gltf.CesiumMilkTruck.CesiumMilkTruck.gltf" ), &db ) ;

                            //auto item = mod_reg->import_from( motor::io::location_t( "gltf.some_tests.gears_corrected.gltf" ), &db ) ;
                            auto item = mod_reg->import_from( motor::io::location_t( "gltf.some_tests.test.gltf" ), &db ) ;
                            //auto item = mod_reg->import_from( motor::io::location_t( "gltf.some_tests.animated_cube.gltf" ), &db ) ;
                            //auto item = mod_reg->import_from( motor::io::location_t( "gltf.some_tests.BoxAnimated.gltf" ), &db ) ;
                            //auto item = mod_reg->import_from( motor::io::location_t( "gltf.some_tests.camera_on_path_and_lookat.gltf" ), &db ) ;
                            //auto item = mod_reg->import_from( motor::io::location_t( "gltf.some_tests.scene2.gltf" ), &db ) ;
                            

                            auto * ret_item = item.get() ;

                            // test scene with visitor
                            if( auto * scene_item = dynamic_cast<motor::format::scene_item_ptr_t>( ret_item ); scene_item!= nullptr )
                            {
                                imported_node = motor::move( scene_item->root ) ;
                                _time_node = motor::move( scene_item->start_node ) ;
                                _merger = motor::move( scene_item->merger_node ) ;

                                _time_node->borrow_time_is()->connect( motor::share(_time) ) ;
                            }
                            else
                            {
                                motor::log::global_t::critical("Failed to load gltf file.") ;
                                std::exit(1) ;
                            }

                            motor::release( motor::move( ret_item ) ) ;
                        }

                        motor::release( motor::move( mod_reg ) ) ;
                    }

                    // test and scale whole imported tree with
                    // only one trafo component.
                    {
                        motor::math::m3d::trafof_t t ;
                        t.set_scale( motor::math::vec3f_t( 1.0f ) ) ;

                        motor::scene::trafo3d_component_t comp ;
                        comp.set_trafo_local( t ) ;

                        {
                            motor::wire::inputs_t inputs ;
                            comp.inputs( inputs ) ;
                            inputs.connect( "trafo", motor::share( _scale_os ) ) ;
                        }

                        imported_node->add_component( motor::shared( std::move( comp ) ) ) ;
                    }
                    rs->add_child( motor::move( imported_node ) ) ;
                    root.add_child( motor::move( rs ) ) ;
                }

                _root = motor::shared( std::move( root ) ) ;
            }
        }

        //******************************************************************************************************
        virtual void_t on_event( window_id_t const wid, 
                motor::application::window_message_listener::state_vector_cref_t sv ) noexcept
        {
            MOTOR_PROBE( "application", "on_event" ) ;

            if( sv.create_changed )
            {
                motor::log::global_t::status("[my_app] : window created") ;
            }
            if( sv.close_changed )
            {
                motor::log::global_t::status("[my_app] : window closed") ;
                this->close() ;
            }
            if ( sv.resize_changed )
            {
                float_t const w = float_t( sv.resize_msg.w ) ;
                float_t const h = float_t( sv.resize_msg.h ) ;
                for( size_t i=0; i<2; ++i )
                {
                    _cameras[ i ]->set_sensor_dims( w, h ) ;
                    _cameras[ i ]->perspective_fov() ;
                }
            }
        }

        //******************************************************************************************************
        virtual void_t on_graphics( motor::application::app::graphics_data_in_t d ) noexcept 
        {
            {
                static float_t t_ = 0.0f ;
                t_ += d.sec_dt ;

                float_t t = t_ ;
                t = motor::math::fn<float_t>::mod( t, 12.0f ) ;
                //t = (motor::math::fn<float_t>::mod( t, 8.0f ) / 4.0f)-1.0f ;
                //t = 1.0f - std::abs( t ) ;

                _time->set_and_exchange( t) ;
            }

            {
                motor::concurrent::global_t::schedule( _time_node->get_task(), motor::concurrent::schedule_type::pool ) ;
            }
        } 

        //******************************************************************************************************
        virtual void_t on_render( this_t::window_id_t const wid, motor::graphics::gen4::frontend_ptr_t fe,
            motor::application::app::render_data_in_t rd ) noexcept
        {
            // configure needs to be done only once per window
            if ( rd.first_frame )
            {
                fe->configure<motor::graphics::state_object_t>( root_so ) ;
            }            
            
            {
                motor::gfx::generic_camera_mtr_t cam = _selected_cam == nullptr ? _cameras[_cam_id] : _selected_cam ;
                //cam->set_dims( 1000.0f, 1000.0f, 1.0f, 1000.0f) ;
                motor::scene::render_visitor_t vis( wid, fe, cam ) ;
                motor::scene::node_t::traverser(_root).apply( &vis ) ;
            }

        }

        //******************************************************************************************************
        virtual void_t on_update( motor::application::app::update_data_in_t ) noexcept 
        {
            MOTOR_PROBE( "application", "on_update" ) ;

            {
                motor::scene::variable_update_visitor_t v ;
                motor::scene::node_t::traverser( _root ).apply( &v ) ;
            }

            {
                motor::scene::trafo_visitor_t v ;
                motor::scene::node_t::traverser( _root ).apply( &v ) ;
            }
        } 

        //******************************************************************************************************
        virtual bool_t on_tool( this_t::window_id_t const wid, motor::application::app::tool_data_ref_t ) noexcept 
        { 
            MOTOR_PROBE( "application", "on_tool" ) ;

            {
                if( ImGui::Begin("Scene Graph Window") )
                {
                    auto t = _scale_os->get_value() ;
                    float_t f = t.get_scale().x() ;
                    if( ImGui::SliderFloat( "Scene Scale small", &f, 0.1f, 30.0f )  )
                    {
                        _scale_os->set_and_exchange( t.set_scale( f ) ) ;
                    }

                    if( ImGui::SliderFloat( "Scene Scale large", &f, 30.0f, 300.0f )  )
                    {
                        _scale_os->set_and_exchange( t.set_scale( f ) ) ;
                    }

                    {
                        motor::tool::imgui_node_visitor_t v( motor::move( _selected ) ) ;
                        motor::scene::node_t::traverser( _root ).apply( &v ) ;
                        _selected = v.move_selected() ;
                    }
                }
                ImGui::End() ;

                if ( ImGui::Begin( "Camera Window" ) )
                {
                    {
                        int used_cam = int_t( _cam_id ) ;
                        ImGui::SliderInt( "Choose Camera", &used_cam, 0, 1 ) ;
                        _cam_id = std::min( size_t( used_cam ), size_t( 2 ) ) ;
                    }

                    {
                        auto const cam_pos = _cameras[_cam_id]->get_position() ;
                        float x = cam_pos.x() ;
                        float y = cam_pos.y() ;
                        ImGui::SliderFloat( "Cur Cam X", &x, -100.0f, 100.0f ) ;
                        ImGui::SliderFloat( "Cur Cam Y", &y, -100.0f, 100.0f ) ;
                        _cameras[_cam_id]->translate_to( motor::math::vec3f_t( x, y, cam_pos.z() ) ) ;
                        
                    }

                    // choose camera from scene
                    {
                        this_file::collect_cameras cc ;
                        motor::scene::node_t::traverser(_root).apply( &cc ) ;

                        auto cams = cc.get_cameras() ;

                        

                        {
                            size_t i=1; 
                            static ImGuiComboFlags flags = 0;
                            motor::vector< char const * > items( cams.size() + 1 ) ;
                            for( auto const & e : cams )
                            {
                                items[i++] = e.first.c_str() ;
                            }
                            items[0] = "free" ;

                            static int item_selected_idx = 0; 
                            const char* combo_preview_value = items[item_selected_idx];
                            if (ImGui::BeginCombo("Scene Camera", combo_preview_value, flags))
                            {
                                for (int n = 0; n < items.size(); n++)
                                {
                                    const bool is_selected = (item_selected_idx == n);
                                    if (ImGui::Selectable(items[n], is_selected))
                                        item_selected_idx = n;

                                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                                    if (is_selected)
                                        ImGui::SetItemDefaultFocus();
                                }
                                ImGui::EndCombo();


                                {
                                    motor::release( motor::move( _selected_cam ) ) ;
                                    if( item_selected_idx != 0 ) 
                                        _selected_cam = motor::move( cams[item_selected_idx-1].second ) ;
                                }
                            }
                        }

                        
                        
                        for( auto e : cams )
                        {
                            motor::release( motor::move( e.second ) ) ;
                        }
                    }
                }
                ImGui::End() ;


                // property window
                {
                    if ( ImGui::Begin( "Property Window" ) )
                    {
                        motor::wire::inputs_t inps ;

                        if( _selected != nullptr && _selected->inputs( inps ) )
                        {
                            inps.for_each_slot( [&] ( motor::string_in_t name, motor::wire::iinput_slot_ptr_t is )
                            {
                                if ( motor::property::add_is_property< float_t >( name, is, _props ) ) return ;
                                if ( motor::property::add_is_property< int_t >( name, is, _props ) ) return ;
                                if ( motor::property::add_is_property< motor::math::vec2f_t >( name, is, _props ) )
                                {
                                    using is_t = motor::wire::input_slot<motor::math::vec2f_t> ;
                                    auto p = _props.borrow_property<is_t>( name ) ;
                                    p->replace_is( motor::share( is ), true ) ;

                                    return ;
                                }
                                if ( motor::property::add_is_property< motor::math::vec3f_t >( name, is, _props ) ) return ;
                                if ( motor::property::add_is_property< motor::math::vec4f_t >( name, is, _props ) ) return ;
                            } ) ;

                            motor::tool::imgui_property::handle( "Property Sheet", _props ) ;
                        }
                    }
                    ImGui::End() ;
                }
            }
            
            return true ; 
        }

        virtual void_t on_shutdown( void_t ) noexcept 
        {
            // release your own slots with wire::release!
            // motor::release will not resolve x-connections.
            motor::wire::release( motor::move( _time ) ) ;
            motor::wire::release( motor::move( _scale_os ) ) ;

            motor::release( motor::move( _root ) ) ;
            motor::release( motor::move( root_so ) ) ;
            

            motor::release( motor::move( _selected ) ) ;
            motor::release( motor::move( _cameras[0] ) ) ;
            motor::release( motor::move( _cameras[1] ) ) ;

            motor::release( motor::move( _time_node  ) ) ;
            motor::release( motor::move( _merger ) ) ;

            motor::release( motor::move( _selected_cam ) ) ;
        }
    };
}

int main( int argc, char ** argv )
{
    return motor::platform::global_t::create_and_exec< this_file::my_app >() ;
}