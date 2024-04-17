
#include "00_world_2d/world.h"

#include <motor/platform/global.h>

#include <motor/gfx/camera/pinhole_camera.h>
#include <motor/gfx/font/text_render_2d.h>
#include <motor/gfx/primitive/primitive_render_2d.h>

#include <motor/format/global.h>
#include <motor/format/future_items.hpp>
#include <motor/property/property_sheet.hpp>

#include <motor/noise/method/gradient_noise.h>
#include <motor/noise/method/fbm.hpp>

#include <motor/math/utility/angle.hpp>

#include <motor/profiling/probe_guard.hpp>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>

#include <future>

#define DRAW_SINGLE 0

namespace proto
{
    using namespace motor::core::types ;

    enum class material_form
    {
        gass,
        liquid,
        solid
    } ;

    enum class material : size_t
    {
        air,
        soil,
        stone,
        grass,
        coal,
        iron,
        gold,
        diamant,
        num_materials
    };

    namespace internal
    {
        static char const * __material_names[] = {
            "air", "soil", "stone", "grass", "coal", "iron", "gold", "diamant",
        } ;
    }

    static motor::string_t to_string( proto::material const m ) noexcept
    {
        return internal::__material_names[ size_t( m ) ] ;
    }

    struct material_property
    {
        proto::material_form form ;
        float_t amplitude = 10.0f ;
        float_t factor = 0.2f ;

        motor::math::vec2f_t  divisor = 23.0f ;
        motor::math::vec2f_t  offset = 0.0f ;
        motor::math::vec4f_t color ;
    };

    using material_funk_t = std::function< bool_t (
        motor::math::vec2f_cref_t pos, motor::noise::gradient_noise_cref_t n, material_property const & props ) > ;

    using material_funktions_t = motor::vector < material_funk_t > ;

    using material_properties_t = motor::vector< proto::material_property > ;

    static material_properties_t get_default_properties( void_t ) noexcept
    {
        return material_properties_t
        {
            // air
            {
                proto::material_form::gass,
                1.0f,
                1.0f,
                motor::math::vec2f_t( 23.0f ),
                motor::math::vec2f_t( 0.0f ),
                motor::math::vec4f_t( 0.0f, 0.3f, 1.0f, 1.0f )
            },
            // soil
            {
                proto::material_form::solid,
                10.0f,
                1.0f,
                motor::math::vec2f_t( 50.0f, 100.0f ),
                motor::math::vec2f_t( 0.0f ),
                motor::math::vec4f_t( 101.0f, 67.0f, 33.0f, 255.0f ) / 255.0f
            },
            // stone
            {
                proto::material_form::solid,
                10.0f,
                1.0f,
                motor::math::vec2f_t( 50.0f, 100.0f ),
                motor::math::vec2f_t( 0.0f ),
                motor::math::vec4f_t( 101.0f, 67.0f, 33.0f, 255.0f ) / 255.0f
            },
                // grass
            {
                proto::material_form::solid,
                10.0f,
                1.0f,
                motor::math::vec2f_t( 50.0f, 100.0f ),
                motor::math::vec2f_t( 0.0f ),
                motor::math::vec4f_t( 0.0f, 1.0f, 0.0f, 1.0f )
            },
                // coal
            {
                proto::material_form::solid,
                10.0f,
                0.4f,
                motor::math::vec2f_t( 15.0f, 15.0f ),
                motor::math::vec2f_t( 0.0f ),
                motor::math::vec4f_t( 0.0f, 0.0f, 0.0f, 1.0f )
            },
                // iron
            {
                proto::material_form::solid,
                10.0f,
                0.4f,
                motor::math::vec2f_t( 18.0f, 18.0f ),
                motor::math::vec2f_t( 0.0f ),
                motor::math::vec4f_t( 185.0f / 255.0f, 169.0f / 255.0f, 180.0f / 255.0f, 1.0f )
            },
                // gold
            {
                proto::material_form::solid,
                10.0f,
                0.2f,
                motor::math::vec2f_t( 23.0f, 23.0f ),
                motor::math::vec2f_t( 0.0f ),
                motor::math::vec4f_t( 1.0f, 1.0f, 0.0f, 1.0f )
            },
                // diamant
            {
                proto::material_form::solid,
                10.0f,
                0.22f,
                motor::math::vec2f_t( 15.0f, 15.0f ),
                motor::math::vec2f_t( 14.2f, 44.2f ),
                motor::math::vec4f_t( 0.0f, 0.7f, 1.0f, 1.0f )
            }
        };
    }

