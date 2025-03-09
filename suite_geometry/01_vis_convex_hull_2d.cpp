
#include <motor/platform/global.h>

#include <motor/gfx/primitive/primitive_render_2d.h>

#include <motor/format/global.h>
#include <motor/format/future_items.hpp>
#include <motor/property/property_sheet.hpp>

#include <motor/noise/method/value_noise.h>
#include <motor/math/camera/3d/orthographic_projection.hpp>

#include <motor/std/vector_pod>
#include <motor/std/insertion_sort.hpp>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>

#include <future>

namespace algorithm
{
    using namespace motor::core::types ;

    template< typename CONT_T >
    class convex_hull
    {
        using container_t = CONT_T ;

    public:

        // inplace convex hull algorithm
        // returns the number of hull edges stored at the beginning of
        // the points container. So the algo swaps the hull points to 
        // the beginning of the point container. 
        // e0 => [p0,p1], e1 =>[p1,p2], e2 => [p2,p3], ...
        // @note The algo fails if two points lay too close together. 
        // Just think about those points. Do you really need such points?
        // So a solution to that is simply merge points which are nearly 
        // lay on the same spot!
        static size_t perform( container_t & points ) noexcept
        {
            size_t idx = 0 ;
            
            // starting with a point on the hull makes it 
            // much simpler, so find the most left point.
            {
                for( size_t i=1; i<points.size(); ++i )
                {
                    if( points[i].x() < points[idx].x() ) idx = i ;
                }
                std::swap( points[0], points[idx] ) ; idx = 0 ;
            }

            size_t num_restarts = 0 ;
            size_t num_segs = 0 ;
            for( size_t i = idx + 1; i< points.size(); ++i )
            {
                auto const dir = (points[i] - points[idx]).normalize() ;
                auto const ortho = dir.ortho() ;
                
                bool_t is_restart = false ;
                for( size_t j = i + 1; j<points.size(); ++j )
                {
                    auto const dist = ortho.dot( points[j] - points[idx] ) ;

                    // point is on the line. Dont take it. It causes 
                    // trouble.
                    if( motor::math::fn<float_t>::abs(dist) < 0.001f ) continue ;
                    
                    // if we find a point that is more to the side 
                    // we test, we just change to that point and restart
                    // the hull finding. I think this is much like 
                    // the gift wrap convex hull algo.
                    if( dist < 0.0f )
                    {
                        std::swap( points[i], points[j] ) ;
                        --i ;
                        ++num_restarts ;
                        is_restart = true ;
                        break ;
                    }
                }

                // using num_restars to track infinite loop
                // because --i.
                if( num_restarts > points.size() ) break ; 

                if( is_restart ) continue ;
                num_restarts = 0 ;

                { 
                    // if the distance with the first convex hull point at idx==0
                    // now is > 0.0f, we just
                    // made the circle complete has we have
                    // to quit the iteration. Comment out 
                    // the "else if" and see what happens.
                    auto const dist = ortho.dot( points[i] - points[0] ) ;

                    if( motor::math::fn<float_t>::abs(dist) < 0.001f)
                    {
                        // point is on the line
                    }
                    else if( dist > 0.0f ) break ;

                    ++num_segs ;
                    ++idx ;
                    
                }
            }
            return num_segs + 1;
        }
    };
}

namespace this_file
{
    using namespace motor::core::types ;

    class my_app : public motor::application::app
    {
        motor_this_typedefs( my_app ) ;
        
        motor::gfx::primitive_render_2d_t pr ;

        motor::noise::value_noise_t _vn ;


    private: // window stuff

        size_t _wid_tool = size_t(-1) ;
        motor::math::vec2f_t _dims = motor::math::vec2f_t( 800.0f ) ;

        bool_t _mouse_down = false ;
        bool_t _lock_point_mouse = false ;
        motor::math::vec2f_t _cur_mouse ;
        size_t _cur_sel_point = size_t(-1) ;

    private:

