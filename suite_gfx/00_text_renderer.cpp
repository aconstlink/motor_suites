#include <motor/platform/global.h>
#include <motor/application/window/window_message_listener.h>
#include <motor/io/global.h>

#include <motor/gfx/font/text_render_2d.h>

#include <motor/format/global.h>
#include <motor/format/future_items.hpp>
#include <motor/property/property_sheet.hpp>

#include <motor/graphics/object/render_object.h>
#include <motor/graphics/object/geometry_object.h>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>

#include <future>

int main( int argc, char ** argv )
{
    using namespace motor::core::types ;

    motor::application::carrier_mtr_t carrier = motor::platform::global_t::create_carrier() ;

    auto fut_update_loop = std::async( std::launch::async, [&]( void_t )
    {
        motor::io::database db( motor::io::path_t( DATAPATH ), "./working", "data" ) ;

        motor::application::window_message_listener_mtr_t msgl_out = motor::memory::create_ptr<
            motor::application::window_message_listener>( "[out] : message listener" ) ;

        motor::gfx::text_render_2d_mtr_t tr = nullptr ;

        {
            motor::application::window_info_t wi ;
            wi.x = 100 ;
            wi.y = 100 ;
            wi.w = 800 ;
            wi.h = 600 ;
            wi.gen = motor::application::graphics_generation::gen4_auto ;

            auto wnd = carrier->create_window( wi ) ;

            wnd->register_out( motor::share( msgl_out ) ) ;

            wnd->send_message( motor::application::show_message( { true } ) ) ;
            wnd->send_message( motor::application::cursor_message_t( {false} ) ) ;
            wnd->send_message( motor::application::vsync_message_t( { true } ) ) ;

            // wait for window creation
            {
                std::this_thread::sleep_for( std::chrono::milliseconds(10) ) ;

                while( true ) 
                {
                    motor::application::window_message_listener_t::state_vector_t sv ;
                    if( msgl_out->swap_and_reset( sv ) )
                    {
                        if( sv.create_changed )
                        {
                            break ;
                        }
                    }
                }
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
                    tr = motor::shared( motor::gfx::text_render_2d_t() ) ;
                    tr->init( "my_text_render", motor::move( ii->obj ) ) ;
                    motor::memory::release_ptr( ii ) ;
                }
                else
                {
                    motor::memory::release_ptr( ii ) ;
                    std::exit( 1 ) ;
                }

                motor::memory::release_ptr( mod_reg ) ;
            }

            {
                auto my_rnd_funk_init = [&]( motor::graphics::gen4::frontend_ptr_t fe )
                {
                    tr->on_frame_init( fe ) ; 
                } ;

                if( wnd->render_frame< motor::graphics::gen4::frontend_t >( my_rnd_funk_init ) )
                {
                    // funk has been rendered.
                }

                while( true ) 
                {
                    std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) ) ;
                
                    motor::application::window_message_listener_t::state_vector_t sv ;
                    if( msgl_out->swap_and_reset( sv ) )
                    {
                        if( sv.close_changed )
                        {
                            break ;
                        }
                    }
                    
                    auto my_rnd_funk = [&]( motor::graphics::gen4::frontend_ptr_t fe )
                    {
                        {
                            tr->draw_text( 0, 0, 10, motor::math::vec2f_t(0.0f, -0.60f), 
                                motor::math::vec4f_t(1.0f), "Hello World Group 0!" ) ;

                            tr->draw_text( 1, 1, 25, motor::math::vec2f_t(0.0f, -0.7f), 
                                motor::math::vec4f_t(1.0f), "Hello World Group 1!" ) ;

                            static uint_t number = 0 ;
                            ++number ;
                            tr->draw_text( 0, 1, 120, motor::math::vec2f_t(0.0f, -0.5f), 
                                motor::math::vec4f_t(1.0f), motor::to_string(number) ) ;

                            tr->prepare_for_rendering(fe) ;
                        }

                        for( size_t i=0; i<10; ++i )
                        {
                            float_t const yoff = 0.08f * float_t(i) ;
                            size_t const pt = 25 - i * 2 ;
                            tr->draw_text( 0, 0, pt, motor::math::vec2f_t(-0.8f, 0.40f-yoff), 
                                motor::math::vec4f_t(1.0f), "Hello World! This is changing point ("+
                                motor::to_string(pt) +") size." ) ;
                        }

                        // render text layer 0 to screen
                        {
                            tr->render( fe, 0 ) ;
                            tr->render( fe, 1 ) ;
                        }
                    } ;

                    if( wnd->render_frame< motor::graphics::gen4::frontend_t >( my_rnd_funk ) )
                    {
                        // funk has been rendered.
                    }
                }
            }

            motor::memory::release_ptr( tr ) ;
            motor::memory::release_ptr( wnd ) ;
        }

        motor::memory::release_ptr( msgl_out ) ;
    }) ;

    // end the program by closing the carrier
    auto fut_end = std::async( std::launch::async, [&]( void_t )
    {
        fut_update_loop.wait() ;

        carrier->close() ;
    } ) ;

    auto const ret = carrier->exec() ;
    
    motor::memory::release_ptr( carrier ) ;

    motor::io::global_t::deinit() ;
    motor::concurrent::global_t::deinit() ;
    motor::log::global::deinit() ;
    motor::memory::global::dump_to_std() ;


    return ret ;
}