    static material_funktions_t get_default_material_funktions( void_t ) noexcept
    {
        return material_funktions_t
        {
            // air
            [=] ( motor::math::vec2f_cref_t, motor::noise::gradient_noise_cref_t, material_property const & )
            {
                return true ;
            },

            // soil
            [=] ( motor::math::vec2f_cref_t pos, motor::noise::gradient_noise_cref_t gn, material_property const & props )
            {
                float_t amp_scale_0 = props.amplitude * gn.noise( pos / props.divisor ) ;
                float_t amp_scale_1 = props.amplitude * gn.noise( pos / props.divisor.yx() ) ;

                float_t amp = ( props.amplitude * amp_scale_0 ) + amp_scale_0 * amp_scale_1 ;

                int_t f = int_t( amp * std::sin( pos.x() /** frequency*/ ) - pos.y() ) ;

                return f > 0 ;
            },

            // stone
            [=] ( motor::math::vec2f_cref_t pos, motor::noise::gradient_noise_cref_t gn, material_property const & props )
            {
                float_t amp_scale_0 = props.amplitude * gn.noise( pos / props.divisor ) ;
                float_t amp_scale_1 = props.amplitude * gn.noise( pos / props.divisor.yx() ) ;

                float_t amp = ( props.amplitude * amp_scale_0 ) + amp_scale_0 * amp_scale_1 ;

                int_t f = int_t( amp * std::sin( pos.x() /** frequency*/ ) - pos.y() ) ;

                return f > 0 ;
            },  

            [=] ( motor::math::vec2f_cref_t pos, motor::noise::gradient_noise_cref_t gn, material_property const & props )
            {
                float_t amp_scale_0 = props.amplitude * gn.noise( pos / props.divisor ) ;
                float_t amp_scale_1 = props.amplitude * gn.noise( pos / props.divisor.yx() ) ;

                float_t amp = ( props.amplitude * amp_scale_0 ) + amp_scale_0 * amp_scale_1 ;

                int_t f = int_t( amp * std::sin( pos.x() /** frequency*/ ) - pos.y() ) ;

                return f > 0 ;
            },

            // grass
            [=] ( motor::math::vec2f_cref_t pos, motor::noise::gradient_noise_cref_t gn, material_property const & props )
            {
                auto p = ( (pos + props.offset) / props.divisor ) ;
                int_t v = int_t( props.amplitude * props.factor * gn.noise( p ) ) ;
                return v > 0 ;
            },

            // coal
            [=] ( motor::math::vec2f_cref_t pos, motor::noise::gradient_noise_cref_t gn, material_property const & props )
            {
                auto p = ( (pos + props.offset) / props.divisor ) ;
                int_t v = int_t( props.amplitude * props.factor * gn.noise( p ) ) ;
                return v > 0 && pos.y() < -50.0f ;
            },

            // iron
            [=] ( motor::math::vec2f_cref_t pos, motor::noise::gradient_noise_cref_t gn, material_property const & props )
            {
                auto p = ( (pos + props.offset) / props.divisor ) ;
                int_t v = int_t( props.amplitude * props.factor * gn.noise( p ) ) ;
                return v > 0 && pos.y() < -50.0f ;
            },

            // gold
            [=] ( motor::math::vec2f_cref_t pos, motor::noise::gradient_noise_cref_t gn, material_property const & props )
            {
                auto p = ( (pos + props.offset) / props.divisor ) ;
                int_t v = int_t( props.amplitude * props.factor * gn.noise( p ) ) ;
                return v > 0 && pos.y() < -100.0f ;
            },

            // diamant
            [=] ( motor::math::vec2f_cref_t pos, motor::noise::gradient_noise_cref_t gn, material_property const & props )
            {
                auto p = ( (pos + props.offset) / props.divisor ) ;
                int_t v = int_t( props.amplitude * props.factor * gn.noise( p ) ) ;
                return v > 0 && pos.y() < -200.0f ;
            }
        } ;
    }
}

namespace this_file
{
    using namespace motor::core::types ;

    class my_app : public motor::application::app
    {
        motor_this_typedefs( my_app ) ;

        motor::io::database db = motor::io::database( motor::io::path_t( DATAPATH ), "./working", "data" ) ;

        motor::gfx::pinhole_camera_t camera ;
        motor::gfx::primitive_render_2d_t pr ;
        motor::gfx::text_render_2d_t tr ;

        world::grid_t _grid = world::grid_t( 
            world::dimensions_t( 
                motor::math::vec2ui_t(1000), // regions_per_grid
                motor::math::vec2ui_t(16), // cells_per_region
                motor::math::vec2ui_t(16)  // pixels_per_cell
            ) 
        ) ;

