
#include <motor/platform/global.h>

#include <motor/controls/types/ascii_keyboard.hpp>
#include <motor/controls/types/three_mouse.hpp>

#include <motor/math/utility/fn.hpp>
#include <motor/math/utility/angle.hpp>
#include <motor/math/animation/keyframe_sequence.hpp>
#include <motor/math/quaternion/quaternion4.hpp>

#include <motor/gfx/camera/generic_camera.h>

#include <motor/format/global.h>
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

        struct geometry
        {
            motor::graphics::msl_object_t msl_obj ;
            motor::graphics::geometry_object_t geo_obj ;
        };
        motor::vector< geometry > geos ;

        motor::graphics::state_object_t rs ;

        motor::format::module_registry_mtr_t mod_reg = nullptr ;
        size_t cur_time = 0 ;

        motor::gfx::generic_camera_t _camera ;
        struct cam_control
        {
            bool_t move_left = false ;
            bool_t move_right = false ;
            bool_t move_forwards = false ;
            bool_t move_backwards = false ;
            bool_t move_upwards = false ;
            bool_t move_downwards = false ;

            motor::math::vec2f_t mouse_coords ;

            int_t rotate_x = 0 ;
            int_t rotate_y = 0 ;
            int_t rotate_z = 0 ;
        };

        cam_control _cc ;

        //******************************************************************************************************
        virtual void_t on_init( void_t ) noexcept
        {
            mod_reg = motor::format::global::register_default_modules(
                motor::shared( motor::format::module_registry_t(), "mod registry" ) ) ;

            motor::property::property_sheet_t sheet ;
            sheet.set_value("normalize_coordinate", true ) ;
            motor::io::database_t db = motor::io::database_t( motor::io::path_t( DATAPATH ), "./working", "data" ) ;
            auto obj_import = mod_reg->import_from( motor::io::location_t( "meshes.giraffe.obj" ), "wavefront", &db, 
                motor::shared( std::move( sheet ) ) ) ;

            #if 0
            {
                motor::application::window_info_t wi ;
                wi.x = 100 ;
                wi.y = 100 ;
                wi.w = 800 ;
                wi.h = 600 ;
                wi.gen = motor::application::graphics_generation::gen4_auto ;

                this_t::send_window_message( this_t::create_window( wi ), [&] ( motor::application::app::window_view & wnd )
                {
                    wnd.send_message( motor::application::show_message( { true } ) ) ;
                    wnd.send_message( motor::application::cursor_message_t( { true } ) ) ;
                    wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                } ) ;
            }
            #endif
            #if 1
            {
                motor::application::window_info_t wi ;
                wi.x = 400 ;
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
                    rss.polygon_s.ss.ff = motor::graphics::front_face::clock_wise ;
                    rss.polygon_s.ss.cm = motor::graphics::cull_mode::back ;
                    rss.polygon_s.ss.fm = motor::graphics::fill_mode::line ;
                    rss.clear_s.do_change = true ;
                    rss.clear_s.ss.clear_color = motor::math::vec4f_t( 0.5f, 0.2f, 0.2f, 1.0f ) ;
                    rss.clear_s.ss.do_activate = true ;
                    rss.clear_s.ss.do_color_clear = true ;
                    rss.clear_s.ss.do_depth_clear = true ;
                    rss.view_s.do_change = false ;
                    rss.view_s.ss.do_activate = false ;
                    rss.view_s.ss.vp = motor::math::vec4ui_t( 0, 0, 500, 500 ) ;
                    so.add_render_state_set( rss ) ;
                }

                rs = std::move( so ) ;
            }

            {
                if( auto * item = dynamic_cast< motor::format::mesh_item_mtr_t >( obj_import.get() ); item != nullptr )
                {
                    for( auto const & g : item->geos )
                    {
                        auto geo_obj = motor::graphics::geometry_object::create( g.name, g.poly ) ;
                        auto msl_obj = motor::graphics::msl_object_t( g.name ) ;
                        msl_obj.add( motor::graphics::msl_api_type::msl_4_0, g.shader ) ;
                        msl_obj.link_geometry( { g.name } ) ;
                        geos.emplace_back( this_t::geometry { std::move( msl_obj ), std::move( geo_obj ) } ) ;
                    }

                    motor::release( motor::move( item ) ) ;
                }
                else
                {
                    // not a mesh item
                    motor::release( motor::move( item ) ) ;
                }
            }

            // camera
            {
                auto cam = motor::gfx::generic_camera_t( 1.0f, 1.0f, 1.0f, 10000.0f ) ;
                cam.perspective_fov( motor::math::angle<float_t>::degree_to_radian( 45.0f ) ) ;
                cam.look_at( motor::math::vec3f_t( 0.0f, 20.0f, -100.0f ),
                    motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ), motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) ) ;

                _camera = std::move( cam ) ;
            }

            // link shader variables
            for( auto & g : geos )
            {
                auto vs = motor::graphics::variable_set_t() ;

                {
                    auto * mat = vs.data_variable<motor::math::mat4f_t>( "world" ) ;
                    mat->set( motor::math::mat4f_t::make_scaling(motor::math::vec3f_t(50.0f)) ) ;
                }

                {
                    auto * mat = vs.data_variable<motor::math::mat4f_t>("view");
                    mat->set( _camera.get_view_matrix() ) ;
                }

                {
                    auto * mat = vs.data_variable<motor::math::mat4f_t>( "proj" );
                    mat->set( _camera.get_proj_matrix() ) ;
                }

                {
                    auto * v = vs.data_variable<motor::math::vec3f_t>( "light_dir" );
                    v->set( motor::math::vec3f_t(-1.0f).normalized() ) ;
                }

                g.msl_obj.add_variable_set( motor::shared( std::move(vs) ) ) ;
            }
        }

        //******************************************************************************************************
        virtual void_t on_event( window_id_t const wid,
            motor::application::window_message_listener::state_vector_cref_t sv ) noexcept
        {
            if ( sv.create_changed )
            {
                motor::log::global_t::status( "[my_app] : window created" ) ;
            }
            if ( sv.close_changed )
            {
                motor::log::global_t::status( "[my_app] : window closed" ) ;
                this->close() ;
            }
            if ( sv.resize_changed )
            {
                float_t const w = float_t( sv.resize_msg.w ) ;
                float_t const h = float_t( sv.resize_msg.h ) ;

            }
        }

        //******************************************************************************************************
        virtual void_t on_device( device_data_in_t dd ) noexcept
        {
            bool_t ctrl = false ;

            // keyboard testing
            {
                motor::controls::types::ascii_keyboard_t keyboard( dd.ascii ) ;

                using layout_t = motor::controls::types::ascii_keyboard_t ;
                using key_t = layout_t::ascii_key ;

                auto const left = keyboard.get_state( key_t::a ) ;
                auto const right = keyboard.get_state( key_t::d ) ;
                auto const forw = keyboard.get_state( key_t::w ) ;
                auto const back = keyboard.get_state( key_t::s ) ;
                auto const asc = keyboard.get_state( key_t::q ) ;
                auto const dsc = keyboard.get_state( key_t::e ) ;

                _cc.move_left = left != motor::controls::components::key_state::none ;
                _cc.move_right = right != motor::controls::components::key_state::none ;
                _cc.move_forwards = forw != motor::controls::components::key_state::none ;
                _cc.move_backwards = back != motor::controls::components::key_state::none ;
                _cc.move_upwards = asc != motor::controls::components::key_state::none ;
                _cc.move_downwards = dsc != motor::controls::components::key_state::none ;

                ctrl = keyboard.get_state( key_t::ctrl_left ) !=
                    motor::controls::components::key_state::none ;
                
            }

            // mouse testing
            {
                motor::controls::types::three_mouse_t mouse( dd.mouse ) ;

                motor::math::vec2f_t const mouse_coords = mouse.get_local() ;
                auto const dif = mouse_coords - _cc.mouse_coords ;
                _cc.mouse_coords = mouse_coords ;

                auto button_funk = [&] ( motor::controls::types::three_mouse_t::button const button )
                {
                    if ( mouse.is_pressed( button ) )
                    {
                        return true ;
                    }
                    else if ( mouse.is_pressing( button ) )
                    {
                        return true ;
                    }
                    else if ( mouse.is_released( button ) )
                    {
                    }
                    return false ;
                } ;

                auto const l = button_funk( motor::controls::types::three_mouse_t::button::left ) ;
                auto const r = button_funk( motor::controls::types::three_mouse_t::button::right ) ;
                auto const m = button_funk( motor::controls::types::three_mouse_t::button::middle ) ;

                _cc.rotate_x = r ? int_t( -motor::math::fn<float_t>::sign( dif.y() ) ) : 0 ;
                _cc.rotate_y = r ? int_t( +motor::math::fn<float_t>::sign( dif.x() ) ) : 0 ;
                _cc.rotate_z = ctrl ? int_t( +motor::math::fn<float_t>::sign( dif.x() ) ) : 0 ;
            }
        }

        //******************************************************************************************************
        virtual void_t on_graphics( motor::application::app::graphics_data_in_t gd ) noexcept
        {
            // global time 
            size_t const time = cur_time ;

            {
                // change camera translation
                {
                    motor::math::vec3f_t translate ;

                    // left/right
                    {
                        if ( _cc.move_left )
                        {
                            translate.x( -100.0f * gd.sec_dt ) ;
                        }
                        else if ( _cc.move_right )
                        {
                            translate.x( +100.0f * gd.sec_dt ) ;
                        }
                    }

                    // forwards/backwards
                    {
                        if ( _cc.move_backwards )
                        {
                            translate.z( -100.0f * gd.sec_dt ) ;
                        }
                        else if ( _cc.move_forwards )
                        {
                            translate.z( +100.0f * gd.sec_dt ) ;
                        }
                    }

                    // upwards/downwards
                    {
                        if ( _cc.move_upwards )
                        {
                            translate.y( -100.0f * gd.sec_dt ) ;
                        }
                        else if ( _cc.move_downwards )
                        {
                            translate.y( +100.0f * gd.sec_dt ) ;
                        }
                    }

                    _camera.translate_by( translate ) ;
                }

                // change camera rotation
                {
                    motor::math::vec3f_t const angle(
                        float_t( _cc.rotate_x ) * 2.0f * gd.sec_dt,
                        float_t( _cc.rotate_y ) * 2.0f * gd.sec_dt,
                        float_t( _cc.rotate_z ) * 2.0f * gd.sec_dt ) ;

                    motor::math::quat4f_t const x( angle.x(), motor::math::vec3f_t( 1.0f, 0.0f, 0.0f ) ) ;
                    motor::math::quat4f_t const y( angle.y(), motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ) ) ;
                    motor::math::quat4f_t const z( angle.z(), motor::math::vec3f_t( 0.0f, 0.0f, 1.0f ) ) ;

                    auto const final_axis = x * y * z ;

                    auto const orientation = final_axis.to_matrix() ;

                    auto const t = motor::math::m3d::trafof_t::rotation_by_matrix( orientation ) ;

                    _camera.transform_by( t ) ;
                }
            }

            for( auto & g : geos )
            {
                auto * vs = g.msl_obj.borrow_varibale_set(0) ;
                auto * mat = vs->data_variable<motor::math::mat4f_t>( "view" );
                mat->set( _camera.get_view_matrix() ) ;
            }
        }

        //******************************************************************************************************
        virtual void_t on_render( this_t::window_id_t const wid, motor::graphics::gen4::frontend_ptr_t fe,
            motor::application::app::render_data_in_t rd ) noexcept
        {
            if ( rd.first_frame )
            {
                fe->configure< motor::graphics::state_object_t>( &rs ) ;
                
                for( auto & g : geos )
                {
                    fe->configure< motor::graphics::geometry_object_t>( &g.geo_obj ) ;
                    fe->configure< motor::graphics::msl_object_t>( &g.msl_obj ) ;
                }
                
            }
            
            {
                fe->push( &rs ) ;
                for( auto & g : geos )
                    fe->render( &g.msl_obj, motor::graphics::gen4::backend::render_detail() ) ;
                fe->pop( motor::graphics::gen4::backend::pop_type::render_state ) ;
            }
        }

        //******************************************************************************************************
        virtual bool_t on_tool( this_t::window_id_t const wid, motor::application::app::tool_data_ref_t td ) noexcept 
        { 
            return true ; 
        }

        //******************************************************************************************************
        virtual void_t on_shutdown( void_t ) noexcept
        {
            motor::release( motor::move( mod_reg ) ) ;
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

    motor::io::global::deinit() ;
    motor::concurrent::global::deinit() ;
    motor::log::global::deinit() ;
    motor::memory::global::dump_to_std() ;


    return ret ;
}
