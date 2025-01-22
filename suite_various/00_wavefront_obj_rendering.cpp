
#include <motor/platform/global.h>

#include <motor/graphics/object/image_object.h>

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

        struct per_material_info
        {
            bool_t alpha_blending ;
            motor::graphics::msl_object_mtr_t msl ;
        };
        motor::vector< per_material_info > materials;
        motor::vector< motor::graphics::geometry_object_mtr_t > geometries ;

        motor::graphics::state_object_t rs_solid ;
        motor::graphics::state_object_t rs_alpha ;
        
        struct image
        {
            motor::string_t name ;
            motor::graphics::image_object_mtr_t io ;
        };
        motor::vector< image > images ;

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

    private: // tmp render window

        size_t _rwid = size_t(-1) ;

    private:

        //******************************************************************************************************
        virtual void_t on_init( void_t ) noexcept
        {
            mod_reg = motor::format::global::register_default_modules(
                motor::shared( motor::format::module_registry_t(), "mod registry" ) ) ;

            
            motor::property::property_sheet_t sheet ;
            sheet.set_value("normalize_coordinate", true ) ;
            motor::io::database_t db = motor::io::database_t( motor::io::path_t( DATAPATH ), "./working", "data" ) ;

            #if 0
            auto obj_import = mod_reg->import_from( motor::io::location_t( "included.mitsuba.mitsuba-sphere.obj" ), "wavefront", &db,
                motor::shared( std::move( sheet ) ) ) ;
            #elif 0 // many materials
            auto obj_import = mod_reg->import_from( motor::io::location_t( "meshes.sibenik.sibenik.obj" ), "wavefront", &db,
                motor::shared( std::move( sheet ) ) ) ;
            #elif 0
            auto obj_import = mod_reg->import_from( motor::io::location_t( "meshes.dragon.obj" ), "wavefront", &db,
                motor::shared( std::move( sheet ) ) ) ;
            #elif 0
            auto obj_import = mod_reg->import_from( motor::io::location_t( "meshes.sponza.sponza.obj" ), "wavefront", &db,
                motor::shared( std::move( sheet ) ) ) ;
            #elif 0
            auto obj_import = mod_reg->import_from( motor::io::location_t( "included.teapot.teapot.obj" ), "wavefront", &db,
                motor::shared( std::move( sheet ) ) ) ;
            #elif 0
            auto obj_import = mod_reg->import_from( motor::io::location_t( "meshes.buddha.obj" ), "wavefront", &db,
                motor::shared( std::move( sheet ) ) ) ;
            #elif 0
            auto obj_import = mod_reg->import_from( motor::io::location_t( "meshes.rungholt.rungholt.obj" ), "wavefront", &db,
                motor::shared( std::move( sheet ) ) ) ;
            #elif 0
            auto obj_import = mod_reg->import_from( motor::io::location_t( "meshes.viper.Viper-mk-IV-fighter.obj" ), "wavefront", &db,
                motor::shared( std::move( sheet ) ) ) ;
            #else
            auto obj_import = mod_reg->import_from( motor::io::location_t( "meshes.house.obj" ), "wavefront", &db, 
                motor::shared( std::move( sheet ) ) ) ;
            #endif
            #if 1
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
                    wnd.send_message( motor::application::vsync_message_t( { false } ) ) ;
                } ) ;
            }
            #endif
            #if 0
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

            // render states for solid objects
            // must be called first, because this is clearing the buffers.
            {
                motor::graphics::state_object_t so = motor::graphics::state_object_t(
                    "solid_rs" ) ;

                {
                    motor::graphics::render_state_sets_t rss ;
                    rss.depth_s.do_change = true ;
                    rss.depth_s.ss.do_activate = true ;
                    rss.depth_s.ss.do_depth_write = true ;
                    rss.polygon_s.do_change = true ;
                    rss.polygon_s.ss.do_activate = true ;
                    rss.polygon_s.ss.ff = motor::graphics::front_face::clock_wise ;
                    rss.polygon_s.ss.cm = motor::graphics::cull_mode::back ;
                    rss.polygon_s.ss.fm = motor::graphics::fill_mode::fill;
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

                rs_solid = std::move( so ) ;
            }

            // render states for transparent objects
            {
                motor::graphics::state_object_t so = motor::graphics::state_object_t(
                    "transparent_rs" ) ;

                {
                    motor::graphics::render_state_sets_t rss ;
                    rss.depth_s.do_change = true ;
                    rss.depth_s.ss.do_activate = true ;
                    rss.depth_s.ss.do_depth_write = false ;
                    rss.polygon_s.do_change = true ;
                    rss.polygon_s.ss.do_activate = true ;
                    rss.polygon_s.ss.ff = motor::graphics::front_face::clock_wise ;
                    rss.polygon_s.ss.cm = motor::graphics::cull_mode::back ;
                    rss.polygon_s.ss.fm = motor::graphics::fill_mode::fill;
                    rss.clear_s.do_change = false ;
                                        
                    rss.blend_s.do_change = true ;
                    rss.blend_s.ss.do_activate = true ;
                    rss.blend_s.ss.blend_func = motor::graphics::blend_function::add ;
                    rss.blend_s.ss.src_blend_factor = motor::graphics::blend_factor::src_alpha ;
                    rss.blend_s.ss.dst_blend_factor = motor::graphics::blend_factor::one_minus_src_alpha ;
                    
                    rss.view_s.ss.vp = motor::math::vec4ui_t( 0, 0, 500, 500 ) ;
                    so.add_render_state_set( rss ) ;
                }

                rs_alpha = std::move( so ) ;
            }
            
            // analyse the import result
            {
                auto * iitem = obj_import.get() ;
                if( auto * item = dynamic_cast< motor::format::mesh_item_mtr_t >( iitem ); item != nullptr )
                {
                    auto tp_begin = std::chrono::high_resolution_clock::now() ;

                    // #1 create msl object for material
                    for( auto const & m : item->materials )
                    {
                        auto msl_obj = motor::graphics::msl_object_t( m.material_name ) ;
                        msl_obj.add( motor::graphics::msl_api_type::msl_4_0, m.shader ) ;

                        materials.emplace_back( this_t::per_material_info{ 
                            m.alpha_blending, motor::shared( std::move( msl_obj ) ) } ) ;
                    }

                    // #2 create a geometry object for each geometric mesh imported
                    // and link that geometry object to its proper material(msl_object)
                    for ( auto const & g : item->geos )
                    {
                        auto geo_obj = motor::graphics::geometry_object::create( g.name, g.poly ) ;

                        if( g.material_idx != size_t(-1) )
                        {
                            auto * msl_obj = materials[g.material_idx].msl ;
                            msl_obj->link_geometry( g.name ) ;
                            msl_obj->add_variable_set( motor::shared( motor::graphics::variable_set_t() ) ) ;
                        }

                        geometries.emplace_back( motor::shared( std::move( geo_obj ) ) ) ;
                    }

                    // - timing ends here
                    {
                        size_t const milli = std::chrono::duration_cast<std::chrono::milliseconds>
                            ( std::chrono::high_resolution_clock::now() - tp_begin ).count() ;

                        motor::log::global_t::status( "[application] : generating geometry took " +
                            motor::to_string( milli ) + " ms." ) ;
                    }
                    
                    // #3 create all required image objects
                    for( auto & i : item->images )
                    {
                        motor::graphics::image_object_t io( i.name, std::move( *i.img_ptr ) ) ;
                        io.set_wrap( motor::graphics::texture_wrap_mode::wrap_s, 
                            motor::graphics::texture_wrap_type::repeat ) ;
                        io.set_wrap( motor::graphics::texture_wrap_mode::wrap_t,
                            motor::graphics::texture_wrap_type::repeat ) ;
                        images.emplace_back( image{ i.name, motor::shared( std::move( io ) ) } ) ;
                        motor::release( motor::move( i.img_ptr) ) ;
                    }
                    
                    motor::release( motor::move( item ) ) ;
                }
                else
                {
                    // not a mesh item
                    motor::release( motor::move( iitem ) ) ;
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

            // fill shader variables with some default value
            for ( auto & pmi : materials )
            {
                auto * ptr = pmi.msl ;

                for ( size_t i = 0; i < ptr->borrow_varibale_sets().size(); ++i )
                {
                    auto & vs = *ptr->borrow_varibale_set(i) ;

                    {
                        auto * mat = vs.data_variable<motor::math::mat4f_t>( "world" ) ;
                        mat->set( motor::math::mat4f_t::make_scaling( motor::math::vec3f_t( 100.0f ) ) ) ;
                    }

                    {
                        auto * mat = vs.data_variable<motor::math::mat4f_t>( "view" );
                        mat->set( _camera.get_view_matrix() ) ;
                    }

                    {
                        auto * mat = vs.data_variable<motor::math::mat4f_t>( "proj" );
                        mat->set( _camera.get_proj_matrix() ) ;
                    }

                    {
                        auto * v = vs.data_variable<motor::math::vec3f_t>( "light_dir" );
                        v->set( motor::math::vec3f_t( -1.0f ).normalized() ) ;
                    }
                }
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
                if( wid == _rwid )
                {
                    motor::log::global_t::status( "[my_app] : window closed" ) ;
                    _rwid = size_t( -1 ) ;
                }
                else
                {
                    motor::log::global_t::status( "[my_app] : window closed" ) ;
                    this->close() ;
                }
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

            {
                motor::controls::types::ascii_keyboard_t keyboard( dd.ascii ) ;

                using layout_t = motor::controls::types::ascii_keyboard_t ;
                using key_t = layout_t::ascii_key ;

                if ( keyboard.get_state( key_t::f3 ) == motor::controls::components::key_state::released && 
                    _rwid == size_t( -1 ) )
                {
                    motor::application::window_info_t wi ;
                    wi.x = 900 ;
                    wi.y = 500 ;
                    wi.w = 800 ;
                    wi.h = 600 ;
                    wi.gen = motor::application::graphics_generation::gen4_gl4 ;

                    _rwid = this_t::create_window( wi ) ;

                    this_t::send_window_message( _rwid, [&] ( motor::application::app::window_view & wnd )
                    {
                        wnd.send_message( motor::application::show_message( { true } ) ) ;
                        wnd.send_message( motor::application::cursor_message_t( { false } ) ) ;
                        wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                    } ) ;
                }
                else if ( keyboard.get_state( key_t::f4 ) == motor::controls::components::key_state::released &&
                    _rwid != size_t( -1 ) )
                {
                    this_t::send_window_message( _rwid, [&] ( motor::application::app::window_view & wnd )
                    {
                        wnd.send_message( motor::application::fullscreen_message( 
                            { 
                                motor::application::three_state::toggle,
                                motor::application::three_state::toggle
                            } ) ) ;
                    } );
                }
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

            for ( auto & pmi : materials )
            {
                auto * ptr = pmi.msl ;

                for ( size_t i = 0; i < ptr->borrow_varibale_sets().size(); ++i )
                {
                    auto * vs = ptr->borrow_varibale_set( i ) ;
                    auto * mat = vs->data_variable<motor::math::mat4f_t>( "view" );
                    mat->set( _camera.get_view_matrix() ) ;
                }
            }
        }

        //******************************************************************************************************
        virtual void_t on_render( this_t::window_id_t const wid, motor::graphics::gen4::frontend_ptr_t fe,
            motor::application::app::render_data_in_t rd ) noexcept
        {
            // rd.first_frame helps in hit and run apps
            // just initialize all the used objects
            if ( rd.first_frame )
            {
                fe->configure< motor::graphics::state_object_t>( &rs_solid ) ;
                fe->configure< motor::graphics::state_object_t>( &rs_alpha ) ;

                for( auto & i : images )
                {
                    fe->configure< motor::graphics::image_object_t>( i.io ) ;
                }

                for( auto * ptr : geometries )
                {
                    fe->configure< motor::graphics::geometry_object_t>( ptr ) ;
                }

                for ( auto & pmi : materials )
                {
                    auto * ptr = pmi.msl ;
                    fe->configure< motor::graphics::msl_object_t>( ptr ) ;
                }
            }
            
            // render all solid objects first
            // fills the depth buffer
            {
                fe->push( &rs_solid ) ;

                // geometry is attached to a msl object
                // so go over all materials(msl objects) and 
                // render those. Geometry to variable set is 1:1.
                // see loading above.
                for( auto & pmi : materials )
                {
                    if( pmi.alpha_blending ) continue ;

                    auto * ptr = pmi.msl ;

                    for( size_t i=0; i<ptr->borrow_varibale_sets().size(); ++i )
                    {
                        motor::graphics::gen4::backend::render_detail det = 
                        { 
                            0, // start elem
                            size_t( -1 ), // num elems
                            i, // variable set index
                            i, // geo index
                            size_t( -1 ), false, false
                        } ;
                        fe->render( ptr, det ) ;
                    }
                }

                fe->pop( motor::graphics::gen4::backend::pop_type::render_state ) ;
            }

            // render all transparent object using
            // depth test only -> no depth write
            // alpha blending enabled
            {
                fe->push( &rs_alpha ) ;

                // geometry is attached to a msl object
                // so go over all materials(msl objects) and 
                // render those. Geometry to variable set is 1:1.
                // see loading above.
                for ( auto & pmi : materials )
                {
                    if ( !pmi.alpha_blending ) continue ;

                    auto * ptr = pmi.msl ;

                    for ( size_t i = 0; i < ptr->borrow_varibale_sets().size(); ++i )
                    {
                        motor::graphics::gen4::backend::render_detail det =
                        {
                            0, // start elem
                            size_t( -1 ), // num elems
                            i, // variable set index
                            i, // geo index
                            size_t( -1 ), false, false
                        } ;
                        fe->render( ptr, det ) ;
                    }
                }

                fe->pop( motor::graphics::gen4::backend::pop_type::render_state ) ;
            }
        }

        //******************************************************************************************************
        virtual bool_t on_tool( this_t::window_id_t const wid, motor::application::app::tool_data_ref_t td ) noexcept 
        { 
            if( ImGui::Begin( "Control" ) )
            {
                bool_t do_wire ;
                rs_solid.access_render_state( 0, [&] ( motor::graphics::render_state_sets_ref_t ss )
                {
                    do_wire = ss.polygon_s.ss.fm == motor::graphics::fill_mode::line ;
                    return false ;
                } ) ;

                
                if( ImGui::Checkbox( "Wire", &do_wire ) )
                {
                    rs_solid.access_render_state( 0, [&] ( motor::graphics::render_state_sets_ref_t ss )
                    {
                        ss.polygon_s.ss.fm = do_wire ? motor::graphics::fill_mode::line :
                            motor::graphics::fill_mode::fill ;
                        return true ;
                    } ) ;
                }
            }
            ImGui::End() ;

            return true ; 
        }

        //******************************************************************************************************
        virtual void_t on_shutdown( void_t ) noexcept
        {
            for( auto & pmi : materials )
            {
                motor::release( motor::move( pmi.msl ) ) ;
            }

            for ( auto * ptr : geometries )
            {
                motor::release( motor::move( ptr ) ) ;
            }

            for( auto & i : images )
            {
                motor::release( motor::move( i.io ) ) ;
            }
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