        bool_t _draw_grid = true ;
        bool_t _draw_debug = false ;
        bool_t _view_preload_extend = false ;

        uint_t _extend_scale = 1 ;
        motor::math::vec2ui_t _extend ;
        motor::math::vec2ui_t _preload_extend ;
        world::dimensions::regions_and_cells_t rac_ ;

        motor::math::vec2f_t _cur_mouse ;

        float_t amplitude = 10.0f ;
        float_t frequency = 0.073f ;

        float_t noise_amplitude = 10.0f ;

        float_t fbm_lacunarity = 0.5f ;
        float_t fbm_h = 0.5f ;
        float_t fbm_octaves = 0.5f ;

        proto::material_property _cave_prop =
        {
            proto::material_form::solid,
            10.0f,
            10.0f,
            motor::math::vec2f_t( 18.0f, 18.0f ),
            motor::math::vec2f_t( 0.0f ),
            motor::math::vec4f_t( 185.0f / 255.0f, 169.0f / 255.0f, 180.0f / 255.0f, 1.0f )
        } ;

        proto::material_properties_t _material_properties = proto::get_default_properties() ;
        proto::material_funktions_t _material_funks = proto::get_default_material_funktions() ;

        motor::noise::gradient_noise_t _gn = motor::noise::gradient_noise_t( 376482, 8 ) ;
        motor::noise::fbm< motor::noise::gradient_noise_t > _fbm ;

        //***************************************************************************************************
        virtual void_t on_init( void_t ) noexcept
        {
            {
                motor::application::window_info_t wi ;
                wi.x = 100 ;
                wi.y = 100 ;
                wi.w = 1240 ;
                wi.h = 800 ;
                wi.gen = motor::application::graphics_generation::gen4_auto ;

                this_t::send_window_message( this_t::create_window( wi ), [&]( motor::application::app::window_view & wnd )
                {
                    wnd.send_message( motor::application::show_message( { true } ) ) ;
                    wnd.send_message( motor::application::cursor_message_t( {true} ) ) ;
                    wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                } ) ;
            }
            
            #if 0
            {
                motor::application::window_info_t wi ;
                wi.x = 500 ;
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
            #endif
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

            // init primitive renderer
            {
                pr.init( "my_prim_render" ) ;
            }

            // init camera
            {
                camera.look_at( motor::math::vec3f_t( 0.0f, 0.0f, -1000.0f ),
                    motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ), motor::math::vec3f_t( 0.0f, 0.0f, 0.0f )) ;
            }
        }

        //***************************************************************************************************
        motor::math::vec2ui_t extend( void_t ) const noexcept
        {
            return _extend * _extend_scale ;
        }

        //***************************************************************************************************
        motor::math::vec2ui_t preload_extend( void_t ) const noexcept
        {
            return _preload_extend ;
        }

        //***************************************************************************************************
        void_t change_extend( motor::math::vec2ui_cref_t ext, uint_t const ext_scale ) noexcept
        {
            _extend_scale = ext_scale ;
            _extend = ext ;
            
            auto const new_ext = this_t::extend() ;

            _preload_extend = new_ext + motor::math::vec2ui_t( _grid.get_dims().get_pixels_per_region() * 2 ) ;

            rac_ = world::dimensions::regions_and_cells_t() ;
            camera.set_dims( float_t( new_ext.x() ), float_t( new_ext.y() ), 1.0f, 1000.0f ) ;
            camera.orthographic() ;
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
            if( sv.resize_changed )
            {
                auto const new_ext = motor::math::vec2ui_t( uint_t( sv.resize_msg.w ), uint_t( sv.resize_msg.h ) ) ;
                this_t::change_extend( new_ext, _extend_scale ) ;
            }
        }

        //***************************************************************************************************
        virtual void_t on_device( device_data_in_t dd ) noexcept 
        {
            motor::controls::types::three_mouse mouse( dd.mouse ) ;
            _cur_mouse = mouse.get_local() * motor::math::vec2f_t( 2.0f ) - motor::math::vec2f_t( 1.0f ) ;
            _cur_mouse = _cur_mouse * (this_t::extend() * motor::math::vec2f_t(0.5f) );

            static motor::math::vec2f_t old = mouse.get_global() * motor::math::vec2f_t( 2.0f ) - motor::math::vec2f_t( 1.0f ) ;
            motor::math::vec2f_t const dif = (mouse.get_global()* motor::math::vec2f_t( 2.0f ) - motor::math::vec2f_t( 1.0f )) - old ;

            if( mouse.is_pressing(motor::controls::types::three_mouse::button::right ) )
            {
                auto old2 = camera.get_transformation() ;
                auto trafo = old2.translate_fl( motor::math::vec3f_t( -dif.x()*2000.0f, -dif.y()*2000.0f, 0.0f ) ) ;
                camera.set_transformation( trafo ) ;
            }

            old = mouse.get_global() * motor::math::vec2f_t( 2.0f ) - motor::math::vec2f_t( 1.0f ) ;
        }