        enum class distribution
        {
            random, 
            triangle
        };
        distribution _distri ;

        float_t _point_radius = 1.0f ;

        // draw the indices as the main color
        bool_t _draw_index_color = false ;

        uint_t _num_points = 170 ;
        size_t _num_segments = 0 ;

        // internal for completely recomputing the points
        bool_t _recompute_points = true ;
        
        // spread points over the window
        float_t _spread_points = 0.25f ;
        
        // do initial sort by x component
        bool_t _do_sort = true ;

        motor::vector< motor::math::vec2f_t > _points ;
                
    private:

        //****************************************************************************************
        virtual void_t on_init( void_t ) noexcept
        {
            {
                _wid_tool = this_t::create_window( motor::application::window_info_t::create( 100, 100, 400, 400, 
                    motor::application::graphics_generation::gen4_auto ) ) ;
            }

            {
                this_t::create_window( motor::application::window_info_t::create( 500, 100, int_t(_dims.x()), int_t(_dims.y()), 
                    motor::application::graphics_generation::gen4_gl4 ) ) ;
            }

            _vn = motor::noise::value_noise_t( 727439126, 10, 2 ) ;

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

                _recompute_points = true ;
            }
        }

        //****************************************************************************************
        virtual void_t on_update( motor::application::app::update_data_in_t ud ) noexcept 
        {
            if( _recompute_points )
            {
                if( _distri == distribution::random )
                {
                    _points.resize( _num_points ) ;
                    for( uint_t i=0; i<_num_points; ++i )
                    {
                        _points[i] = this_t::rand_point( i ) * _dims * motor::math::vec2f_t(_spread_points) ;
                    }

                    // inplace dublicate point merge
                    {
                        size_t size = _points.size() ;
                        for( uint_t i=0; i<size; ++i )
                        {
                            for( uint_t j=i+1; j<size; ++j )
                            {
                                if( (_points[i] - _points[j]).length2() < 0.0001f )
                                {
                                    std::swap( _points[j--], _points[--size] ) ;
                                }
                            }
                        }
                        _points.resize( size ) ;
                        _num_points = uint_t( size ) ;
                    }

                }
                else if( _distri == distribution::triangle )
                {
                    _num_points = 4 ;
                    motor::math::vec2f_t const points[3] = 
                    {
                        -_dims* _spread_points,
                        +_dims* _spread_points*motor::math::vec2f_t(1.0f, -1.0f),
                        motor::math::vec2f_t(0.0, _dims.y()* _spread_points)
                    } ;
                    _points.resize( _num_points ) ;
                    for( size_t i=0; i<3; ++i )
                    {
                        _points[i] = points[i] ;
                    }
                    _points[3] = (_points[0] + _points[1])*0.5f ;
                }
                
                #if 0
                if( _do_sort )
                {
                    motor::mstd::insertion_sort< decltype(_points) >::for_all( _points, [&]( motor::math::vec2f_cref_t a, motor::math::vec2f_cref_t b )
                    {
                        return a.x() < b.x() ;
                    } ) ;
                }
                #endif

                _recompute_points = false ;
            }

            _num_segments = algorithm::convex_hull< decltype(_points) >::perform( _points ) ;

            {
                size_t hit = size_t(-1) ;
                float_t min_d = !_lock_point_mouse ? _point_radius*_point_radius*80.0f : _dims.length2() * 2.0f ;
                for( size_t i=0; i<_points.size(); ++i )
                {
                    float_t const d = (_points[i] - _cur_mouse).length2() ;
                    if( d <= min_d )
                    {
                        //std::printf("%zu\n", i) ;
                        hit = i ;
                        min_d = d ;
                    }
                }
                
                if( hit == size_t(-1) ) _lock_point_mouse = false ;
                if( _lock_point_mouse )
                {
                    _points[hit] = _cur_mouse ;
                }
                
                _cur_sel_point = hit ;
            }
        }

