
#include <motor/platform/global.h>

#include <motor/gfx/sprite/sprite_render_2d.h>

#include <motor/format/global.h>
#include <motor/format/future_items.hpp>
#include <motor/format/motor/motor_module.h>

#include <motor/property/property_sheet.hpp>

#include <motor/math/interpolation/interpolate.hpp>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>
#include <motor/profiling/global.h>

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

        motor::vector< motor::gfx::sprite_sheet_t > sheets ;

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

            // import motor file
            {
                motor::format::module_registry_mtr_t mod_reg = motor::format::global::register_default_modules( 
                    motor::shared( motor::format::module_registry_t() ) ) ;

                auto item = mod_reg->import_from( motor::io::location_t( "sprite_sheet.motor" ), &db ) ;
        
                if( auto * ni = dynamic_cast<motor::format::motor_item_mtr_t>( item.get() ); ni != nullptr )
                {
                    motor::format::motor_document_t doc = std::move( ni->doc ) ;

                    // taking all slices
                    motor::graphics::image_t imgs ;

                    // load images
                    {
                        motor::vector< motor::format::future_item_t > futures ;
                        for( auto const & ss : doc.sprite_sheets )
                        {
                            auto const l = motor::io::location_t::from_path( motor::io::path_t(ss.image.src) ) ;
                            futures.emplace_back( mod_reg->import_from( l, &db ) ) ;
                        }

                        for( size_t i=0; i<doc.sprite_sheets.size(); ++i )
                        {
                            if( auto * ii = dynamic_cast<motor::format::image_item_mtr_t>( futures[i].get() ) ; ii != nullptr )
                            {
                                imgs.append( *ii->img ) ;
                                motor::memory::release_ptr( ii->img ) ;
                                motor::memory::release_ptr( ii ) ;
                            }
                            else
                            {
                                motor::memory::release_ptr( ii ) ;
                                std::exit( 1 ) ;
                            }

                            motor::gfx::sprite_sheet ss ;
                            sheets.emplace_back( ss ) ;
                        }
                    }

                    // make sprite animation infos
                    {
                        // as an image array is used, the max dims need to be
                        // used to compute the particular rect infos
                        auto dims = imgs.get_dims() ;
                    
                        size_t i=0 ;
                        for( auto const & ss : doc.sprite_sheets )
                        {
                            auto & sheet = sheets[i++] ;

                            for( auto const & s : ss.sprites )
                            {
                                motor::math::vec4f_t const rect = 
                                    (motor::math::vec4f_t( s.animation.rect ) + 
                                        motor::math::vec4f_t(0.5f,0.5f, 0.5f, 0.5f))/ 
                                    motor::math::vec4f_t( dims.xy(), dims.xy() )  ;

                                motor::math::vec2f_t const pivot =
                                    motor::math::vec2f_t( s.animation.pivot ) / 
                                    motor::math::vec2f_t( dims.xy() ) ;

                                motor::gfx::sprite_sheet::sprite s_ ;
                                s_.rect = rect ;
                                s_.pivot = pivot ;

                                sheet.rects.emplace_back( s_ ) ;
                            }

                            motor::map< motor::string_t, size_t > object_map ;
                            for( auto const & a : ss.animations )
                            {
                                size_t obj_id = 0 ;
                                {
                                    auto iter = object_map.find( a.object ) ;
                                    if( iter != object_map.end() ) obj_id = iter->second ;
                                    else 
                                    {
                                        obj_id = sheet.objects.size() ;
                                        sheet.objects.emplace_back( motor::gfx::sprite_sheet::object { a.object, {} } ) ;
                                    }
                                }

                                motor::gfx::sprite_sheet::animation a_ ;

                                size_t tp = 0 ;
                                for( auto const & f : a.frames )
                                {
                                    auto iter = std::find_if( ss.sprites.begin(), ss.sprites.end(), 
                                        [&]( motor::format::motor_document_t::sprite_sheet_t::sprite_cref_t s )
                                    {
                                        return s.name == f.sprite ;
                                    } ) ;

                                    size_t const d = std::distance( ss.sprites.begin(), iter ) ;
                                    motor::gfx::sprite_sheet::animation::sprite s_ ;
                                    s_.begin = tp ;
                                    s_.end = tp + f.duration ;
                                    s_.idx = d ;
                                    a_.sprites.emplace_back( s_ ) ;

                                    tp = s_.end ;
                                }
                                a_.duration = tp ;
                                a_.name = a.name ;
                                sheet.objects[obj_id].animations.emplace_back( std::move( a_ ) ) ;
                            }
                        }
                    }

                    img_obj = motor::graphics::image_object_t( 
                        "image_array", std::move( imgs ) )
                        .set_type( motor::graphics::texture_type::texture_2d_array )
                        .set_wrap( motor::graphics::texture_wrap_mode::wrap_s, motor::graphics::texture_wrap_type::repeat )
                        .set_wrap( motor::graphics::texture_wrap_mode::wrap_t, motor::graphics::texture_wrap_type::repeat )
                        .set_filter( motor::graphics::texture_filter_mode::min_filter, motor::graphics::texture_filter_type::nearest )
                        .set_filter( motor::graphics::texture_filter_mode::mag_filter, motor::graphics::texture_filter_type::nearest );

                    motor::memory::release_ptr( ni ) ;
                    motor::memory::release_ptr( mod_reg ) ;
                }
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

        virtual void_t on_graphics( motor::application::app::graphics_data_in_t gd ) noexcept 
        {
            size_t const sheet = 0 ;
            size_t const ani = 0 ;
            static size_t ani_time = 0 ;

            if( ani_time > sheets[sheet].objects[0].animations[ani].duration ) ani_time = 0 ;

            
            
            ani_time += gd.micro_dt / 1000 ;

            // animation driven
            {
                auto & s = sheets[0].objects[0].animations[0].sprites[0] ;
                auto const & rect = sheets[sheet].rects[s.idx] ;
                
                float_t const rw = rect.rect.z() - rect.rect.x() ; 
                static size_t r = 0 ;
                size_t const w = 100 ;
                size_t const h = 100 ;

                r+=4 ;

                if( r > w*h ) r = 0 ;

                int_t const ne = 2000;
                for( int_t i=0; i<ne; ++i )
                {
                    int_t r2 = std::max( int_t( r ) - (i*2), 0 );
                    float_t const x = (float_t( r2 % w ) / float_t(w )) *2.0f -1.0f ;
                    float_t const y = (float_t( r2 / w ) / float_t(h )) *2.0f -1.0f ;

                    sr.draw( 0, 
                        motor::math::vec2f_t( x, y ),
                        motor::math::mat2f_t().identity(),
                        motor::math::vec2f_t(5.0f),
                        rect.rect,  
                        sheet, rect.pivot, 
                        motor::math::vec4f_t(1.0f) ) ;
                }
            }

            for( auto const & s : sheets[sheet].objects[0].animations[ani].sprites )
            {
                if( ani_time >= s.begin && ani_time < s.end )
                {
                    auto const & rect = sheets[sheet].rects[s.idx] ;
                    motor::math::vec2f_t pos( -0.0f, 0.0f ) ;
                    sr.draw( 0, 
                        pos, 
                        motor::math::mat2f_t().identity(),
                        motor::math::vec2f_t(10.0f),
                        rect.rect,  
                        sheet, rect.pivot, 
                        motor::math::vec4f_t(1.0f, 0.5f, 0.25f, 0.5f ) ) ;
                    break ;
                }
            }

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
    motor::profiling::global_t::deinit() ;
    motor::memory::global::dump_to_std() ;


    return ret ;
}