        //***************************************************************************************************
        virtual void_t on_graphics( motor::application::app::graphics_data_in_t ) noexcept 
        {
            // grid rendering
            if( _draw_grid )
            {
                auto const cpos = camera.get_position().xy() ;

                // draw grid for extend
                {
                    world::dimensions::regions_and_cells_t rac = 
                    _grid.get_dims().calc_regions_and_cells( motor::math::vec2i_t( cpos ), 
                        this_t::extend() >> motor::math::vec2ui_t( 1 ) ) ;

                    // draw cells
                    this_t::draw_cells( rac ) ;

                    // draw regions
                    this_t::draw_regions( rac, 3 ) ;
                }

                // draw regions for preload extend
                {
                    world::dimensions::regions_and_cells_t rac = 
                    _grid.get_dims().calc_regions_and_cells( motor::math::vec2i_t( cpos ), 
                        motor::math::vec2ui_t( this_t::preload_extend() ) >> motor::math::vec2ui_t( 1 ) ) ;

                    // draw regions
                    this_t::draw_regions( rac, 2, motor::math::vec4f_t( 0.0f, 0.0f, 0.5f, 1.0f) ) ;
                }

                // draw current cells for mouse pos in grid
                {
                    auto const m = this_t:: _cur_mouse + cpos ;
                    auto const mouse_global = _grid.get_dims().calc_cell_ij_global( motor::math::vec2i_t( m ) ) ;
                    auto const ij = mouse_global ;
                    
                    auto const start = _grid.get_dims().transform_to_center( _grid.get_dims().cells_to_pixels( mouse_global ) )  ;
                    auto const cdims = _grid.get_dims().get_pixels_per_cell() ;

                    motor::math::vec2f_t p0 = start ;
                    motor::math::vec2f_t p1 = start + motor::math::vec2f_t(0.0f, float_t( cdims.y() )) ;
                    motor::math::vec2f_t p2 = start + cdims ;
                    motor::math::vec2f_t p3 = start + motor::math::vec2f_t( float_t(cdims.x()), 0.0f ) ;

                    pr.draw_rect( 4, p0, p1,p2,p3,
                        motor::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ),
                        motor::math::vec4f_t( 1.0f ) ) ;
                    
                    tr.draw_text( 1, 0, 13, motor::math::vec2f_t(p0), motor::math::vec4f_t(1.0f), 
                        "(i,j) : (" + motor::to_string( ij.x() ) + ", " + motor::to_string( ij.y() ) + ")" ) ;
                    
                }
            }

            // content section
            {
                auto const cpos = camera.get_position().xy() ;

                world::dimensions::regions_and_cells_t rac = 
                    _grid.get_dims().calc_regions_and_cells( motor::math::vec2i_t( cpos ), 
                        this_t::extend() >> motor::math::vec2ui_t( 1 ) ) ;
                
                this_t::draw_content( rac ) ;

                rac_ = rac ;
            }

            // draw preload extend 
            if( _draw_debug )
            {
                auto const cpos = camera.get_position().xy() ;

                motor::math::vec2f_t p0 = cpos + this_t::preload_extend() * motor::math::vec2f_t(-0.5f,-0.5f) ;
                motor::math::vec2f_t p1 = cpos + this_t::preload_extend() * motor::math::vec2f_t(-0.5f,+0.5f) ;
                motor::math::vec2f_t p2 = cpos + this_t::preload_extend() * motor::math::vec2f_t(+0.5f,+0.5f) ;
                motor::math::vec2f_t p3 = cpos + this_t::preload_extend() * motor::math::vec2f_t(+0.5f,-0.5f) ;

                motor::math::vec4f_t color0( 1.0f, 1.0f, 1.0f, 0.0f ) ;
                motor::math::vec4f_t color1( 0.0f, 0.0f, 1.0f, 1.0f ) ;

                pr.draw_rect( 50, p0, p1, p2, p3, color0, color1 ) ;
            }