        //****************************************************************************************
        virtual void_t on_graphics( motor::application::app::graphics_data_in_t ) noexcept 
        {
            static float_t inc = 0.0f ;

            // draw points
            {
                motor::math::vec4f_t const col_a(1.0f, 1.0f, 1.0f, 1.0f ) ;
                motor::math::vec4f_t const col_b(1.0f, 0.0f, 1.0f, 1.0f ) ;
                pr.draw_circles( 1, 10, _points.size(), [&]( size_t const i )
                {
                    float_t radius = _point_radius ;
                    motor::math::vec4f_t color( 0.0f, 0.0f, 0.0f, 1.0f ) ;
                    if( _draw_index_color )
                        color = motor::math::vec4f_t( motor::math::vec3f_t (float_t(i)/float_t(_points.size())), 1.0f ) ;
                    else if( (i / _num_segments) < 1 )
                    {
                        color = motor::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ) ;
                        radius = radius * 3.0f ;
                    }
                    return motor::gfx::primitive_render_2d::circle_t { _points[i], radius, color, col_b } ;
                } ) ;

                pr.draw_circle( 0, 10, _points[0], _point_radius * 5.0f, motor::math::vec4f_t(0.0f), motor::math::vec4f_t(0.0f, 1.0f, 0.0f, 1.0f) );
            }

            // draw convex hull
            {
                motor::math::vec4f_t const col(1.0f, 1.0f, 0.0f, 1.0f ) ;
                pr.draw_lines( 2, _num_segments, [&]( size_t const i )
                {
                    size_t const cur = i ;
                    size_t const nxt = (i + 1) % _num_segments ;
                    return motor::gfx::line_render_2d::line_t{ {_points[cur], _points[nxt]}, col } ;
                } ) ;
            }

            inc += 0.01f  ;
            inc = motor::math::fn<float_t>::mod( inc, 1.0f ) ;

            {
                motor::math::mat4f_t view = motor::math::mat4f_t::make_identity() ;
                motor::math::mat4f_t proj = motor::math::m3d::orthographic<float_t>::create( 
                    _dims.x(), _dims.y(), 0, 100 ) ;
                pr.set_view_proj( view, proj ) ;
            }
            pr.prepare_for_rendering() ;
        } 

        //****************************************************************************************
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

            if( ImGui::Begin("Delaunay") )
            {
                ImGui::Separator() ; ImGui::SameLine() ;
                ImGui::Text("General Properties") ;
                if( _distri == distribution::random )
                {
                    recomp |= uint_slider( "Number of Points", _num_points, 10, 1000 ) ;
                    recomp |= ImGui::Checkbox( "Do sort", &_do_sort ) ;
                }
                else if( _distri == distribution::triangle )
                {
                }
                

                ImGui::Separator() ; ImGui::SameLine() ;
                ImGui::Text("Drawing Properties") ;
                ImGui::SliderFloat( "Point Radius", &_point_radius, 1.0f, 5.0f ) ;
                recomp |= ImGui::SliderFloat( "Spread Points", &_spread_points, 0.1f, 0.5f ) ;
                ImGui::Checkbox( "Use point index as color", &_draw_index_color ) ;

                
                {
                    static int_t cur_item = 0 ;
                    static char const * names[] = { "random", "triangle" } ;
                    if( ImGui::ListBox( "Point Distribution", &cur_item, names, IM_ARRAYSIZE(names) ) )
                    {
                        _distri = distribution( cur_item ) ;
                        recomp = true ;
                    }
                }
            }

            _recompute_points = recomp ;

            ImGui::End() ;


            return true ;
        }

        //****************************************************************************************
        virtual void_t on_shutdown( void_t ) noexcept
        {
        }

    private:

        motor::math::vec2f_t rand_point( uint_t const idx ) const noexcept
        {
            return motor::math::vec2f_t( 
                _vn.lattice( (idx << 1) + 0 ), 
                _vn.lattice( (idx << 1) + 1 ) ) ;
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
