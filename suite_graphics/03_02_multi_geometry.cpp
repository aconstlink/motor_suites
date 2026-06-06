
#include <motor/platform/global.h>

#include <motor/geometry/3d/cube.h>

#include <motor/graphics/frontend/gen4/frontend.hpp>
#include <motor/graphics/object/render_object.h>
#include <motor/graphics/object/geometry_object.h>

#include <motor/gfx/camera/generic_camera.h>
#include <motor/math/utility/3d/transformation.hpp>
#include <motor/math/utility/angle.hpp>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>

#include <motor/math/camera/3d/orthographic_projection.hpp>
#include <motor/math/camera/3d/perspective_fov.hpp>

#include <future>

namespace this_file
{
    using namespace motor::core::types ;

    class my_app : public motor::application::app
    {
        motor_this_typedefs( my_app ) ;

        motor::math::vec4ui_t fb_dims = motor::math::vec4ui_t(0, 0, 1920, 1080) ;

        motor::graphics::state_object_t scene_so ;
        motor::graphics::geometry_object_t geo_obj0 ;
        motor::graphics::geometry_object_t geo_obj1 ;
        motor::graphics::geometry_object_t geo_obj2 ;
        motor::graphics::msl_object_mtr_t msl_obj_scene ;
                

        motor::graphics::state_object_t fb_so ;

        size_t _cam_id = 0 ;
        motor::vector<motor::gfx::generic_camera_mtr_t> _cameras ;

        virtual void_t on_init( void_t ) noexcept
        {
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

            {
                motor::application::window_info_t wi ;
                wi.x = 400 ;
                wi.y = 100 ;
                wi.w = 800 ;
                wi.h = 600 ;
                wi.gen = motor::application::graphics_generation::gen4_gl4 ;
                this_t::send_window_message( this_t::create_window( wi ), [&]( motor::application::app::window_view & wnd )
                {
                    wnd.send_message( motor::application::show_message( { true } ) ) ;
                    wnd.send_message( motor::application::cursor_message_t( {true} ) ) ;
                    wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                } ) ;
            }

            

            {
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
                    _cameras.emplace_back( motor::shared( std::move( cam ) ) ) ;
                }

                // camera
                {
                    auto cam = motor::gfx::generic_camera_t( 1.0f, 1.0f, 1.0f, 100.0f ) ;
                    cam.perspective_fov( motor::math::angle<float_t>::degree_to_radian( 45.0f ) ) ;
                    cam.look_at( motor::math::vec3f_t( -50.0f, 00.0f, 0.0f ),
                        motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ), motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) ) ;

