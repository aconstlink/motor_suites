#include <motor/platform/global.h>
#include <motor/application/window/window_message_listener.h>

#include <motor/graphics/frontend/gen4/frontend.h>

#include <motor/log/global.h>
#include <motor/device/global.h>
#include <motor/memory/global.h>

#include <future>

int main( int argc, char ** argv )
{
    using namespace motor::core::types ;

    motor::application::carrier_mtr_t carrier = motor::platform::global_t::create_carrier() ;

    auto fut_update_loop = std::async( std::launch::async, [&]( void_t )
    {
        motor::application::window_message_listener_mtr_t msgl_out = motor::memory::create_ptr<
            motor::application::window_message_listener>( "[out] : message listener" ) ;

        motor::graphics::state_object_t root_so ;

        auto my_rnd_funk = [&]( motor::graphics::gen4::frontend_ptr_t fe )
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
                rss.polygon_s.ss.ff = motor::graphics::front_face::counter_clock_wise ;
                rss.polygon_s.ss.cm = motor::graphics::cull_mode::back ;
                rss.clear_s.do_change = true ;
                rss.clear_s.ss.clear_color = motor::math::vec4f_t(0.5f, 0.9f, 0.5f, 1.0f ) ;
                rss.clear_s.ss.do_activate = true ;
                rss.clear_s.ss.do_color_clear = true ;
                rss.clear_s.ss.do_depth_clear = true ;
                rss.view_s.do_change = true ;
                rss.view_s.ss.do_activate = true ;
                rss.view_s.ss.vp = motor::math::vec4ui_t( 0, 0, 500, 500 ) ;
                so.add_render_state_set( rss ) ;
            }

            root_so = std::move( so ) ; 
            fe->configure( motor::delay(&root_so) ) ;
            fe->push( motor::delay(&root_so) ) ;
            fe->pop( motor::graphics::gen4::backend::pop_type::render_state ) ; 
        } ;

        motor::application::iwindow_mtr_t wnd1 ;
        motor::application::iwindow_mtr_t wnd2 ;

        // create window 1
        {
            motor::application::window_info_t wi ;
            wi.x = 100 ;
            wi.y = 100 ;
            wi.w = 800 ;
            wi.h = 600 ;
            //wi.gen = ... is auto
            
            wnd1 = carrier->create_window( wi ) ;
            

            wnd1->register_out( motor::share( msgl_out ) ) ;

            wnd1->send_message( motor::application::show_message( { true } ) ) ;
            wnd1->send_message( motor::application::cursor_message_t( {false} ) ) ;
        }

        // create window 2
        {
            motor::application::window_info_t wi ;
            wi.x = 200 ;
            wi.y = 200 ;
            wi.w = 800 ;
            wi.h = 500 ;
            wi.gen = motor::application::graphics_generation::gen4_gl4 ;

            wnd2 = carrier->create_window( wi ) ;
            
            wnd2->register_out( motor::share( msgl_out ) ) ;

            wnd2->send_message( motor::application::show_message( { true } ) ) ;
            wnd2->send_message( motor::application::cursor_message_t( {false} ) ) ;
        }

        {
            while( true ) 
            {
                std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) ) ;
                
                motor::application::window_message_listener_t::state_vector_t sv ;
                if( msgl_out->swap_and_reset( sv ) )
                {
                    if( sv.close_changed )
                    {
                        break ;
                    }
                }

                if( wnd1->render_frame< motor::graphics::gen4::frontend_t >( my_rnd_funk ) )
                {
                    // funk has been rendered.
                }

                if( wnd2->render_frame< motor::graphics::gen4::frontend_t >( my_rnd_funk ) )
                {
                    // funk has been rendered.
                }
            }
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

    motor::device::global_t::deinit() ;
    motor::log::global::deinit() ;
    motor::memory::global::dump_to_std() ;


    return ret ;
}