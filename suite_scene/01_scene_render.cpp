

#include <motor/profiling/probe_guard.hpp>

#include <motor/platform/global.h>

#include <motor/geometry/mesh/tri_mesh.h>
#include <motor/geometry/mesh/flat_tri_mesh.h>
#include <motor/geometry/3d/cube.h>

#include <motor/controls/types/ascii_keyboard.hpp>
#include <motor/controls/types/three_mouse.hpp>

#include <motor/scene/node/group/logic_group.h>
#include <motor/scene/node/decorator/logic_decorator.h>
#include <motor/scene/node/leaf/logic_leaf.h>
#include <motor/scene/component/name_component.hpp>
#include <motor/scene/component/trafo_3d_component.hpp>
#include <motor/scene/component/render/camera_component.h>
#include <motor/scene/component/render/msl_component.h>
#include <motor/scene/component/render/render_state_component.h>

#include <motor/tool/imgui/imgui_node_visitor.h>

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
        motor::graphics::msl_object_t msl_obj ;
        motor::graphics::geometry_object_t geo_obj ;

        //******************************************************************************************************
        virtual void_t on_init( void_t ) noexcept
        {
            MOTOR_PROBE( "application", "on_init" ) ;

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

            // #2 : make msl objects
            {
            }

            // #3 : init scene tree
            {
                motor::scene::logic_decorator_t root ;
                root.add_component( motor::shared( motor::scene::name_component_t( "my root name" ) ) ) ;

                {
                    auto g = motor::shared( motor::scene::logic_group() ) ;

                    // add transformation to g
                    {
                        g->add_component( motor::shared( motor::scene::trafo_3d_component_t(
                            motor::math::m3d::trafof_t() ) ) ) ;
                    }

                    // add render state to g
                    {
                        motor::graphics::state_object_t scene_so ;
                        {
                            motor::graphics::render_state_sets_t rss ;

                            rss.depth_s.do_change = true ;
                            rss.depth_s.ss.do_activate = true ;
                            rss.depth_s.ss.do_depth_write = true ;

                            rss.polygon_s.do_change = true ;
                            rss.polygon_s.ss.do_activate = true ;
                            rss.polygon_s.ss.ff = motor::graphics::front_face::counter_clock_wise ;
                            rss.polygon_s.ss.cm = motor::graphics::cull_mode::back ;
                            rss.polygon_s.ss.fm = motor::graphics::fill_mode::fill ;

                            rss.clear_s.do_change = true ;
                            rss.clear_s.ss.do_activate = true ;
                            rss.clear_s.ss.clear_color = motor::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                            rss.clear_s.ss.do_color_clear = true ;
                            rss.clear_s.ss.do_depth_clear = true ;

                            rss.view_s.do_change = true ;
                            rss.view_s.ss.do_activate = true ;
                            rss.view_s.ss.vp = fb_dims ;

                            scene_so = motor::graphics::state_object_t( "scene_render_states" ) ;
                            scene_so.add_render_state_set( rss ) ;
                        }
                        g->add_component( motor::shared( motor::scene::render_state_component_t( std::move( scene_so ) ) ) ) ;
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
                                    out vec4_t color1 : color1 ;
                                    out vec4_t color2 : color2 ;

                                    void main()
                                    {
                                        float_t light = dot( normalize( in.nrm ), normalize( vec3_t( 1.0, 1.0, 0.5) ) ) ;
                                        out.color0 = color ' texture( tex, in.tx ) ;
                                        out.color1 = vec4_t( in.nrm, 1.0 ) ;
                                        out.color2 = vec4_t( light, light, light , 1.0 ) ;
                                    }
                                }
                            })" ) ;

                            mslo.link_geometry( { "cube" } ) ;
                            
                            msl_obj = std::move( mslo ) ;
                        }

                        {
                            motor::graphics::variable_set_t vars ;

                            {
                                auto * var = vars.data_variable< motor::math::vec4f_t >( "color" ) ;
                                var->set( motor::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ) ) ;
                            }

                            {
                                auto * var = vars.data_variable< float_t >( "u_time" ) ;
                                var->set( 0.0f ) ;
                            }

                            {
                                auto * var = vars.texture_variable( "tex" ) ;
                                var->set( "checker_board" ) ;
                            }

                            {
                                auto * var = vars.data_variable< motor::math::mat4f_t >( "world" ) ;
                                //var->set( trans.get_transformation() ) ;
                            }
                            
                            msl_obj.add_variable_set( motor::memory::create_ptr( std::move( vars ), "a variable set" ) ) ;
                        }
                    }

                    // add geometry 1
                    {
                        auto leaf = motor::shared( motor::scene::logic_leaf_t() ) ;
                        leaf->add_component( motor::shared( motor::scene::name_component_t( "geometry 1" ) ) ) ;
                        leaf->add_component( motor::shared( motor::scene::trafo_3d_component_t( motor::math::m3d::trafof_t()  ) ) ) ;
                        //leaf->add_component( motor::shared( motor::scene::msl_component_t() ) ) ;

                        g->add_child( motor::move( leaf ) ) ;
                    }

                    #if 0
                    // add geometry 2
                    {
                        auto leaf = motor::shared( motor::scene::logic_leaf_t() ) ;
                        leaf->add_component( motor::shared( motor::scene::name_component_t( "geometry 2" ) ) ) ;
                        g->add_child( motor::move( leaf ) ) ;
                    }

                    // add geometry 3
                    {
                        auto leaf = motor::shared( motor::scene::logic_leaf_t() ) ;
                        leaf->add_component( motor::shared( motor::scene::name_component_t( "geometry 3" ) ) ) ;
                        g->add_child( motor::move( leaf ) ) ;
                    }
                    #endif

                    root.set_decorated( motor::move( g ) ) ;
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
        }

        //******************************************************************************************************
        virtual bool_t on_tool( this_t::window_id_t const wid, motor::application::app::tool_data_ref_t ) noexcept 
        { 
            MOTOR_PROBE( "application", "on_tool" ) ;

            {
                if( ImGui::Begin("test window") )
                {
                    motor::tool::imgui_node_visitor_t v ;
                    motor::scene::node_t::traverser( _root ).apply( &v ) ;
                }
                ImGui::End() ;
            }
            
            return true ; 
        }

        virtual void_t on_shutdown( void_t ) noexcept {}
    };
}

int main( int argc, char ** argv )
{
    return motor::platform::global_t::create_and_exec< this_file::my_app >() ;
}