                    _cameras.emplace_back( motor::shared( std::move( cam ) ) ) ;
                } 
                
            }
            {
                motor::graphics::render_state_sets_t rss ;
                rss.depth_s.do_change = true ;
                rss.depth_s.ss.do_activate = false ;
                rss.depth_s.ss.do_depth_write = false ;
                rss.polygon_s.do_change = true ;
                rss.polygon_s.ss.do_activate = true ;
                rss.polygon_s.ss.ff = motor::graphics::front_face::clock_wise ;
                rss.polygon_s.ss.cm = motor::graphics::cull_mode::back ;
                rss.polygon_s.ss.fm = motor::graphics::fill_mode::fill ;
                rss.clear_s.do_change = true ;
                rss.clear_s.ss.clear_color = motor::math::vec4f_t(0.5f, 0.9f, 0.5f, 1.0f ) ;
                rss.clear_s.ss.do_activate = true ;
                rss.clear_s.ss.do_color_clear = true ;
                rss.clear_s.ss.do_depth_clear = true ;
                rss.view_s.do_change = true ;
                rss.view_s.ss.do_activate = true ;
                rss.view_s.ss.vp = fb_dims ;

                scene_so = motor::graphics::state_object_t("scene_render_states") ;
                scene_so.add_render_state_set( rss ) ;
            }

            {
                motor::graphics::render_state_sets_t rss ;
                rss.depth_s.do_change = true ;
                rss.depth_s.ss.do_activate = false ;
                rss.depth_s.ss.do_depth_write = false ;

                rss.polygon_s.ss.do_activate = true ;
                rss.polygon_s.ss.ff = motor::graphics::front_face::clock_wise ;
                rss.polygon_s.ss.cm = motor::graphics::cull_mode::back ;

                rss.clear_s.do_change = false ;

                fb_so = motor::graphics::state_object_t("framebuffer_render_states") ;
                fb_so.add_render_state_set( rss ) ;
            }
                
            struct vertex { motor::math::vec4f_t pos ; motor::math::vec4f_t col ; } ;

            // geometry configuration 1
            {
                auto vb = motor::graphics::vertex_buffer_t()
                    .add_layout_element( motor::graphics::vertex_attribute::position, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                    .add_layout_element( motor::graphics::vertex_attribute::color0, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                    .resize( 4 ).update<vertex>( [=] ( vertex* array, size_t const ne )
                {
                    array[ 0 ].pos = motor::math::vec4f_t( -0.5f, -0.5f, 1.5f, 1.0f ) ;
                    array[ 1 ].pos = motor::math::vec4f_t( -0.5f, +0.5f, 1.5f, 1.0f ) ;
                    array[ 2 ].pos = motor::math::vec4f_t( +0.5f, +0.5f, 1.5f, 1.0f ) ;
                    array[ 3 ].pos = motor::math::vec4f_t( +0.5f, -0.5f, 1.5f, 1.0f ) ;

                    array[ 0 ].col = motor::math::vec4f_t( 1.0f, 0.0f,0.0f,1.0f ) ;
                    array[ 1 ].col = motor::math::vec4f_t( 1.0f, 0.0f,0.0f,1.0f ) ;
                    array[ 2 ].col = motor::math::vec4f_t( 1.0f, 0.0f,0.0f,1.0f ) ;
                    array[ 3 ].col = motor::math::vec4f_t( 1.0f, 0.0f,0.0f,1.0f ) ;
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

                geo_obj0 = motor::graphics::geometry_object_t( "geo0",
                    motor::graphics::primitive_type::triangles, std::move( vb ), std::move( ib ) ) ;
                    
            }

            // geometry configuration 2
            {
                auto vb = motor::graphics::vertex_buffer_t()
                    .add_layout_element( motor::graphics::vertex_attribute::position, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                    .add_layout_element( motor::graphics::vertex_attribute::color0, motor::graphics::type::tfloat, motor::graphics::type_struct::vec4 )
                    .resize( 9 ).update<vertex>( [=] ( vertex* array, size_t const ne )
                {
                    array[ 0 ].pos = motor::math::vec4f_t( -0.25f, +0.50f, 0.0f, 1.0f ) ;
                    array[ 1 ].pos = motor::math::vec4f_t( +0.25f, +0.50f, 0.0f, 1.0f ) ;
                    array[ 2 ].pos = motor::math::vec4f_t( +0.50f, +0.25f, 0.0f, 1.0f ) ;
                    array[ 3 ].pos = motor::math::vec4f_t( +0.50f, -0.25f, 0.0f, 1.0f ) ;
                    array[ 4 ].pos = motor::math::vec4f_t( +0.25f, -0.50f, 0.0f, 1.0f ) ;
                    array[ 5 ].pos = motor::math::vec4f_t( -0.25f, -0.50f, 0.0f, 1.0f ) ;
                    array[ 6 ].pos = motor::math::vec4f_t( -0.50f, -0.25f, 0.0f, 1.0f ) ;
                    array[ 7 ].pos = motor::math::vec4f_t( -0.50f, +0.25f, 0.0f, 1.0f ) ;
                    array[ 8 ].pos = motor::math::vec4f_t( 0.0f, 0.0f, 0.0f, 1.0f ) ;

                    array[ 0 ].col = motor::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    array[ 1 ].col = motor::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    array[ 2 ].col = motor::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    array[ 3 ].col = motor::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    array[ 4 ].col = motor::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    array[ 5 ].col = motor::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    array[ 6 ].col = motor::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    array[ 7 ].col = motor::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    array[ 8 ].col = motor::math::vec4f_t( 1.0f, 1.0f, 1.0f, 1.0f ) ;
                } );

                auto ib = motor::graphics::index_buffer_t().
                    set_layout_element( motor::graphics::type::tuint ).resize( 24 ).
                    update<uint_t>( [] ( uint_t* array, size_t const ne )
                {
                    array[ 0 ] = 0 ;
                    array[ 1 ] = 1 ;
                    array[ 2 ] = 8 ;

                    array[ 3 ] = 1 ;
                    array[ 4 ] = 2 ;
                    array[ 5 ] = 8 ;

                    array[ 6 ] = 2 ;
                    array[ 7 ] = 3 ;
                    array[ 8 ] = 8 ;

                    array[ 9 ] = 3 ;
                    array[ 10 ] = 4 ;
                    array[ 11 ] = 8 ;

                    array[ 12 ] = 4 ;
                    array[ 13 ] = 5 ;
                    array[ 14 ] = 8 ;

                    array[ 15 ] = 5 ;
                    array[ 16 ] = 6 ;
                    array[ 17 ] = 8 ;

                    array[ 18 ] = 6 ;
                    array[ 19 ] = 7 ;
                    array[ 20 ] = 8 ;

                    array[ 21 ] = 7 ;
                    array[ 22 ] = 0 ;
                    array[ 23 ] = 8 ;
                } ) ;

                geo_obj1 = motor::graphics::geometry_object_t( "geo1",
                    motor::graphics::primitive_type::triangles, std::move( vb ), std::move( ib ) ) ;
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

                    geo_obj2 = motor::graphics::geometry_object_t( "cube",
                        motor::graphics::primitive_type::triangles, std::move( vb ), std::move( ib ) ) ;
                }
            }
            
            // msl object for scene
            {
                // quad object
                {
                    motor::graphics::msl_object_t mslo("scene") ;

                    mslo.add( motor::graphics::msl_api_type::msl_4_0, R"(
                    config test_multi_geometry
                    {
                        vertex_shader
                        {
                            in vec4_t pos : position ;
                            in vec3_t nrm : normal ;
                            in vec4_t col : color0 ;
                            

                            out vec4_t pos : position ;
                            out vec4_t col : color ;
                            out vec3_t nrm ;

                            mat4_t u_proj : projection ;
                            mat4_t u_view : view ;
                            mat4_t u_world : world ;

                            void main()
                            {
                                out.col = in.col ;
                                out.nrm = in.nrm ;
                                vec4_t new_pos = u_view * u_world * (in.pos) ;
                                out.pos = u_proj * (new_pos + vec4_t(0.0, 0.0, 0.0, 0.0 ) );
                            }
                        }

                        pixel_shader
                        {
                            in vec4_t col : color0 ;
                            in vec3_t nrm ;
                            out vec4_t color : color0 ;

                            void main()
                            {
                                out.color = vec4_t(in.nrm.x,in.nrm.y, in.nrm.z,1.0) ;
                            }
                        }
                    })" ) ;

                        
                    mslo.link_geometry({"geo0","geo1", "cube"}) ;

                    msl_obj_scene = motor::shared( std::move( mslo ) ) ;
                }
                    
                {
                    motor::graphics::variable_set_t vars ;
                    msl_obj_scene->add_variable_set( motor::memory::create_ptr( std::move( vars ), "a variable set" ) ) ;
                }
                
                {
                    motor::graphics::variable_set_t vars ;
                    msl_obj_scene->add_variable_set( motor::memory::create_ptr( std::move(vars), "a variable set" ) ) ;
                }

                {
                    motor::graphics::variable_set_t vars ;
                    msl_obj_scene->add_variable_set( motor::memory::create_ptr( std::move(vars), "a variable set" ) ) ;
                }
            }             
        }

        virtual void_t on_event( window_id_t const wid, 
                motor::application::window_message_listener::state_vector_cref_t sv ) noexcept
        {
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
                for( size_t i=0; i<_cameras.size();++i)
                {
                    _cameras[i]->set_dims( w*2.0f, h*2.0f, 0.1f, 1000.0f ) ;
                    _cameras[i]->perspective_fov() ;
                }
                

                scene_so.access_render_state( 0, [&]( motor::graphics::render_state_sets_ref_t rss )
                {
                    rss.view_s.ss.vp = motor::math::vec4ui_t(0,0,uint_t(sv.resize_msg.w), uint_t(sv.resize_msg.h)) ;
                    return true ;
                } ) ;
            }
        }

        virtual void_t on_render( this_t::window_id_t const wid, motor::graphics::gen4::frontend_ptr_t fe,
            motor::application::app::render_data_in_t rd ) noexcept 
        {            
            if( rd.first_frame )
            {
                {
                    fe->configure<motor::graphics::state_object_t>( &scene_so ) ;
                    fe->configure<motor::graphics::state_object_t>( &fb_so ) ;
                }

                {
                    fe->configure<motor::graphics::geometry_object_t>( &geo_obj0 ) ;
                    fe->configure<motor::graphics::geometry_object_t>( &geo_obj1 ) ;
                    fe->configure<motor::graphics::geometry_object_t>( &geo_obj2 ) ;
                    fe->configure<motor::graphics::msl_object_t>( msl_obj_scene ) ;
                }

            }

            // render
            {
                {
                    size_t i = 0 ;
                    for( auto * vs : msl_obj_scene->borrow_varibale_sets() )
                    {
                        {
                            auto * var = vs->data_variable<motor::math::mat4f_t>( "u_proj" ) ;
                            var->set( _cameras[_cam_id]->mat_proj() ) ;
                        }
                        
                        {
                            auto * var = vs->data_variable<motor::math::mat4f_t>("u_view") ;
                            var->set( _cameras[_cam_id]->mat_view() ) ;
                        }
                        
                        {
                            auto const of = float_t(i) /float_t(msl_obj_scene->borrow_varibale_sets().size()-1) ;
                            motor::math::m3d::trafof_t t ;
                            t.scale_fl( 2.0f ) ;
                            t.translate_fl( motor::math::vec3f_t( 6.0f * (of * 2.0f - 1.0f), 0.0f, 0.0f ) ) ;

                            auto * var = vs->data_variable<motor::math::mat4f_t>("u_world") ;
                            var->set( t.get_transformation() ) ;
                        }
                        
                        ++i ;
                    }
                }

                
                
                fe->push( &scene_so ) ;
                {
                    motor::graphics::gen4::backend_t::render_detail_t detail ;
                    detail.varset = 0 ;
                    detail.geo = 0 ;
                    fe->render(  msl_obj_scene, detail ) ;
                }
                {
                    motor::graphics::gen4::backend_t::render_detail_t detail ;
                    detail.varset = 1 ;
                    detail.geo = 1 ;
                    fe->render(  msl_obj_scene, detail ) ;
                }
                {
                    motor::graphics::gen4::backend_t::render_detail_t detail ;
                    detail.varset = 2 ;
                    detail.geo = 2 ;
                    fe->render(  msl_obj_scene, detail ) ;
                }
                fe->pop( motor::graphics::gen4::backend::pop_type::render_state ) ; 
                
                fe->fence( [=]( void_t ){} ) ;
            }
        }

        //******************************************************************************************************
        virtual bool_t on_tool( this_t::window_id_t const wid, motor::application::app::tool_data_ref_t ) noexcept 
        { 
            {
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
                        float z = cam_pos.z() ;
                        ImGui::SliderFloat( "Cur Cam X", &x, -100.0f, 100.0f ) ;
                        ImGui::SliderFloat( "Cur Cam Y", &y, -100.0f, 100.0f ) ;
                        ImGui::SliderFloat( "Cur Cam Z", &z, -100.0f, 100.0f ) ;
                        _cameras[_cam_id]->translate_to( motor::math::vec3f_t( x, y, z ) ) ;
                        
                    }
                }
                ImGui::End() ;
            }
            
            return true ; 
        }

        virtual void_t on_shutdown( void_t ) noexcept 
        {
            motor::release( motor::move( msl_obj_scene ) ) ;
        }
    };
}

int main( int argc, char ** argv )
{
    using namespace motor::core::types ;

    motor::application::carrier_mtr_t carrier = motor::platform::global_t::create_carrier(
        motor::shared( this_file::my_app() ) ) ;
    
    auto const ret = carrier->exec() ;
    
    motor::memory::release_ptr( carrier ) ;

    motor::concurrent::global::deinit() ;
    motor::log::global::deinit() ;
    motor::memory::global::dump_to_std() ;


    return ret ;
}