            {
                pr.set_view_proj( camera.mat_view(), camera.mat_proj() ) ;
                pr.prepare_for_rendering() ;

                tr.set_view_proj( camera.mat_view(), camera.mat_proj() ) ;
                tr.prepare_for_rendering() ;
            }
        } 

        //***************************************************************************************************
        virtual void_t on_render( this_t::window_id_t const wid, motor::graphics::gen4::frontend_ptr_t fe, 
            motor::application::app::render_data_in_t rd ) noexcept 
        {
            if( rd.first_frame )
            {
                pr.configure( fe ) ;
                tr.configure( fe ) ;
            }

            {
                pr.prepare_for_rendering( fe ) ;
                tr.prepare_for_rendering( fe ) ;
                for( size_t i=0; i<100; ++i )
                {
                    pr.render( fe, i ) ;
                    tr.render( fe, i ) ;
                }
            }
        }

        //***************************************************************************************************
        void_t draw_cells( world::dimensions::regions_and_cells_cref_t rac ) noexcept
        {
            MOTOR_PROBE( "application", "draw_cells" ) ;

            auto const num_cells = rac.cell_dif() ;
            auto const pixels_min = _grid.get_dims().cells_to_pixels( rac.cell_min() ) ;
            auto const view_pixels = _grid.get_dims().cells_to_pixels( num_cells ) ;

            // x lines / vertical lines
            {
                auto const start = _grid.get_dims().transform_to_center( pixels_min ) ;
                motor::math::vec2f_t p0( start ) ;
                motor::math::vec2f_t p1( start + motor::math::vec2i_t( 0, view_pixels.y() ) ) ;

                #if DRAW_SINGLE
                for( uint_t i = 0 ; i < num_cells.x() +1 ; ++i )
                {
                    pr.draw_line( 0, p0, p1, motor::math::vec4f_t(0.6f) ) ;
                    p0 = p0 + motor::math::vec2f_t( float_t( _grid.get_dims().get_pixels_per_cell().x()), 0.0f ) ;
                    p1 = p1 + motor::math::vec2f_t( float_t(_grid.get_dims().get_pixels_per_cell().x()), 0.0f ) ;
                }
                #else
                pr.draw_lines( 2, num_cells.x() + 1, [&] ( size_t const i ) 
                {
                    motor::gfx::line_render_2d::line_t const ln { { p0, p1 }, motor::math::vec4f_t( 0.6f ) } ;

                    p0 = p0 + motor::math::vec2f_t( float_t( _grid.get_dims().get_pixels_per_cell().x() ), 0.0f ) ;
                    p1 = p1 + motor::math::vec2f_t( float_t( _grid.get_dims().get_pixels_per_cell().x() ), 0.0f ) ;

                    return std::move( ln ) ;
                } ) ;
                #endif
            }

            // y lines / horizontal lines
            {
                auto const start = _grid.get_dims().transform_to_center( pixels_min ) ;
                motor::math::vec2f_t p0( start ) ;
                motor::math::vec2f_t p1( start + motor::math::vec2i_t( view_pixels.x(), 0  ) ) ;

                #if DRAW_SINGLE
                for( uint_t i = 0 ; i < num_cells.y() + 1 ; ++i )
                {
                    pr.draw_line( 0, p0, p1, motor::math::vec4f_t(0.6f) ) ;
                    p0 = p0 + motor::math::vec2f_t( 0.0f, float_t( _grid.get_dims().get_pixels_per_cell().y() ) ) ;
                    p1 = p1 + motor::math::vec2f_t( 0.0f, float_t( _grid.get_dims().get_pixels_per_cell().y() ) ) ;
                }
                #else
                pr.draw_lines( 2, num_cells.y() + 1, [&] ( size_t const i )
                {
                    motor::gfx::line_render_2d::line_t const ln { { p0, p1 }, motor::math::vec4f_t( 0.6f ) } ;

                    p0 = p0 + motor::math::vec2f_t( 0.0f, float_t( _grid.get_dims().get_pixels_per_cell().y() ) ) ;
                    p1 = p1 + motor::math::vec2f_t( 0.0f, float_t( _grid.get_dims().get_pixels_per_cell().y() ) ) ;

                    return std::move( ln ) ;
                } ) ;
                #endif
            }
        }

