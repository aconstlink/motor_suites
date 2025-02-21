
#include <motor/platform/global.h>

#include <motor/gfx/primitive/primitive_render_2d.h>

#include <motor/format/global.h>
#include <motor/format/future_items.hpp>
#include <motor/property/property_sheet.hpp>
#include <motor/profiling/probe_guard.hpp>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>
#include <motor/profiling/global.h>

#include <future>

#define DRAW_SINGLE 0

namespace this_file
{
    using namespace motor::core::types ;

    class my_app : public motor::application::app
    {
        motor_this_typedefs( my_app ) ;

        motor::gfx::primitive_render_2d_t pr ;

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

                this_t::send_window_message( this_t::create_window( wi ), [&]( motor::application::app::window_view & wnd )
                {
                    wnd.send_message( motor::application::show_message( { true } ) ) ;
                    wnd.send_message( motor::application::cursor_message_t( {true} ) ) ;
                    wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                } ) ;
            }

            pr.init( "my_prim_render" ) ;
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
        }

        virtual void_t on_graphics( motor::application::app::graphics_data_in_t ) noexcept
        {
            MOTOR_PROBE( "application", "on_graphics" ) ;

            static float_t inc = 0.0f ;

            motor::math::vec2f_t pos( -1.0f, 0.5f ) ;

            #if DRAW_SINGLE
            for ( size_t i = 0; i < 1000; ++i )
            {
                pr.draw_line( 0,
                    pos + motor::math::vec2f_t( 0.0f, -0.4f ),
                    pos + motor::math::vec2f_t( 0.0f, +0.4f ),
                    motor::math::vec4f_t(
                        ( inc + float_t( i + 600 ) / 1000.0f ) / 2.0f,
                        ( inc + float_t( ( i + 300 ) % 1000 ) / 1000.0f ) / 2.0f,
                        ( inc + float_t( ( i + 900 ) % 1000 ) / 1000.0f ) / 2.0f,
                        1.0f ) ) ;

                pos += motor::math::vec2f_t( float_t( i ) / 1000.0f, 0.0f ) ;
            }
            #else
            pr.draw_lines( 0, 1000, [&] ( size_t const i ) 
            {
                float_t const x = float_t( i % 250 ) ;
                float_t const y = float_t( i / 250 ) ;

                auto const color = motor::math::vec4f_t(
                    ( inc + float_t( i + 600 ) / 1000.0f ) / 2.0f,
                    ( inc + float_t( ( i + 300 ) % 1000 ) / 1000.0f ) / 2.0f,
                    ( inc + float_t( ( i + 900 ) % 1000 ) / 1000.0f ) / 2.0f,
                    1.0f ) ;

                auto const p = pos + motor::math::vec2f_t( float_t( x*4.0f ) / 500.0f, y * 0.25f ) ;

                auto const p0 = p + motor::math::vec2f_t( 0.0f, -0.2f ) ;
                auto const p1 = p + motor::math::vec2f_t( 0.0f, +0.2f ) ;

                return motor::gfx::line_render_2d::line_t { { p0, p1 }, color } ;
            } ) ;
            #endif

            pos = motor::math::vec2f_t( 1.0f, -0.5f ) ;

            #if DRAW_SINGLE
            for( size_t i=0; i<1000; ++i )
            {
                size_t const idx = 999 - i ;
                pr.draw_line( 1, 
                    pos + motor::math::vec2f_t( 0.0f, -0.4f ), 
                    pos + motor::math::vec2f_t( 0.0f, +0.4f ),
                    motor::math::vec4f_t( 
                        (float_t(idx)/1000.0f), 
                        (float_t((2*idx+300)%1000)/1000.0f), 
                        (float_t((idx+900)%1000)/1000.0f)/2.0f, 
                        1.0f)) ;

                pos -= motor::math::vec2f_t( (float_t(i) / 1000.0f), 0.0f ) ;
            }
            #else
            pr.draw_lines( 0, 1000, [&] ( size_t const i )
            {
                size_t const idx = 999 - i ;

                auto const color = motor::math::vec4f_t(
                    ( float_t( idx ) / 1000.0f ),
                    ( float_t( ( 2 * idx + 300 ) % 1000 ) / 1000.0f ),
                    ( float_t( ( idx + 900 ) % 1000 ) / 1000.0f ) / 2.0f,
                    1.0f ) ;

                auto const p = pos - motor::math::vec2f_t( ( float_t( i ) / 1000.0f ), 0.0f ) ;

                auto const p0 = p + motor::math::vec2f_t( 0.0f, -0.4f ) ;
                auto const p1 = p + motor::math::vec2f_t( 0.0f, +0.4f ) ;

                

                return motor::gfx::line_render_2d::line_t { { p0, p1 }, color } ;
            } ) ;
            #endif
            
            // tris
            {
                pos = motor::math::vec2f_t( 1.0f, -0.0f ) ;
                size_t const max_tris = 50 ;

                #if DRAW_SINGLE
                for( size_t i=0; i<max_tris; ++i )
                {
                    size_t const idx = max_tris - 1 - i ;
                    pr.draw_tri( 0, 
                        pos + motor::math::vec2f_t( -.02f, -0.04f ), 
                        pos + motor::math::vec2f_t( +0.02f, -0.04f ),
                        pos + motor::math::vec2f_t( 0.0f, 0.04f ),
                        motor::math::vec4f_t( 
                            (inc + float_t((idx+600)%max_tris)/float_t(max_tris))/2.0f, 
                            (inc + float_t((idx+300)%max_tris)/float_t(max_tris))/2.0f, 
                            (inc + float_t((idx+900)%max_tris)/float_t(max_tris))/2.0f, 
                            0.5f + (float_t(idx)/float_t(max_tris))* 0.5f )  +
                        motor::math::vec4f_t( size_t( inc * float_t(max_tris) ) == i, 0.0f,0.0f,0.0f )
                    )  ;

                    pos -= motor::math::vec2f_t( (1.0f/float_t(max_tris))*2.0f, 0.0f ) ;
                }
                #else
                pr.draw_tris( 1, max_tris, [&] ( size_t const i ) 
                {
                    size_t const idx = max_tris - 1 - i ;

                    auto const color = motor::math::vec4f_t(
                        ( inc + float_t( ( idx + 600 ) % max_tris ) / float_t( max_tris ) ) / 2.0f,
                        ( inc + float_t( ( idx + 300 ) % max_tris ) / float_t( max_tris ) ) / 2.0f,
                        ( inc + float_t( ( idx + 900 ) % max_tris ) / float_t( max_tris ) ) / 2.0f,
                        0.5f + ( float_t( idx ) / float_t( max_tris ) ) * 0.5f ) +
                        motor::math::vec4f_t( size_t( inc * float_t( max_tris ) ) == i, 0.0f, 0.0f, 0.0f ) ;

                    auto const p0 = pos + motor::math::vec2f_t( -.02f, -0.04f ) ;
                    auto const p1 = pos + motor::math::vec2f_t( +0.02f, -0.04f ) ;
                    auto const p2 = pos + motor::math::vec2f_t( 0.0f, 0.04f ) ;

                    pos -= motor::math::vec2f_t( ( 1.0f / float_t( max_tris ) ) * 2.0f, 0.0f ) ;

                    return motor::gfx::tri_render_2d::tri_md_t { { p0, p1, p2 }, color } ;
                } ) ;
                #endif
            }
            
            // tris
            {
                pos = motor::math::vec2f_t( -1.0f, -0.0f ) ;

                size_t const max_tris = 50 ;
                #if DRAW_SINGLE
                for( size_t i=0; i<max_tris; ++i )
                {
                    size_t const idx = i ;
                    pr.draw_tri( 0, 
                        pos + motor::math::vec2f_t( +.02f, 0.04f ), 
                        pos + motor::math::vec2f_t( -0.02f, 0.04f ),
                        pos + motor::math::vec2f_t( 0.0f, -0.04f ),
                        motor::math::vec4f_t( 
                            (inc + float_t((idx+600)%max_tris)/float_t(max_tris))/2.0f, 
                            (inc + float_t((idx+300)%max_tris)/float_t(max_tris))/2.0f, 
                            (inc + float_t((idx+900)%max_tris)/float_t(max_tris))/2.0f, 
                            0.5f + float_t(idx)/float_t(max_tris) * 0.5f )  +
                        motor::math::vec4f_t( size_t( inc * float_t(max_tris) ) == idx, 0.0f,0.0f,0.0f )
                    ) ;

                    pos += motor::math::vec2f_t( (1.0f/float_t(max_tris))*2.0f, 0.0f ) ;
                }
                #else
                pr.draw_tris( 1, max_tris, [&] ( size_t const i )
                {
                    size_t const idx = max_tris - 1 - i ;

                    auto const color = motor::math::vec4f_t(
                        ( inc + float_t( ( idx + 600 ) % max_tris ) / float_t( max_tris ) ) / 2.0f,
                        ( inc + float_t( ( idx + 300 ) % max_tris ) / float_t( max_tris ) ) / 2.0f,
                        ( inc + float_t( ( idx + 900 ) % max_tris ) / float_t( max_tris ) ) / 2.0f,
                        0.5f + ( float_t( idx ) / float_t( max_tris ) ) * 0.5f ) +
                        motor::math::vec4f_t( size_t( inc * float_t( max_tris ) ) == i, 0.0f, 0.0f, 0.0f ) ;

                    auto const p = pos + motor::math::vec2f_t( ( float_t(i) / float_t( max_tris ) ) * 2.0f, 0.0f ) ;
                    auto const p0 = p + motor::math::vec2f_t( +.02f, 0.04f ) ;
                    auto const p1 = p + motor::math::vec2f_t( -0.02f, 0.04f ) ;
                    auto const p2 = p + motor::math::vec2f_t( 0.0f, -0.04f ) ;

                    return motor::gfx::tri_render_2d::tri_md_t { { p0, p1, p2 }, color } ;
                } ) ;
                #endif
            }

            // rects
            {
                pos = motor::math::vec2f_t( -1.0f, 0.1f ) ;

                size_t const max_rects = 40 ;
                
                #if DRAW_SINGLE
                for( size_t i=0; i< max_rects; ++i )
                {
                    size_t const idx = i ;
                    pr.draw_rect( 0, 
                        pos + motor::math::vec2f_t( -.02f, -0.04f ), 
                        pos + motor::math::vec2f_t( -.02f, +0.04f ),
                        pos + motor::math::vec2f_t( +.02f, +0.04f ),
                        pos + motor::math::vec2f_t( +.02f, -0.04f ),
                        motor::math::vec4f_t( 0.5f,0.5f,0.5f,
                            0.5f + float_t(idx)/float_t( max_rects ) * 0.5f )  +
                        motor::math::vec4f_t( size_t( inc * float_t( max_rects ) ) == idx, 0.0f,0.0f,0.0f ),
                        motor::math::vec4f_t( 0.5f,1.0f,0.5f,1.0f)
                    ) ;

                    pos += motor::math::vec2f_t( (1.0f/float_t( max_rects ))*2.0f, 0.0f ) ;
                }
                #else
                pr.draw_rects_border( 0, max_rects, [&] ( size_t const i ) 
                {
                    size_t const idx = i ;

                    auto const orig = pos + motor::math::vec2f_t( ( float_t(i) / float_t( max_rects ) ) * 2.0f, 0.0f ) ;

                    auto const p0 = orig + motor::math::vec2f_t( -.02f, -0.04f ) ;
                    auto const p1 = orig + motor::math::vec2f_t( -.02f, +0.04f ) ;
                    auto const p2 = orig + motor::math::vec2f_t( +.02f, +0.04f ) ;
                    auto const p3 = orig + motor::math::vec2f_t( +.02f, -0.04f ) ;

                    auto const color = motor::math::vec4f_t( 0.5f, 0.5f, 0.5f,
                        0.5f + float_t( idx ) / float_t( max_rects ) * 0.5f ) +
                        motor::math::vec4f_t( size_t( inc * float_t( max_rects ) ) == idx, 0.0f, 0.0f, 0.0f ) ;

                    auto const border = motor::math::vec4f_t( 0.5f, 1.0f, 0.5f, 1.0f ) ;

                    return motor::gfx::primitive_render_2d::rect_t {
                        { p0, p1, p2, p3 }, color, border } ;
                } ) ;
                #endif
            }

            // circles
            {
                pos = motor::math::vec2f_t( 1.0f, -0.1f ) ;

                size_t const max_circles = 50 ;

                #if DRAW_SINGLE
                for ( size_t i = 0; i < max_circles; ++i )
                {
                    size_t const idx = max_circles - 1 - i ;
                    pr.draw_circle( 1, 20, pos, float_t( 0.04f ),
                        motor::math::vec4f_t(
                            0.5f, 0.5f, 0.5f,
                            0.5f + ( float_t( idx ) / float_t( max_circles ) ) * 0.5f ) +
                        motor::math::vec4f_t( size_t( inc * float_t( max_circles ) ) == i, 0.0f, 0.0f, 0.0f ),
                        motor::math::vec4f_t( 0.5f, 0.5f, 1.0f, 1.0f )
                    )  ;

                    pos -= motor::math::vec2f_t( ( 1.0f / float_t( max_circles ) ) * 2.0f, 0.0f ) ;
                }
                #else
                pr.draw_circles_border( 1, 20, max_circles, [&] ( size_t const i ) 
                {
                    size_t const idx = max_circles - 1 - i ;

                    auto const p = pos - motor::math::vec2f_t( ( float_t(i) / float_t( max_circles ) ) * 2.0f, 0.0f ) ; ;

                    motor::math::vec4f_t const color = motor::math::vec4f_t(
                        0.5f, 0.5f, 0.5f,
                        0.5f + ( float_t( idx ) / float_t( max_circles ) ) * 0.5f ) +
                        motor::math::vec4f_t( size_t( inc * float_t( max_circles ) ) == i, 0.0f, 0.0f, 0.0f ) ;

                    motor::math::vec4f_t const border = 
                        motor::math::vec4f_t( 0.5f, 0.5f, 1.0f, 1.0f ) ;


                    return motor::gfx::primitive_render_2d::circle_t { p, float_t( 0.04f ), color, border } ;
                } ) ;
                #endif
            }

            // circles
            {
                pos = motor::math::vec2f_t( -1.0f, 0.2f ) ;

                size_t const max_circles = 50 ;

                #if DRAW_SINGLE
                for ( size_t i = 0; i < max_circles; ++i )
                {
                    size_t const idx = i ;
                    pr.draw_circle( 1, 20, pos, float_t( 0.04f ),
                        motor::math::vec4f_t(
                            0.5f, 0.5f, 0.5f,
                            0.5f + ( float_t( idx ) / float_t( max_circles ) ) * 0.5f ) +
                        motor::math::vec4f_t( size_t( inc * float_t( max_circles ) ) == i, 0.0f, 0.0f, 0.0f ),
                        motor::math::vec4f_t( 0.5f, 0.5f, 1.0f, 1.0f )
                    )  ;

                    pos += motor::math::vec2f_t( ( 1.0f / float_t( max_circles ) ) * 2.0f, 0.0f ) ;
                }
                #else
                pr.draw_circles( 1, 20, max_circles, [&] ( size_t const i )
                {
                    size_t const idx =  i ;

                    auto const p = pos + motor::math::vec2f_t( ( float_t( i ) / float_t( max_circles ) ) * 2.0f, 0.0f ) ; ;

                    motor::math::vec4f_t const color = motor::math::vec4f_t(
                        0.5f, 0.5f, 0.5f,
                        0.5f + ( float_t( idx ) / float_t( max_circles ) ) * 0.5f ) +
                        motor::math::vec4f_t( size_t( inc * float_t( max_circles ) ) == i, 0.0f, 0.0f, 0.0f ) ;

                    return motor::gfx::primitive_render_2d::circle_t { p, float_t( 0.04f ), color } ;
                } ) ;
                #endif
            }

            inc += 0.01f  ;
            inc = motor::math::fn<float_t>::mod( inc, 1.0f ) ;

            pr.prepare_for_rendering() ;
        } 

        virtual void_t on_render( this_t::window_id_t const wid, motor::graphics::gen4::frontend_ptr_t fe, 
            motor::application::app::render_data_in_t rd ) noexcept 
        {
            if( rd.first_frame )
            {
                pr.configure( fe ) ;
            }

            // render text layer 0 to screen
            {
                pr.prepare_for_rendering( fe ) ;
                pr.render( fe, 0 ) ;
                pr.render( fe, 1 ) ;
                pr.render( fe, 2 ) ;
            }
        }

        virtual bool_t on_tool( this_t::window_id_t const, motor::application::app::tool_data_ref_t ) noexcept { return true ; }
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
    motor::profiling::global_t::deinit() ;
    motor::memory::global::dump_to_std() ;


    return ret ;
}
