
#include <motor/platform/global.h>

#include <motor/gfx/sprite/sprite_render_2d.h>

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

        motor::graphics::image_object_t img_obj ;
        motor::gfx::sprite_render_2d_t sr ;

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

            // image configuration
            {
                motor::format::module_registry_mtr_t mod_reg = motor::format::global::register_default_modules( 
                    motor::shared( motor::format::module_registry_t() ) ) ;

                motor::format::future_item_t items[4] = 
                {
                    mod_reg->import_from( motor::io::location_t( "images.industrial.industrial.v2.png" ), &db ),
                    mod_reg->import_from( motor::io::location_t( "images.Paper-Pixels-8x8.Enemies.png" ), &db ),
                    mod_reg->import_from( motor::io::location_t( "images.Paper-Pixels-8x8.Player.png" ), &db ),
                    mod_reg->import_from( motor::io::location_t( "images.Paper-Pixels-8x8.Tiles.png" ), &db )
                } ;

                // taking all slices
                motor::graphics::image_t img ;

                // load each slice into the image
                for( size_t i=0; i<4; ++i )
                {
                    if( auto * ii = dynamic_cast<motor::format::image_item_mtr_t>( items[i].get() ); ii != nullptr )
                    {
                        img.append( *ii->img ) ;
                        motor::memory::release_ptr( ii->img ) ;
                        motor::memory::release_ptr( ii ) ;
                    }
                    else
                    {
                        motor::memory::release_ptr( ii ) ;
                        std::exit( 1 ) ;
                    }
                }

                img_obj = motor::graphics::image_object_t( 
                    "image_array", std::move( img ) )
                    .set_type( motor::graphics::texture_type::texture_2d_array )
                    .set_wrap( motor::graphics::texture_wrap_mode::wrap_s, motor::graphics::texture_wrap_type::repeat )
                    .set_wrap( motor::graphics::texture_wrap_mode::wrap_t, motor::graphics::texture_wrap_type::repeat )
                    .set_filter( motor::graphics::texture_filter_mode::min_filter, motor::graphics::texture_filter_type::nearest )
                    .set_filter( motor::graphics::texture_filter_mode::mag_filter, motor::graphics::texture_filter_type::nearest );

                motor::memory::release_ptr( mod_reg ) ;
            }

            sr.init( "my_sprite_render", "image_array" ) ;
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
            static float_t inc = 0.0f ;

            motor::math::vec2f_t const max_dims = motor::math::vec2f_t( img_obj.image().get_dims() ) ;

            motor::math::vec2f_t pos( -1.0f, 0.5f ) ;

            // take sprite from image plane 1. value are setup for that.
            for( size_t i=0; i<1000; ++i )
            {
                sr.draw( 0, 
                    pos + motor::math::vec2f_t( 0.0f, -0.4f ), 
                    motor::math::mat2f_t::make_rotation_matrix( std::sin(inc*2.0f), std::cos(inc) ),
                    motor::math::vec2f_t(1.6f),
                    motor::math::vec4f_t(34.0f, 112.0f-23.0f, 40.0f, 112.0f-16.0f) / motor::math::vec4f_t(max_dims,max_dims), 
                    1 ) ;

                pos += motor::math::vec2f_t( float_t(4) / 1000.0f, 
                    std::sin((float_t(i)/50.0f)*2.0f*motor::math::constants<float_t>::pi())*0.05f) ;
            }

            pos = motor::math::vec2f_t( 1.0f, -0.3f ) ;

            {
                size_t const max_objects = 100;
                for( size_t i=0; i<max_objects; ++i )
                {
                    size_t const idx = 999 - i ;
                    sr.draw( 1, 
                        pos + motor::math::vec2f_t( 0.0f, -0.4f ), 
                        motor::math::mat2f_t().identity(),
                        motor::math::vec2f_t(10.1f),
                        motor::math::vec4f_t(18.0f,512.0f-398.0f,29.0f,512.0f-385.0f) / motor::math::vec4f_t(max_dims,max_dims), 
                        0 ) ;

                    pos -= motor::math::vec2f_t( (float_t(i) / float_t(max_objects)), 0.0f ) ;
                }
            }

            sr.draw( 1, 
                    motor::math::vec2f_t( 0.0f, 0.0f ), 
                    motor::math::mat2f_t().identity(),
                    motor::math::vec2f_t(10.5f),
                    motor::math::vec4f_t(9.0f, 320.0f-287.0f, 24.0f, 320.0f-260.0f) / motor::math::vec4f_t(max_dims,max_dims), 
                    3 ) ;

            inc += 0.01f  ;
            inc = motor::math::fn<float_t>::mod( inc, 1.0f ) ;
            
            sr.prepare_for_rendering() ;
        } 

        virtual void_t on_render( this_t::window_id_t const wid, motor::graphics::gen4::frontend_ptr_t fe, 
            motor::application::app::render_data_in_t rd ) noexcept 
        {
            if( rd.first_frame )
            {
                fe->configure<motor::graphics::image_object_t>( &img_obj ) ;
                sr.configure( fe ) ;
            }

            // render text layer 0 to screen
            {
                sr.prepare_for_rendering( fe ) ;
                sr.render( fe, 0 ) ;
                sr.render( fe, 1 ) ;
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
