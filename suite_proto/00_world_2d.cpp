
#include "00_world_2d/world.h"

#include <motor/platform/global.h>

#include <motor/gfx/camera/pinhole_camera.h>
#include <motor/gfx/font/text_render_2d.h>
#include <motor/gfx/primitive/primitive_render_2d.h>

#include <motor/format/global.h>
#include <motor/format/future_items.hpp>
#include <motor/property/property_sheet.hpp>

#include <motor/math/utility/angle.hpp>

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

        motor::gfx::pinhole_camera_t camera ;
        motor::gfx::primitive_render_2d_t pr ;
        motor::gfx::text_render_2d_t tr ;

        world::grid_t _grid = world::grid_t( 
            world::dimensions_t( 
                motor::math::vec2ui_t(1000), // regions_per_grid
                motor::math::vec2ui_t(16), // cells_per_region
                motor::math::vec2ui_t(8)  // pixels_per_cell
            ) 
        ) ;

        bool_t _draw_grid = true ;
        bool_t _draw_debug = false ;
        bool_t _view_preload_extend = false ;

        motor::math::vec2ui_t _extend ;
        motor::math::vec2ui_t _preload_extend ;
        world::dimensions::regions_and_cells_t rac_ ;

        motor::math::vec2f_t _cur_mouse ;

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
                    wnd.send_message( motor::application::cursor_message_t( {true} ) ) ;
                    wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                } ) ;
            }

            {
                motor::application::window_info_t wi ;
                wi.x = 100 ;
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
                _extend = motor::math::vec2ui_t( uint_t(sv.resize_msg.w), uint_t( sv.resize_msg.h ) ) ;
                _preload_extend = _extend + motor::math::vec2ui_t( _grid.get_dims().get_pixels_per_region() * 2 ) ;

                
                rac_ = world::dimensions::regions_and_cells_t() ;
                camera.set_dims( float_t(_extend.x()), float_t(_extend.y()), 1.0f, 1000.0f ) ;
                camera.orthographic() ;
            }
        }

        //***************************************************************************************************
        virtual void_t on_device( device_data_in_t dd ) noexcept 
        {
            motor::device::layouts::three_mouse mouse( dd.mouse ) ;
            _cur_mouse = mouse.get_local() * motor::math::vec2f_t( 2.0f ) - motor::math::vec2f_t( 1.0f ) ;
            _cur_mouse = _cur_mouse * (_extend * motor::math::vec2f_t(0.5f) );

            static motor::math::vec2f_t old = mouse.get_global() * motor::math::vec2f_t( 2.0f ) - motor::math::vec2f_t( 1.0f ) ;
            motor::math::vec2f_t const dif = (mouse.get_global()* motor::math::vec2f_t( 2.0f ) - motor::math::vec2f_t( 1.0f )) - old ;

            if( mouse.is_pressing(motor::device::layouts::three_mouse::button::right ) )
            {
                auto old2 = camera.get_transformation() ;
                auto trafo = old2.translate_fl( motor::math::vec3f_t( -dif.x()*200.0f, -dif.y()*200.0f, 0.0f ) ) ;
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
                        motor::math::vec2ui_t( _extend ) >> motor::math::vec2ui_t( 1 ) ) ;

                    // draw cells
                    this_t::draw_cells( rac ) ;

                    // draw regions
                    this_t::draw_regions( rac, 3 ) ;
                }

                // draw regions for preload extend
                {
                    world::dimensions::regions_and_cells_t rac = 
                    _grid.get_dims().calc_regions_and_cells( motor::math::vec2i_t( cpos ), 
                        motor::math::vec2ui_t( _preload_extend ) >> motor::math::vec2ui_t( 1 ) ) ;

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
                        motor::math::vec2ui_t( _extend ) >> motor::math::vec2ui_t( 1 ) ) ;
                
                this_t::draw_content( rac ) ;

                rac_ = rac ;
            }

            // draw preload extend 
            if( _draw_debug )
            {
                auto const cpos = camera.get_position().xy() ;

                motor::math::vec2f_t p0 = cpos + _preload_extend * motor::math::vec2f_t(-0.5f,-0.5f) ;
                motor::math::vec2f_t p1 = cpos + _preload_extend * motor::math::vec2f_t(-0.5f,+0.5f) ;
                motor::math::vec2f_t p2 = cpos + _preload_extend * motor::math::vec2f_t(+0.5f,+0.5f) ;
                motor::math::vec2f_t p3 = cpos + _preload_extend * motor::math::vec2f_t(+0.5f,-0.5f) ;

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
            auto const num_cells = rac.cell_dif() ;
            auto const pixels_min = _grid.get_dims().cells_to_pixels( rac.cell_min() ) ;
            auto const view_pixels = _grid.get_dims().cells_to_pixels( num_cells ) ;

            // x lines / vertical lines
            {
                auto const start = _grid.get_dims().transform_to_center( pixels_min ) ;
                motor::math::vec2f_t p0( start ) ;
                motor::math::vec2f_t p1( start + motor::math::vec2i_t( 0, view_pixels.y() ) ) ;

                for( uint_t i = 0 ; i < num_cells.x() +1 ; ++i )
                {
                    pr.draw_line( 0, p0, p1, motor::math::vec4f_t(0.6f) ) ;
                    p0 = p0 + motor::math::vec2f_t( float_t( _grid.get_dims().get_pixels_per_cell().x()), 0.0f ) ;
                    p1 = p1 + motor::math::vec2f_t( float_t(_grid.get_dims().get_pixels_per_cell().x()), 0.0f ) ;
                }
            }

            // y lines / horizontal lines
            {
                auto const start = _grid.get_dims().transform_to_center( pixels_min ) ;
                motor::math::vec2f_t p0( start ) ;
                motor::math::vec2f_t p1( start + motor::math::vec2i_t( view_pixels.x(), 0  ) ) ;
                        
                for( uint_t i = 0 ; i < num_cells.y() + 1 ; ++i )
                {
                    pr.draw_line( 0, p0, p1, motor::math::vec4f_t(0.6f) ) ;
                    p0 = p0 + motor::math::vec2f_t( 0.0f, float_t( _grid.get_dims().get_pixels_per_cell().y() ) ) ;
                    p1 = p1 + motor::math::vec2f_t( 0.0f, float_t( _grid.get_dims().get_pixels_per_cell().y() ) ) ;
                }
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

                for( uint_t i = 0 ; i < num_region.x() + 1; ++i )
                {
                    pr.draw_line( l, p0, p1, border_color ) ;
                    p0 = p0 + motor::math::vec2f_t( float_t( _grid.get_dims().get_pixels_per_region().x() ), 0.0f ) ;
                    p1 = p1 + motor::math::vec2f_t( float_t( _grid.get_dims().get_pixels_per_region().x() ), 0.0f ) ;
                }
            }

            // y lines / horizontal lines
            {
                auto const start = _grid.get_dims().transform_to_center( pixels_min ) ;
                motor::math::vec2f_t p0( start ) ;
                motor::math::vec2f_t p1( start + motor::math::vec2i_t( view_pixels.x(), 0  ) ) ;
                        
                for( uint_t i = 0 ; i < num_region.y() + 1; ++i )
                {
                    pr.draw_line( l, p0, p1, border_color ) ;
                    p0 = p0 + motor::math::vec2f_t( 0.0f, float_t( _grid.get_dims().get_pixels_per_region().y() ) ) ;
                    p1 = p1 + motor::math::vec2f_t( 0.0f, float_t( _grid.get_dims().get_pixels_per_region().y() ) ) ;
                }
            }
        }

        //***************************************************************************************************
        void_t draw_content( world::dimensions::regions_and_cells_cref_t rac ) noexcept
        {
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
        }

        virtual bool_t on_tool( this_t::window_id_t const wid, motor::application::app::tool_data_ref_t ) noexcept 
        { 
            if( wid != 0 ) return false ;

            ImGui::Begin( "Control and Info" ) ;

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
                        camera.set_dims( float_t(_preload_extend.x()), float_t(_preload_extend.y()), 1.0f, 1000.0f ) ;
                    }
                    else
                    {
                        camera.set_dims( float_t(_extend.x()), float_t(_extend.y()), 1.0f, 1000.0f ) ;
                    }

                    camera.orthographic() ;
                }
            }

            if( _view_preload_extend )
            {
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
    motor::memory::global::dump_to_std() ;


    return ret ;
}
