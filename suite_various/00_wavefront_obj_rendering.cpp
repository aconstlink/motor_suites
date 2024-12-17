
#include <motor/platform/global.h>


#include <motor/math/utility/fn.hpp>
#include <motor/math/utility/angle.hpp>
#include <motor/math/animation/keyframe_sequence.hpp>
#include <motor/math/quaternion/quaternion4.hpp>


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

        motor::graphics::msl_object_t msl_obj ;
        motor::graphics::geometry_object_t geo_obj ;
        motor::graphics::state_object_t rs ;

        motor::format::module_registry_mtr_t mod_reg = nullptr ;
        size_t cur_time = 0 ;

        //******************************************************************************************************
        virtual void_t on_init( void_t ) noexcept
        {
            mod_reg = motor::format::global::register_default_modules(
                motor::shared( motor::format::module_registry_t(), "mod registry" ) ) ;

            motor::io::database_t db = motor::io::database_t( motor::io::path_t( DATAPATH ), "./working", "data" ) ;
            auto obj_import = mod_reg->import_from( motor::io::location_t( "meshes.giraffe.obj" ), "wavefront", &db ) ;

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
                    geo_obj = motor::graphics::geometry_object::create( item->name, item->poly ) ;
                    msl_obj = motor::graphics::msl_object_t( item->name ) ;
                    msl_obj.add( motor::graphics::msl_api_type::msl_4_0, item->shader ) ;
                    msl_obj.link_geometry( { item->name } ) ;
                    msl_obj.add_variable_set( motor::shared(motor::graphics::variable_set_t()) ) ;
                    motor::release( motor::move( item ) ) ;
                }
                else
                {
                    // not a mesh item
                    motor::release( motor::move( item ) ) ;
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
        {}

        //******************************************************************************************************
        virtual void_t on_graphics( motor::application::app::graphics_data_in_t gd ) noexcept
        {
            // global time 
            size_t const time = cur_time ;


        }

        //******************************************************************************************************
        virtual void_t on_render( this_t::window_id_t const wid, motor::graphics::gen4::frontend_ptr_t fe,
            motor::application::app::render_data_in_t rd ) noexcept
        {
            if ( rd.first_frame )
            {
                fe->configure< motor::graphics::state_object_t>( &rs ) ;
                fe->configure< motor::graphics::geometry_object_t>( &geo_obj ) ;
                fe->configure< motor::graphics::msl_object_t>( &msl_obj ) ;
                
            }
            
            {
                fe->push( &rs ) ;
                fe->render( &msl_obj, motor::graphics::gen4::backend::render_detail() ) ;
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
