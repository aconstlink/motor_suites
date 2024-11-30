



#include <motor/profiling/probe_guard.hpp>

#include <motor/tool/imgui/node_kit/imgui_node_visitor.h>
#include <motor/tool/imgui/imgui_property.h>

#include <motor/platform/global.h>

#include <motor/geometry/mesh/tri_mesh.h>
#include <motor/geometry/mesh/flat_tri_mesh.h>
#include <motor/geometry/3d/cube.h>

#include <motor/controls/types/ascii_keyboard.hpp>
#include <motor/controls/types/three_mouse.hpp>

#include <motor/gfx/camera/generic_camera.h>
#include <motor/math/utility/angle.hpp>

#include <motor/scene/node/logic_group.h>
#include <motor/scene/node/logic_leaf.h>
#include <motor/scene/node/camera_node.h>
#include <motor/scene/node/trafo3d_node.h>
#include <motor/scene/node/render_node.h>
#include <motor/scene/node/render_settings.h>

#include <motor/scene/component/name_component.hpp>
#include <motor/scene/component/msl_component.h>
#include <motor/scene/component/render_state_component.h>
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
        motor::graphics::msl_object_mtr_t msl_obj ;
        motor::graphics::geometry_object_t geo_obj ;

        motor::wire::output_slot< float_t > * _time = motor::shared( motor::wire::output_slot< float_t >( 0.0f ) ) ;

        motor::wire::output_slot< motor::math::m3d::trafof_t > * _out_0 = nullptr ;
        motor::wire::output_slot< motor::math::m3d::trafof_t > * _out_1 = nullptr ;

        motor::wire::trafo3fv_t _trafo = motor::wire::trafo3fv_t("trafo") ;
        motor::wire::vec3fv_t _pos = motor::wire::vec3fv_t("position") ;

        size_t _cam_id = 0 ;

        // 0 : this is the free moving camera
        // 1 : second camera for testing shader variable bindings
        motor::gfx::generic_camera_mtr_t _cameras[2] ;


        //******************************************************************************************************
        virtual void_t on_init( void_t ) noexcept
        {
            MOTOR_PROBE( "application", "on_init" ) ;

            motor::tool::imgui_node_visitor_t::init_function_callbacks() ;

            // #1 : init window
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

            // #2 : init window
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

            // camera
            {
                auto cam = motor::gfx::generic_camera_t( 1.0f, 1.0f, 0.1f, 500.0f ) ;
                cam.perspective_fov( motor::math::angle<float_t>::degree_to_radian( 30.0f ) ) ;
                cam.look_at( motor::math::vec3f_t( 0.0f, 0.0f, -400.0f ),
                    motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ), motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) ) ;

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
            
            // msl object for scene
            {
                {
                    motor::graphics::msl_object_t mslo( "scene_obj" ) ;

                    mslo.add( motor::graphics::msl_api_type::msl_4_0, R"(
                        config just_render
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
                                    pos.xyz = pos.xyz * 10.0;
                                    out.tx = in.tx ;
                                    out.pos = proj * view * world * vec4_t( pos, 1.0 ) ;
                                    out.nrm = normalize( world * vec4_t( in.nrm, 0.0 ) ).xyz ;
                                }
                            }

                            pixel_shader
                            {
                                tex2d_t tex ;
                                vec4_t color ;
                                vec3_t light_dir ;
                                float_t time ;

                                in vec2_t tx : texcoord ;
                                in vec3_t nrm : normal ;
                                out vec4_t color : color0 ;
                                //out vec4_t color1 : color1 ;
                                //out vec4_t color2 : color2 ;

                                void main()
                                {
                                    float_t light = dot( normalize( in.nrm ), normalize( light_dir ) ) ;
                                    out.color = vec4_t( light*time, light, light, 1.0 ) ;

                                    //out.color = color ' texture( tex, in.tx ) ;
                                    //out.color1 = vec4_t( in.nrm, 1.0 ) ;
                                    //out.color2 = vec4_t( light, light, light , 1.0 ) ;
                                }
                            }
                        })" ) ;

                    mslo.link_geometry( { "cube" } ) ;

                    msl_obj = motor::shared( motor::graphics::msl_object_t( std::move( mslo ) ) ) ;
                }
            }
            

            {
                motor::graphics::state_object_t so = motor::graphics::state_object_t(
                    "root_render_states" ) ;

                {
                    motor::graphics::render_state_sets_t rss ;
                    rss.depth_s.do_change = true ;
                    rss.depth_s.ss.do_activate = false ;
                    rss.depth_s.ss.do_depth_write = true ;
                    rss.polygon_s.do_change = true ;
                    rss.polygon_s.ss.do_activate = true ;
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

            {
                _out_0 = motor::shared( motor::wire::output_slot< motor::math::m3d::trafof_t >(), "output 0" ) ;
                _out_0->set_value( motor::math::m3d::trafof_t(
                    motor::math::vec3f_t( 1.0f ),
                    motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ),
                    motor::math::vec3f_t( -50.0f, 30.0f, 0.0f ) ) ) ;

                _out_1 = motor::shared( motor::wire::output_slot< motor::math::m3d::trafof_t >(), "output 1" );
                _out_1->set_value( motor::math::m3d::trafof_t(
                    motor::math::vec3f_t( 1.0f ),
                    motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ),
                    motor::math::vec3f_t( 50.0f, 0.0f, 0.0f ) ) ) ;


                _pos = motor::wire::vec3fv_t( "position", _trafo.get_value().get_translation() ) ;

                _trafo.connect( motor::share( _out_1 ) ) ;
                _trafo.inputs().connect( "trafo.position", _pos.get_os() ) ;
            }

            // #3 : init scene tree
            {
                motor::scene::logic_group_t root ;
                root.add_component( motor::shared( motor::scene::name_component_t( "my root name" ) ) ) ;

                // add camera
                { 
                    auto cam = motor::shared( motor::scene::camera_node_t( 
                        motor::shared( motor::gfx::generic_camera_t( 800.0f, 600.0f, 1.0f, 100.0f ) ) ) ) ;

                    cam->add_component( motor::shared( motor::scene::name_component_t( "Camera" ) ) ) ;
                    root.add_child( motor::move( cam ) ) ;
                }

                {
                    // add transformation node g
                    auto t = motor::shared( motor::scene::trafo3d_node_t( 
                        motor::math::m3d::trafof_t(
                            motor::math::vec3f_t( 1.0f ),
                            motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ),
                            motor::math::vec3f_t( 0.0f, -10.0f, 0.0f ) ) ) ) ;

                    {
                        t->add_component( motor::shared( motor::scene::name_component_t( "trafo node 1" ) ) ) ;
                    }

                    // add render settings
                    {
                        auto rs = motor::shared( motor::scene::render_settings_t( motor::share( root_so ) ) ) ;
                        rs->add_component( motor::shared( motor::scene::name_component_t( "Render Settings" ) ) ) ;

                        {
                            motor::scene::logic_group_t g ;

                            {
                                // add transformation node g
                                auto t2 = motor::shared( motor::scene::trafo3d_node_t() ) ;
                                
                                // name component
                                {
                                    t2->add_component( motor::shared( motor::scene::name_component_t( "trafo left" ) ) ) ;
                                }

                                // animation connect
                                {
                                    motor::wire::inputs_t inputs ;
                                    if( t2->inputs( inputs ) ) 
                                    {
                                        inputs.connect( "trafo", motor::share( _out_0 ) ) ;
                                    }
                                }

                                auto rn = motor::scene::render_node_t( motor::share(msl_obj), 0 ) ;
                                rn.add_component( motor::shared( motor::scene::name_component_t( "Render Object 0" ) ) ) ;

                                // add shader variable slots
                                {
                                    auto & inputs = *rn.borrow_shader_inputs() ;
                                    {
                                        auto s = motor::wire::input_slot( motor::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ) ) ;
                                        inputs.add( "color", motor::shared( std::move( s ) ) ) ;
                                    }
                                    {
                                        auto s = motor::wire::input_slot( motor::math::vec3f_t( 1.0f, 1.0f, -0.5f ) ) ;
                                        inputs.add( "light_dir", motor::shared( std::move( s ) ) ) ;
                                    }
                                    {
                                        auto s = motor::shared( motor::wire::input_slot( 0.0f ) ) ;
                                        s->connect( motor::share( _time ) ) ;
                                        inputs.add( "time", motor::move( s ) ) ;
                                    }
                                    #if 0
                                    // @note will be set by transformation visitor
                                    {
                                        motor::math::m3d::trafof_t trafo ;
                                        trafo.set_translation( motor::math::vec3f_t( 50.0f, 0.0f, 0.0f ) ) ;

                                        auto s = motor::wire::input_slot( trafo.get_transformation() ) ;
                                        inputs.add( "world", motor::shared( std::move( s ) ) ) ;
                                    }
                                    #endif
                                }

                                t2->set_decorated( motor::shared( std::move( rn ) ) ) ;
                                g.add_child( motor::move( t2 ) ) ;
                            }
                            
                            {
                                // add transformation node g
                                auto t2 = motor::shared( motor::scene::trafo3d_node_t() ) ;
                                
                                {
                                    t2->add_component( motor::shared( motor::scene::name_component_t( "trafo right" ) ) ) ;
                                }

                                // animation connect
                                {
                                    motor::wire::inputs_t inputs ;
                                    if ( t2->inputs( inputs ) )
                                    {
                                        //inputs.connect( "trafo", motor::share( _out_1 ) ) ;
                                        inputs.connect("trafo", _trafo.get_value_os() ) ;
                                    }
                                }

                                auto rn = motor::scene::render_node_t( motor::share( msl_obj ), 1 ) ;
                                rn.add_component( motor::shared( motor::scene::name_component_t( "Render Object 1" ) ) ) ;

                                // add shader variable slots
                                {
                                    auto & inputs = *rn.borrow_shader_inputs() ;
                                    {
                                        auto s = motor::wire::input_slot( motor::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ) ) ;
                                        inputs.add( "color", motor::shared( std::move( s ) ) ) ;
                                    }
                                    {
                                        auto s = motor::wire::input_slot( motor::math::vec3f_t( 1.0f, 1.0f, -0.5f ) ) ;
                                        inputs.add( "light_dir", motor::shared( std::move( s ) ) ) ;
                                    }
                                    {
                                        auto s = motor::shared( motor::wire::input_slot( 0.0f ) ) ;
                                        s->connect( motor::share( _time ) ) ;
                                        inputs.add( "time", motor::move( s ) ) ;
                                    }
                                    #if 0
                                    // @note will be set by transformation visitor
                                    {
                                        motor::math::m3d::trafof_t trafo ;
                                        trafo.set_translation( motor::math::vec3f_t( 50.0f, 0.0f, 0.0f ) ) ;

                                        auto s = motor::wire::input_slot( trafo.get_transformation() ) ;
                                        inputs.add( "world", motor::shared( std::move( s ) ) ) ;
                                    }
                                    #endif
                                }

                                t2->set_decorated( motor::shared( std::move( rn ) ) ) ;
                                g.add_child( motor::move( t2 ) ) ;
                            }
                            
                            rs->set_decorated(  motor::shared( std::move( g ) ) ) ;
                        }
                        
                        t->set_decorated( motor::move( rs ) ) ;
                    }

                    root.add_child( motor::move( t ) ) ;
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

            {
                auto * os = _out_0;

                static float_t angle = 0.0f ;
                angle += d.sec_dt * motor::math::constants<float_t>::pix2() ;
                angle = angle > motor::math::constants<float_t>::pix2() ? 0.0f : angle ;

                static float_t ts = 0.0f ;
                ts += d.sec_dt * 0.5f ;
                ts = ts > 1.0f ? 0.0f : ts ;

                float_t const ani_t = 1.0f - motor::math::fn<float_t>::abs( ts * 2.0f - 1.0f ) ;

                float_t const s = motor::math::interpolation<float_t>::linear( 1.0f, 5.0f, ani_t ) ;

                motor::math::vec3f_t pos = os->get_value().get_translation() ;

                motor::math::m3d::trafof_t const t( 
                    motor::math::vec3f_t(s), 
                    motor::math::vec3f_t(angle, 0.0f, angle),
                    pos ) ;

                os->set_and_exchange( t ) ;
            }

            {
                auto * os = _out_1;

                static float_t angle = 0.0f ;
                angle += d.sec_dt * motor::math::constants<float_t>::pix2() ;
                angle = angle > motor::math::constants<float_t>::pix2() ? 0.0f : angle ;

                motor::math::vec3f_t pos = os->get_value().get_translation() ;

                motor::math::m3d::trafof_t const t(
                    motor::math::vec3f_t( 1.0f ),
                    motor::math::vec3f_t( angle*0.347f, angle, 0.0f ),
                    pos ) ;

                // disabled because this slot is connected to the 
                // trafo variable which is tested below.
                //os->set_and_exchange( t ) ;
            }

            {
                static float_t angle = 0.0f ;
                angle += d.sec_dt * motor::math::constants<float_t>::pix2() ;
                angle = angle > motor::math::constants<float_t>::pix2() ? 0.0f : angle ;

                motor::math::vec3f_t pos = _pos.get_value() ;

                pos.x( motor::math::fn< float_t >::sin( angle ) * 10.0f ) ;
                _pos.set_value( pos ) ;
            }

            {
                _pos.update() ;
                _trafo.update() ;
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
                fe->configure<motor::graphics::geometry_object_t>( &geo_obj ) ;
                fe->configure<motor::graphics::msl_object_t>( msl_obj ) ;
            }
            
            {
                motor::scene::render_visitor_t vis( fe, _cameras[_cam_id] ) ;
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
            motor::release( motor::move( msl_obj ) ) ;

            motor::release( motor::move( _selected ) ) ;
            motor::release( motor::move( _cameras[0] ) ) ;
            motor::release( motor::move( _cameras[1] ) ) ;

            motor::release( motor::move( _out_0 ) ) ;
            motor::release( motor::move( _out_1 ) ) ;
        }
    };
}

int main( int argc, char ** argv )
{
    return motor::platform::global_t::create_and_exec< this_file::my_app >() ;
}