        //***************************************************************************************************
        void_t draw_regions( world::dimensions::regions_and_cells_cref_t rac, 
            size_t l, motor::math::vec4f_cref_t border_color = motor::math::vec4f_t(1.0f) ) noexcept
        {
            auto const num_region = rac.region_dif() ;
            auto const pixels_min = _grid.get_dims().regions_to_pixels( rac.region_min() ) ;
            auto const view_pixels = _grid.get_dims().regions_to_pixels( num_region ) ;

            // x lines / vertical lines
            {
                auto const start = _grid.get_dims().transform_to_center( pixels_min ) ;
                motor::math::vec2f_t p0( start ) ;
                motor::math::vec2f_t p1( start + motor::math::vec2i_t( 0, view_pixels.y() ) ) ;

                pr.draw_lines( l, num_region.x() + 1, [&] ( size_t const i )
                {
                    motor::gfx::line_render_2d::line_t const ln { { p0, p1 }, border_color } ;

                    p0 = p0 + motor::math::vec2f_t( float_t( _grid.get_dims().get_pixels_per_region().x() ), 0.0f ) ;
                    p1 = p1 + motor::math::vec2f_t( float_t( _grid.get_dims().get_pixels_per_region().x() ), 0.0f ) ;

                    return std::move( ln ) ;
                } ) ;
            }

            // y lines / horizontal lines
            {
                auto const start = _grid.get_dims().transform_to_center( pixels_min ) ;
                motor::math::vec2f_t p0( start ) ;
                motor::math::vec2f_t p1( start + motor::math::vec2i_t( view_pixels.x(), 0  ) ) ;
                
                pr.draw_lines( l, num_region.y() + 1, [&] ( size_t const i )
                {
                    motor::gfx::line_render_2d::line_t const ln { { p0, p1 }, border_color } ;

                    p0 = p0 + motor::math::vec2f_t( 0.0f, float_t( _grid.get_dims().get_pixels_per_region().y() ) ) ;
                    p1 = p1 + motor::math::vec2f_t( 0.0f, float_t( _grid.get_dims().get_pixels_per_region().y() ) ) ;

                    return std::move( ln ) ;
                } ) ;
            }
        }

