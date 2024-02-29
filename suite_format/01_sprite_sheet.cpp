


#include <motor/format/global.h>
#include <motor/format/motor/motor_module.h>

#include <motor/io/database.h>
#include <motor/log/global.h>

#include <motor/math/vector/vector4.hpp>

namespace this_file
{
    using namespace motor::core::types ;

    struct sprite_sheet
    {
        struct animation
        {
            struct sprite
            {
                size_t idx ;
                size_t begin ;
                size_t end ;
            };
            size_t duration ;
            motor::vector< sprite > sprites ;

        };
        motor::vector< animation > animations ;

        struct sprite
        {
            motor::math::vec4f_t rect ;
            motor::math::vec2f_t pivot ;
        };
        motor::vector< sprite > rects ;
    };
}

//
int main( int argc, char ** argv )
{
    motor::vector< this_file::sprite_sheet > sheets ;

    motor::io::database_mtr_t db = motor::shared( motor::io::database_t( motor::io::path_t( DATAPATH ), "./working", "data" ) ) ;

    motor::format::module_registry_mtr_t mod_reg = motor::format::global::register_default_modules( 
        motor::shared( motor::format::module_registry_t(), "mod registry"  ) ) ;
    
    // import motor file
    {
        auto item = mod_reg->import_from( motor::io::location_t( "sprite_sheet.motor" ), db ) ;
        
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
                    futures.emplace_back( mod_reg->import_from( l, db ) ) ;
                }

                for( size_t i=0; i<doc.sprite_sheets.size(); ++i )
                {
                    if( auto * ii = dynamic_cast<motor::format::image_item_mtr_t>( futures[i].get() ) ; ii != nullptr )
                    {
                        imgs.append( *ii->img ) ;
                    }

                    this_file::sprite_sheet ss ;
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

                        this_file::sprite_sheet::sprite s_ ;
                        s_.rect = rect ;
                        s_.pivot = pivot ;

                        sheet.rects.emplace_back( s_ ) ;
                    }

                    for( auto const & a : ss.animations )
                    {
                        this_file::sprite_sheet::animation a_ ;

                        size_t tp = 0 ;
                        for( auto const & f : a.frames )
                        {
                            auto iter = std::find_if( ss.sprites.begin(), ss.sprites.end(), 
                                [&]( motor::format::motor_document_t::sprite_sheet_t::sprite_cref_t s )
                            {
                                return s.name == f.sprite ;
                            } ) ;

                            size_t const d = std::distance( ss.sprites.begin(), iter ) ;
                            this_file::sprite_sheet::animation::sprite s_ ;
                            s_.begin = tp ;
                            s_.end = tp + f.duration ;
                            s_.idx = d ;
                            a_.sprites.emplace_back( s_ ) ;

                            tp = s_.end ;
                        }
                        a_.duration = tp ;
                        sheet.animations.emplace_back( std::move( a_ ) ) ;
                    }
                }
            }
        }
    }

    // export 
    {
        motor::format::motor_document doc ;
        
        for( size_t no = 0; no < 3; ++no )
        {
            motor::format::motor_document_t::sprite_sheet_t ss ;
            ss.name = "sprite_sheet_name." + motor::to_string(no) ;

            {
                 motor::format::motor_document_t::sprite_sheet_t::image_t img ;
                 img.src = "images/a_img."+motor::to_string(no)+".png" ;
                 ss.image = img ;
            }

            for( size_t i=0; i<10; ++i )
            {
                motor::format::motor_document_t::sprite_sheet_t::sprite_t sp ;
                sp.name = "sprite." + motor::to_string(i) ;
                sp.animation.rect = motor::math::vec4ui_t() ;
                sp.animation.pivot = motor::math::vec2i_t() ;

                ss.sprites.emplace_back( sp ) ;
            }

            for( size_t i=0; i<3; ++i )
            {
                motor::format::motor_document_t::sprite_sheet_t::animation_t ani ;
                ani.name = "animation." + motor::to_string(i) ;

                for( size_t j=0; j<10; ++j )
                {
                    motor::format::motor_document_t::sprite_sheet_t::animation_t::frame_t fr ;
                    fr.sprite = "sprite." +motor::to_string(j) ;
                    fr.duration = 50 ;
                    ani.frames.emplace_back( std::move( fr ) ) ;
                }

                ss.animations.emplace_back( std::move( ani ) ) ;
            }

            doc.sprite_sheets.emplace_back( ss ) ;
        }
        
        auto item = mod_reg->export_to( motor::io::location_t( "sprite_sheet_motor.motor" ), db, 
            motor::shared( motor::format::motor_item_t( std::move( doc ) ) ) ) ;

        if( auto * si = dynamic_cast<motor::format::status_item_mtr_t>(item.get()); si != nullptr )
        {
            motor::log::global_t::status( si->msg ) ;
        }
    }

    // import export the same file
    {
        auto im_item = mod_reg->import_from( motor::io::location_t( "sprite_sheet.motor" ), db ) ;

        
        if( auto * ni = dynamic_cast<motor::format::motor_item_mtr_t>( im_item.get() ); ni != nullptr )
        {
            motor::format::motor_document_t doc = std::move( ni->doc ) ;
            auto ex_item = mod_reg->export_to( motor::io::location_t( "imported_one.motor" ), db, 
                motor::shared( motor::format::motor_item_t( std::move( doc ) ) ) ) ;

            if( auto * si = dynamic_cast<motor::format::status_item_mtr_t>(ex_item.get()); si != nullptr )
            {
                motor::log::global_t::status( si->msg ) ;
            }
        }
    }

    motor::memory::release_ptr( mod_reg ) ;
    motor::memory::release_ptr( db ) ;

    motor::io::global_t::deinit() ;
    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;

    return 0 ;
}
