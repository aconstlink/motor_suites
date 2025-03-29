
#include <motor/platform/global.h>

#include <motor/gfx/primitive/primitive_render_2d.h>
#include <motor/gfx/font/text_render_2d.h>

#include <motor/format/global.h>
#include <motor/format/future_items.hpp>
#include <motor/property/property_sheet.hpp>

#include <motor/geometry/simple_polygon.hpp>
#include <motor/geometry/2d/convex_hull_2d.hpp>

#include <motor/math/camera/3d/orthographic_projection.hpp>

#include <motor/std/vector_pod>
#include <motor/std/insertion_sort.hpp>

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
        
        motor::gfx::primitive_render_2d_t pr ;
        motor::gfx::text_render_2d_t tr ;

    private: // window stuff

        size_t _wid_tool = size_t(-1) ;
        motor::math::vec2f_t _dims = motor::math::vec2f_t( 800.0f ) ;

        bool_t _mouse_down = false ;
        bool_t _lock_point_mouse = false ;
        motor::math::vec2f_t _cur_mouse ;
        size_t _cur_sel_point = size_t(-1) ;

    private: // mesh

        motor_typedefs( motor::geometry::simple_polygon< motor::math::vec2f_t >, polygon ) ;
        motor_typedefs( motor::geometry::convex_hull_2d< float_t >, convex_hull ) ;

        motor::vector< motor::math::vec2f_t > _points ;
        polygon_t _polygon ;
        convex_hull_t _convex_hull ;

        void_t add_point( motor::math::vec2f_cref_t p ) noexcept
        {
            _points.emplace_back( p ) ;
            _polygon = polygon_t( _points ) ;
        }

        void_t remove_last_point( void_t ) noexcept
        {
            if( _points.size() <= 1  )
            {
                _points.clear() ;
                return ;
            }
            _points.resize( _points.size() - 1 ) ;
            _polygon = polygon_t( _points ) ;
        }

    private: // ui stuff

        float_t _point_radius = 1.0f ;
        
        bool_t _left_is_released = false ;
        bool_t _right_is_released = false ;

        bool_t _polygon_is_complete = false ;
        bool_t _mouse_is_near_start = false ;

        bool_t _draw_outline = true ;
        bool_t _draw_tris = true ;
        bool_t _draw_convex_hull = false ;
        
    private:

        //****************************************************************************************
        virtual void_t on_init( void_t ) noexcept
        {
            motor::application::carrier::set_cpu_sleep( std::chrono::microseconds(50) ) ;

            {
                _wid_tool = this_t::create_window( motor::application::window_info_t::create( 100, 100, 400, 400, 
                    motor::application::graphics_generation::gen4_auto ) ) ;
            }

            {
                this_t::create_window( motor::application::window_info_t::create( 500, 100, int_t(_dims.x()), int_t(_dims.y()), 
                    motor::application::graphics_generation::gen4_gl4 ) ) ;
            }

            // import fonts and create text render
            {
                motor::io::database db = motor::io::database( motor::io::path_t( DATAPATH ), "./working", "data" ) ;
                motor::property::property_sheet_mtr_t ps = motor::shared( motor::property::property_sheet_t() ) ;

                {
                    motor::font::code_points_t pts ;
                    for( uint32_t i = 33; i <= 126; ++i ) pts.emplace_back( i ) ;
                    for( uint32_t i : {uint32_t( 0x00003041 )} ) pts.emplace_back( i ) ;
                    ps->set_value< motor::font::code_points_t >( "code_points", pts ) ;
                }

                #if 0
                {
                    motor::vector< motor::io::location_t > locations = 
                    {
                        motor::io::location_t("fonts.LCD_Solid.ttf"),
                        //motor::io::location_t("")
                    } ;
                    ps->set_value( "additional_locations", locations ) ;
                }
                #endif

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

            pr.init( "my_prim_render" ) ;
        }

        //***************************************************************************************************
        virtual void_t on_device( device_data_in_t dd ) noexcept 
        {
            motor::controls::types::three_mouse mouse( dd.mouse ) ;
            _cur_mouse = mouse.get_local() * motor::math::vec2f_t( 2.0f ) - motor::math::vec2f_t( 1.0f ) ;
            _cur_mouse = _cur_mouse * _dims * motor::math::vec2f_t(0.5f) ;
            //std::printf( "%f;%f\n", _cur_mouse.x(), _cur_mouse.y() ) ;

            bool_t const pressed = mouse.is_pressed( motor::controls::types::three_mouse::button::left ) ;
            bool_t const released = mouse.is_released( motor::controls::types::three_mouse::button::left ) ;

            if( _cur_sel_point != size_t(-1) && pressed )
            {
                _lock_point_mouse = true ;
            }
            else if( released )
                _lock_point_mouse = false ;

            _left_is_released = released ;


            _right_is_released = mouse.is_released( motor::controls::types::three_mouse::button::right ) ;
        }

        //****************************************************************************************
        virtual void_t on_event( window_id_t const wid, 
                motor::application::window_message_listener::state_vector_cref_t sv ) noexcept
        {
            // if any window closes, close the app
            if( sv.close_changed ) this->close() ;
            if( sv.resize_changed && wid != _wid_tool )
            {
                _dims = motor::math::vec2f_t( 
                    float_t( sv.resize_msg.w ), 
                    float_t( sv.resize_msg.h ) ) ; 
            }
        }

        //****************************************************************************************
        virtual void_t on_update( motor::application::app::update_data_in_t ud ) noexcept 
        {
            if( _points.size() > 0 )
            {
                auto const rad = (_point_radius * 5.0f) ;
                _mouse_is_near_start = ( _points[0] - _cur_mouse ).length2() < rad*rad ;
            }

            if( _left_is_released && !_polygon_is_complete )
            {
                _polygon_is_complete = _mouse_is_near_start ;

                if( !_polygon_is_complete )
                {
                    this_t::add_point( motor::math::vec2f_t( _cur_mouse ) ) ;
                    _left_is_released = false ;
                }
            }

            if( _right_is_released )
            {
                this_t::remove_last_point() ;
                _right_is_released = false ;
                _polygon_is_complete = false ;
            }

            //_mesh.triangulate() ;
        }

        //****************************************************************************************
        virtual void_t on_graphics( motor::application::app::graphics_data_in_t ) noexcept 
        {
            static float_t inc = 0.0f ;

            // draw points
            if( _points.size() > 1 )
            {
                motor::math::vec4f_t const col_a(1.0f, 1.0f, 1.0f, 1.0f ) ;
                motor::math::vec4f_t const col_b(1.0f, 0.0f, 1.0f, 1.0f ) ;
                pr.draw_circles( 1, 10, _points.size(), [&]( size_t const i )
                {
                    float_t radius = _point_radius ;
                    motor::math::vec4f_t color( 0.0f, 0.0f, 0.0f, 1.0f ) ;
                    return motor::gfx::primitive_render_2d::circle_t { _points[i], radius, color, col_b } ;
                } ) ;
            }

            if( _points.size() > 0 && !_polygon_is_complete )
            {
                auto circ_col = motor::math::vec4f_t(1.0f, 0.0f, 0.0f, 1.0f) ;
                if( _mouse_is_near_start ) circ_col = motor::math::vec4f_t(0.0f, 1.0f, 0.0f, 1.0f) ;
                pr.draw_circle( 0, 10, _points[0], _point_radius * 5.0f, motor::math::vec4f_t(0.0f), circ_col );
            }

            // draw cur mouse as point
            if( !_polygon_is_complete )
            {
                motor::math::vec4f_t const col_a(1.0f, 1.0f, 1.0f, 1.0f ) ;
                motor::math::vec4f_t const col_b(1.0f, 0.0f, 1.0f, 1.0f ) ;
                pr.draw_circle( 1, 10, _cur_mouse, _point_radius*2.0f, col_a, col_b ) ;
            }

            // draw mouse segment
            if( _points.size() > 0 && !_polygon_is_complete )
            {
                motor::math::vec4f_t const col_a(1.0f, 0.0f, 0.0f, 1.0f ) ;
                pr.draw_line( 0, _points.back(), _cur_mouse, col_a ) ;
            }
            
            {
                auto const polygon = _polygon.triangulate() ;
                _convex_hull.construct( _points ) ;

                // draw outline
                if( _draw_outline)
                {
                    auto const & edges = polygon.edges() ;
                    auto const & points = polygon.points() ;

                    motor::math::vec4f_t const col( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    pr.draw_lines( 2, edges.size(), [&] ( size_t const i )
                    {
                        auto const [a, b] = edges[ i ] ;
                        return motor::gfx::line_render_2d::line_t { { points[ a ], points[ b ] }, col } ;
                    } ) ;
                }
                
                if( _draw_tris )
                {
                    motor::math::vec4f_t const col( 0.0f, 1.0f, 1.0f, 1.0f ) ;

                    auto const & points = polygon.points() ;
                    auto const & edges = polygon.edges() ;

                    pr.draw_lines( 1, edges.size(), [&]( size_t const i )
                    {
                        auto const [a, b] = edges[ i ] ;
                        return motor::gfx::line_render_2d::line_t { { points[ a ], points[ b ] }, col } ;
                    } ) ;
                }

                // draw triangles
                {
                    auto const & points = polygon.points() ;
                    auto const & edges = polygon.edges() ;

                    pr.draw_tris( 0, edges.size() / 3, [&] ( size_t const i )
                    {
                        auto const p0 = points[ edges[ i * 3 + 0 ].a ] ;
                        auto const p1 = points[ edges[ i * 3 + 1 ].a ] ;
                        auto const p2 = points[ edges[ i * 3 + 2 ].a ] ;

                        return motor::gfx::tri_render_2d::tri_md { { p0, p1, p2 },
                            motor::math::vec4f_t( 1.0f, 1.0f, 1.0f, 0.5f ) } ;
                    } ) ;
                }

                // draw convex hull
                if( _draw_convex_hull )
                {
                    motor::math::vec4f_t const col( 1.0f, 1.0f, 1.0f, 1.0f ) ;

                    auto const & points = _convex_hull.get_points() ;
                    pr.draw_lines( 1, points.size(), [&]( size_t const i )
                    {
                        size_t const a = i ;
                        size_t const b = (i+1) % points.size() ;
                        return motor::gfx::line_render_2d::line_t { { points[ a ], points[ b ] }, col } ;
                    } ) ;
                }

                {
                    //_mesh.dual_graph( edges, pr, tr, _dims*0.5f ) ;
                }
            }

            inc += 0.01f  ;
            inc = motor::math::fn<float_t>::mod( inc, 1.0f ) ;

            {
                motor::math::mat4f_t view = motor::math::mat4f_t::make_identity() ;
                motor::math::mat4f_t proj = motor::math::m3d::orthographic<float_t>::create( 
                    _dims.x(), _dims.y(), 0, 100 ) ;
                pr.set_view_proj( view, proj ) ;
                tr.set_view_proj( view, proj ) ;
            }
            pr.prepare_for_rendering() ;
            tr.prepare_for_rendering() ;
        } 

        //****************************************************************************************
        virtual void_t on_render( this_t::window_id_t const wid, motor::graphics::gen4::frontend_ptr_t fe, 
            motor::application::app::render_data_in_t rd ) noexcept 
        {
            if( rd.first_frame )
            {
                pr.configure( fe ) ;
                tr.configure( fe ) ;
            }

            // render text layer 0 to screen
            {
                pr.prepare_for_rendering( fe ) ;
                pr.render( fe, 0 ) ;
                pr.render( fe, 1 ) ;
                pr.render( fe, 2 ) ;
            }

            // render text layer 0 to screen
            {
                tr.prepare_for_rendering( fe ) ;
                tr.render( fe, 0 ) ;
                tr.render( fe, 1 ) ;
                tr.render( fe, 2 ) ;
            }
        }

        //****************************************************************************************
        bool_t on_tool( this_t::window_id_t const wid, motor::application::app::tool_data_ref_t td ) noexcept 
        { 
            if( _wid_tool != wid ) return false ;

            if( ImGui::IsAnyMouseDown() ) 
                _lock_point_mouse = false ;

            auto uint_slider = []( char const * label, uint_t & n, int_t const min_, int_t const max_ )
            {
                int_t v = (int_t)n ;
                bool_t const changed = ImGui::SliderInt( label, &v, min_, max_ ) ;
                n = uint_t(v ) ;
                return changed ;
            } ;

            bool_t recomp = false ;

            if( ImGui::Begin("Triangulation") )
            {
                ImGui::Separator() ; ImGui::SameLine() ;
                ImGui::Text("General Properties") ;
                if( ImGui::Button( "Reset##triangulation" ) )
                {
                    _polygon.clear() ;
                }
                
                //ImGui::Checkbox( "Advance", &_props.advance ) ;
                ImGui::Checkbox( "Draw Outline", &_draw_outline ) ;
                ImGui::Checkbox( "Draw Triagles", &_draw_tris ) ;
                ImGui::Checkbox( "Draw Convex Hull", &_draw_convex_hull ) ;

                ImGui::Separator() ; ImGui::SameLine() ;
                ImGui::Text("Drawing Properties") ;
                ImGui::SliderFloat( "Point Radius", &_point_radius, 1.0f, 5.0f ) ;

                
            }


            ImGui::End() ;


            return true ;
        }

        //****************************************************************************************
        virtual void_t on_shutdown( void_t ) noexcept
        {
        }

    private:

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