        //***************************************************************************************************
        void_t draw_content( world::dimensions::regions_and_cells_cref_t rac ) noexcept
        {
            MOTOR_PROBE( "application", "draw_content" ) ;

            #if DRAW_SINGLE
            auto const cells_start = rac.cell_min() ;
            for( auto y = rac.ocell_min().y(); y < rac.ocell_max().y(); ++y )
            {
                for( auto x = rac.ocell_min().x(); x < rac.ocell_max().x(); ++x )
                {
                    auto const cur_pos = motor::math::vec2i_t( x, y ) ;
                    motor::math::vec2f_t const p0 = _grid.get_dims().cells_to_pixels( cur_pos ) ;
                    motor::math::vec2f_t const d = motor::math::vec2f_t( _grid.get_dims().get_pixels_per_cell() ) ;
                    motor::math::vec2f_t const dh = motor::math::vec2f_t( _grid.get_dims().get_pixels_per_cell() >> uint_t(1) ) ;

                    {
                        auto const p1 = p0 + motor::math::vec2f_t( 0.0f, d.y() ) ;
                        auto const p2 = p0 + motor::math::vec2f_t( d.x(), d.y() ) ;
                        auto const p3 = p0 + motor::math::vec2f_t( d.x(), 0.0f ) ;

                        int_t f = int_t( 10.0f * std::sin( x * 100.0f ) - y ) ;
                        
                        if( f > 0 )
                        {
                            pr.draw_rect( 0, p0, p1, p2, p3, motor::math::vec4f_t(0.0f,0.0f,1.0f,1.0f), motor::math::vec4f_t(0.6f) ) ;
                        }                        
                        else
                        {
                            pr.draw_rect( 0, p0, p1, p2, p3, motor::math::vec4f_t(0.6f,0.1f,0.3f,1.0f), motor::math::vec4f_t(0.6f) ) ;
                        }
                    }
                }
            }
            #else
            {
                motor::math::vec2ui_t const cell_dif = rac.cell_dif() ;
                
                size_t const num_cells = cell_dif.x() * cell_dif.y() ;

                auto const cells_start = rac.cell_min() ;
                pr.draw_rects( 0, num_cells, [&] ( size_t const i ) 
                {
                    uint_t const x_ = i % cell_dif.x() ;
                    uint_t const y_ = i / cell_dif.x() % cell_dif.y() ;

                    auto const abs_pos = motor::math::vec2ui_t( cells_start.x() + x_, cells_start.y() + y_ ) ;

                    auto const cur_pos = _grid.get_dims().transform_cell_to_center( abs_pos ) ;
                    motor::math::vec2f_t const p0 = _grid.get_dims().cells_to_pixels( cur_pos ) ;
                    motor::math::vec2f_t const d = motor::math::vec2f_t( _grid.get_dims().get_pixels_per_cell() ) ;
                    motor::math::vec2f_t const dh = motor::math::vec2f_t( _grid.get_dims().get_pixels_per_cell() >> uint_t( 1 ) ) ;

                    auto const p1 = p0 + motor::math::vec2f_t( 0.0f, d.y() ) ;
                    auto const p2 = p0 + motor::math::vec2f_t( d.x(), d.y() ) ;
                    auto const p3 = p0 + motor::math::vec2f_t( d.x(), 0.0f ) ;

                    motor::math::vec4f_t air ( 0.0f, 0.3f, 1.0f, 1.0f  ) ;
                    motor::math::vec4f_t soil ( 0.4f, 0.0f, 0.0f, 1.0f ) ;
                    motor::math::vec4f_t cave ( 0.0f, 0.3f, 1.0f, 1.0f ) ;

                    motor::math::vec4f_t gras ( 0.0f, 1.0f, 0.0f, 1.0f ) ;
                    motor::math::vec4f_t gold ( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    motor::math::vec4f_t coal ( 0.0f, 0.0f, 0.0f, 1.0f ) ;
                    motor::math::vec4f_t iron ( 185.0f/255.0f, 169.0f / 255.0f, 180.0f / 255.0f, 1.0f ) ;
                    motor::math::vec4f_t dias ( 0.0f, 0.7f, 1.0f, 1.0f ) ;

                    motor::math::vec4f_t color = air ;

                    // evaluate implicit function
                    {

                        auto const x = cur_pos.x() ;
                        auto const y = cur_pos.y() ;

                        float_t amp_scale_0 = 10.0f * _gn.noise( motor::math::vec2f_t( x / 50.0f, y / 100.0f ) ) ;
                        float_t amp_scale_1 = 10.0f * _gn.noise( motor::math::vec2f_t( x / 100.0f, y / 200.0f ) ) ;
                        float_t amp = (amplitude * amp_scale_0)+ amp_scale_0*amp_scale_1 ;

                        
                        int_t f = int_t( y ) ;

                        f += int_t( amp * std::sin( x * frequency ) ) ;
                        f += int_t( noise_amplitude * _gn.noise( motor::math::vec2f_t( x / 10.0f, y / 10.0f ) ) ) ;
                        
                        int_t f_cave = f ;

                        // cave function
                        {
                            motor::math::vec2f_t const p = ( motor::math::vec2f_t( cur_pos ) + _cave_prop.offset ) / _cave_prop.divisor ;
                            f_cave = int_t( _cave_prop.amplitude * _gn.noise( p ) ) ;

                            //f = std::max( f, f_cave ) ;
                            //f = f_cave ;
                        }

                        // over solid material surface
                        // aka. in gas space
                        

                        #if 1
                        // on solid material surface
                        if ( f <= 1 && f >= 0 )
                        {
                            color = gras ;
                        }

                        // under solid material surface
                        else if ( f < 0 )
                        {
                            color = soil ;

                            if ( f_cave > 0 && y < -20.0f )
                            {
                                color = cave ;
                            }
                            else 
                            {
                                size_t const b = size_t( proto::material::coal ) ;
                                size_t const e = size_t( proto::material::num_materials ) ;

                                for ( size_t m = b; m < e; ++m )
                                {
                                    if ( _material_funks[ m ]( cur_pos, _gn, _material_properties[m] ) )
                                    {
                                        color = _material_properties[ m ].color ;
                                    }
                                }
                            }
                        }
                        #endif
                    }

                    return motor::gfx::primitive_render_2d::rect_t { { p0, p1, p2, p3 }, color } ;
                } ) ;

            }

            #endif

        }

        void_t draw_props_imgui( char const * name, proto::material_property & props )
        {
            {
                float_t v = props.amplitude ;
                ImGui::SliderFloat( "Amplitude", &v, 1.0f, 15.0f ) ;
                props.amplitude = v ;
            }

            {
                float_t v = props.factor ;
                ImGui::SliderFloat( "Factor", &v, 0.1f, 0.4f ) ;
                props.factor = v ;
            }
            {
                float_t v[ 2 ] = { props.divisor.x(), props.divisor.y() } ;
                ImGui::SliderFloat2( "Divisor", v, 3.0f, 50.0f ) ;
                props.divisor = motor::math::vec2f_t( v[0], v[1] ) ;
            }

            {
                float_t v[ 2 ] = { props.offset.x(), props.offset.y() } ;
                ImGui::SliderFloat2( "Offset", v, -100.0f, 100.0f ) ;
                props.offset = motor::math::vec2f_t( v[ 0 ], v[ 1 ] ) ;
            }
        }

        void_t draw_props_imgui_cave( char const * name, proto::material_property & props )
        {
            {
                float_t v = props.amplitude ;
                ImGui::SliderFloat( "Amplitude##cave", &v, 1.0f, 50.0f ) ;
                props.amplitude = v ;
            }

            {
                float_t v = props.factor ;
                ImGui::SliderFloat( "Factor##cave", &v, 0.1f, 10.4f ) ;
                props.factor = v ;
            }
            {
                float_t v[ 2 ] = { props.divisor.x(), props.divisor.y() } ;
                ImGui::SliderFloat2( "Divisor##cave", v, 3.0f, 50.0f ) ;
                props.divisor = motor::math::vec2f_t( v[ 0 ], v[ 1 ] ) ;
            }

            {
                float_t v[ 2 ] = { props.offset.x(), props.offset.y() } ;
                ImGui::SliderFloat2( "Offset##cave", v, -100.0f, 100.0f ) ;
                props.offset = motor::math::vec2f_t( v[ 0 ], v[ 1 ] ) ;
            }
        }

        virtual bool_t on_tool( this_t::window_id_t const wid, motor::application::app::tool_data_ref_t ) noexcept 
        { 
            if( wid != 0 ) return false ;

            if ( ImGui::Begin( "Materials" ) )
            {
                static int item_current_idx = 0;
                auto * items = proto::internal::__material_names ;
                const char * combo_preview_value = items[ item_current_idx] ;

                if ( ImGui::BeginCombo( "material_combo_box", combo_preview_value, 0 ) )
                {
                    for ( int n = 0; n < int(proto::material::num_materials); n++ )
                    {
                        const bool is_selected = ( item_current_idx == n );
                        if ( ImGui::Selectable( items[ n ], is_selected ) )
                            item_current_idx = n;

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if ( is_selected )
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                this_t::draw_props_imgui( items[ item_current_idx ], _material_properties[ item_current_idx ] ) ;

                ImGui::Text( "cave" );
                this_t::draw_props_imgui_cave( "cave", _cave_prop ) ;
            }
            ImGui::End() ;


            ImGui::Begin( "Control and Info" ) ;

            ImGui::Text( "Control" ) ;

            {
                int_t v = _extend_scale ;
                ImGui::SliderInt( "Extend Scale", &v, 1, 3 ) ;
                this_t::change_extend( _extend, v ) ;
            }

            {
                ImGui::Checkbox( "Draw Grid", &_draw_grid ) ;
            }

            {
                ImGui::Checkbox( "Draw Debug", &_draw_debug ) ;
            }

            {
                if( ImGui::Checkbox( "Draw From Preload Extend", &_view_preload_extend) )
                {
                    if( _view_preload_extend )
                    {
                        camera.set_dims( float_t( this_t::preload_extend().x()), float_t( this_t::preload_extend().y()), 1.0f, 1000.0f ) ;
                    }
                    else
                    {
                        camera.set_dims( float_t(_extend.x()), float_t(_extend.y()), 1.0f, 1000.0f ) ;
                    }

                    camera.orthographic() ;
                }
            }

            ImGui::Text( "noise" ) ;
            {
                ImGui::SliderFloat( "Amplitude", &amplitude, 1.0f, 15.0f ) ;
                ImGui::SliderFloat( "Frequency", &frequency, 0.001f, 0.1f ) ;
            }

            {
                ImGui::SliderFloat( "Noise Amplitude", &noise_amplitude, 1.0f, 15.0f ) ;
            }
            {
                ImGui::SliderFloat( "Lacunarity", &fbm_lacunarity, 0.01f, 2.0f ) ;
                ImGui::SliderFloat( "H", &fbm_h, 0.01f, 2.0f ) ;
                ImGui::SliderFloat( "Octaves", &fbm_octaves, 1.0f, 10.0f ) ;
            }

            if( _view_preload_extend )
            {
                ImGui::Text( "Preload" ) ;

                static float_t mult = 1.0f ;
                if( ImGui::SliderFloat( "Preload extend mult", &mult, 1.0f, 3.0f ) )
                {
                    auto const v = mult * _preload_extend ;
                    camera.set_dims( float_t(v.x()), float_t(v.y()), 1.0f, 1000.0f ) ;
                    camera.orthographic() ;
                }
            }

            ImGui::End() ;

            return true ; 
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
    motor::profiling::global::deinit() ;
    motor::memory::global::dump_to_std() ;

    return ret ;
}
