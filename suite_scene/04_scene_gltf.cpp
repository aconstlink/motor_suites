



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

    class my_app : public motor::application::app
    {
        motor_this_typedefs( my_app ) ;

        motor::math::vec4ui_t fb_dims = motor::math::vec4ui_t( 0, 0, 1920, 1080 ) ;

        motor::scene::node_mtr_t _root ;
        motor::scene::node_mtr_t _selected = nullptr ;
        motor::property::property_sheet_t _props ;

        motor::graphics::state_object_mtr_t root_so ;

        motor::wire::output_slot< float_t > * _time = motor::shared( motor::wire::output_slot< float_t >( 0.0f ) ) ;

        size_t _cam_id = 0 ;

        // 0 : this is the free moving camera
        // 1 : second camera for testing shader variable bindings
        motor::gfx::generic_camera_mtr_t _cameras[2] ;


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
                cam.perspective_fov( motor::math::angle<float_t>::degree_to_radian( 30.0f ) ) ;
                cam.look_at( motor::math::vec3f_t( 0.0f, 0.0f, -20.0f ),
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
                cam.perspective_fov( motor::math::angle<float_t>::degree_to_radian( 90.0f ) ) ;
                cam.look_at( motor::math::vec3f_t( -50.0f, 20.0f, -100.0f ),
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
                    rss.polygon_s.ss.fm = motor::graphics::fill_mode::line ;
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
                            auto item = mod_reg->import_from( motor::io::location_t( "gltf.scaled_cube.scaled_cube.gltf" ), &db ) ;

                            auto * ret_item = item.get() ;

                            // test scene with visitor
                            if( auto * scene_item = dynamic_cast<motor::format::scene_item_ptr_t>( ret_item ); scene_item!= nullptr )
                            {
                                imported_node = motor::move( scene_item->root ) ;
                            }

                            motor::release( motor::move( ret_item ) ) ;
                        }
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
                float_t const t = _time->get_value() + d.sec_dt * 0.5f ;
                _time->set_and_exchange( t > 1.0f ? 0.0f : t ) ;
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
                motor::scene::render_visitor_t vis( wid, fe, _cameras[_cam_id] ) ;
                motor::scene::node_t::traverser(_root).apply( &vis ) ;
            }

        }

        //******************************************************************************************************
        virtual void_t on_update( motor::application::app::update_data_in_t ) noexcept 
        {
            MOTOR_PROBE( "application", "on_update" ) ;

            {
                motor::scene::trafo_visitor_t v ;
                motor::scene::node_t::traverser( _root ).apply( &v ) ;
            }

            {
                motor::scene::variable_update_visitor_t v ;
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
                    motor::tool::imgui_node_visitor_t v( motor::move( _selected ) ) ;
                    motor::scene::node_t::traverser( _root ).apply( &v ) ;
                    _selected = v.get_selected() ;
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
            motor::release( motor::move( _time ) ) ;
            motor::release( motor::move( _root ) ) ;
            motor::release( motor::move( root_so ) ) ;

            motor::release( motor::move( _selected ) ) ;
            motor::release( motor::move( _cameras[0] ) ) ;
            motor::release( motor::move( _cameras[1] ) ) ;
        }
    };
}

int main( int argc, char ** argv )
{
    return motor::platform::global_t::create_and_exec< this_file::my_app >() ;
}