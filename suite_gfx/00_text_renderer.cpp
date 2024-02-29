
#include <motor/platform/global.h>

#include <motor/gfx/font/text_render_2d.h>
#include <motor/format/global.h>
#include <motor/format/future_items.hpp>
#include <motor/property/property_sheet.hpp>

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

        motor::io::database db = motor::io::database( motor::io::path_t( DATAPATH ), "./working", "data" ) ;
        motor::gfx::text_render_2d_t tr ;

        //***************************************************************************************************
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
                    wnd.send_message( motor::application::cursor_message_t( {false} ) ) ;
                    wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                } ) ;
            }

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
                    wnd.send_message( motor::application::cursor_message_t( {false} ) ) ;
                    wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                } ) ;
            }

            // import fonts and create text render
            {
                motor::property::property_sheet_mtr_t ps = motor::shared( motor::property::property_sheet_t() ) ;

                {
                    motor::font::code_points_t pts ;
                    for( uint32_t i = 33; i <= 126; ++i ) pts.emplace_back( i ) ;
                    for( uint32_t i : {uint32_t( 0x00003041 )} ) pts.emplace_back( i ) ;
                    ps->set_value< motor::font::code_points_t >( "code_points", pts ) ;
                }

                {
                    motor::vector< motor::io::location_t > locations = 
                    {
                        motor::io::location_t("fonts.mitimasu.ttf"),
                        //motor::io::location_t("")
                    } ;
                    ps->set_value( "additional_locations", locations ) ;
                }

                {
                    ps->set_value<size_t>( "atlas_width", 128 ) ;
                    ps->set_value<size_t>( "atlas_height", 512 ) ;
                    ps->set_value<size_t>( "point_size", 90 ) ;
                }

                motor::format::module_registry_mtr_t mod_reg = motor::format::global::register_default_modules( 
                    motor::shared( motor::format::module_registry_t() ) ) ;

                auto fitem = mod_reg->import_from( motor::io::location_t( "fonts.LCD_Solid.ttf" ), &db, motor::move( ps ) ) ;

                if( auto * ii = dynamic_cast<motor::format::glyph_atlas_item_mtr_t>( fitem.get() ); ii != nullptr )
                {
                    tr.init( "my_text_render", motor::move( ii->obj ) ) ;
                    motor::memory::release_ptr( ii ) ;
                }
                else
                {
                    motor::memory::release_ptr( ii ) ;
                    std::exit( 1 ) ;
                }

                motor::memory::release_ptr( mod_reg ) ;
            }
        }

        //***************************************************************************************************
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
        }

        //***************************************************************************************************
        virtual void_t on_graphics( motor::application::app::graphics_data_in_t ) noexcept 
        {
            {
                tr.draw_text( 0, 0, 10, motor::math::vec2f_t(0.0f, -0.60f), 
                    motor::math::vec4f_t(1.0f), "Hello World Group 0!" ) ;

                tr.draw_text( 1, 1, 25, motor::math::vec2f_t(0.0f, -0.7f), 
                    motor::math::vec4f_t(1.0f), "Hello World Group 1!" ) ;

                static uint_t number = 0 ;
                ++number ;
                tr.draw_text( 0, 1, 120, motor::math::vec2f_t(0.0f, -0.5f), 
                    motor::math::vec4f_t(1.0f), motor::to_string(number) ) ;
            }

            for( size_t i=0; i<10; ++i )
            {
                float_t const yoff = 0.08f * float_t(i) ;
                size_t const pt = 25 - i * 2 ;
                tr.draw_text( 0, 0, pt, motor::math::vec2f_t(-0.8f, 0.40f-yoff), 
                    motor::math::vec4f_t(1.0f), "Hello World! This is changing point ("+
                    motor::to_string(pt) +") size." ) ;
            }

            tr.prepare_for_rendering() ;
        } 

        //***************************************************************************************************
        virtual void_t on_render( this_t::window_id_t const wid, motor::graphics::gen4::frontend_ptr_t fe, 
            motor::application::app::render_data_in_t rd ) noexcept 
        {
            if( rd.first_frame )
            {
                tr.configure( fe ) ;
            }

            // prepare and render layer 0 and 1
            {
                tr.prepare_for_rendering(fe) ;
                tr.render( fe, 0 ) ;
                tr.render( fe, 1 ) ;
            }
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
