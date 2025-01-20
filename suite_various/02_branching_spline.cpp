

#include <motor/platform/global.h>

#include <motor/gfx/camera/generic_camera.h>
#include <motor/gfx/primitive/primitive_render_2d.h>

#include <motor/math/utility/angle.hpp>
#include <motor/math/interpolation/interpolate.hpp>

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

        motor::gfx::generic_camera_t camera ;
        motor::gfx::primitive_render_2d_t pr ;

        motor::math::vec2ui_t _extend ;
        motor::math::vec2f_t _cur_mouse ;
        motor::math::vec2f_t _cur_mouse_dif ;

        bool_t _left_down = false ;

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
                camera.set_dims( float_t(_extend.x()), float_t(_extend.y()), 1.0f, 5000.0f ) ;
                camera.orthographic() ;
            }
        }

        //***************************************************************************************************
        virtual void_t on_device( device_data_in_t dd ) noexcept 
        {
            motor::controls::types::three_mouse mouse( dd.mouse ) ;
            _cur_mouse_dif = _cur_mouse ;
            _cur_mouse = mouse.get_local() * motor::math::vec2f_t( 2.0f ) - motor::math::vec2f_t( 1.0f ) ;
            _cur_mouse_dif = _cur_mouse - _cur_mouse_dif ;

            _left_down = mouse.is_pressing( motor::controls::types::three_mouse::button::left ) ;
        }

        //***************************************************************************************************
        float_t thickness = 10.0f ;
        float_t split_angle = motor::math::angle<float_t>::degree_to_radian( 180.0f ) ;
        float_t sex_offset = thickness * 0.5f ;

        bool_t draw_normal = false ;
        bool_t draw_tangent = false ;
        bool_t draw_extended = true ;
        bool_t draw_sex = true ;
        bool_t draw_split_points = false ;
        bool_t draw_outlines = true ;
        
        virtual void_t on_graphics( motor::application::app::graphics_data_in_t gd ) noexcept 
        {
            // one entry per point
            typedef motor::vector< motor::math::vec2f_t > points_t ;
            typedef motor::vector< motor::math::vec2f_t > normals_t ;
            
            // one entry per point and
            // num neighbors of point per point
            typedef motor::vector< size_t > neighbors_t ;

            // num neighbors of point per point
            typedef motor::vector< motor::math::vec2f_t > extended_t ;
            typedef motor::vector< uint8_t > counts_t ;

            // one entry per point
            // where to look for the first exteded for a point
            typedef motor::vector< size_t > offsets_t ;

            // two entries per segment
            typedef motor::vector< size_t > indices_t ;

            // nnh - 1 per point
            typedef motor::vector< float_t > angles_t ;

            static points_t pts( {
                motor::math::vec2f_t( -100.0f, -50.0f ),
                motor::math::vec2f_t( 100.0f, 50.0f ),
                motor::math::vec2f_t( 50.0f, 100.0f ),
                motor::math::vec2f_t( 200.0f, -50.0f ),
                motor::math::vec2f_t( 50.0f, 200.0f ),
                motor::math::vec2f_t( 200.0f, 100.0f ),
                motor::math::vec2f_t( 100.0f, 250.0f ),
                motor::math::vec2f_t( 200.0f, 250.0f ),
                motor::math::vec2f_t( 150.0f, 75.0f ),
                } ) ;

            //indices_t inds( { 0, 1, 1, 2 } ) ;
            indices_t inds( { 0, 1, 1, 2, 1, 3,0, 3 , 2,4, 4,6, 8, 1, 6, 8, 8,5 } ) ;
            //indices_t inds( { 0, 1, 1, 2, 2, 0 } ) ;
            //indices_t inds( { 0, 1, 1, 3, 1,2 } ) ;
            
            neighbors_t nnhs( pts.size() ) ; // num_neighbors : number of neighbors
            neighbors_t nhs( pts.size() ) ; // neighbors : actual neighbor indices
            offsets_t offs( pts.size() ) ; // offsets : where to start looking for the first neighbor
            angles_t fas ; // full_angles : required for sorting the neighbors
            offsets_t fa_offs( pts.size() ) ; // full_angles_offsets : the entry of a point with one neighbor is invalid.
            extended_t exts ; // extended : extended points per point
            counts_t num_sexts ; // number super extended per extended
            normals_t norms ; // normals : num_neighbor normals per point
            points_t split_points ; 

            // make even
            {
                if( inds.size() % 2 != 0 )
                {
                    inds.emplace_back( 0 ) ; 
                }
            }

            // find number of neighbors
            {
                for( size_t i=0; i<pts.size(); ++i )
                {
                    size_t count = 0 ;
                    for( size_t j=0; j<inds.size(); ++j )
                    {
                        count += (inds[j] == i) ? 1 : 0 ;
                    }
                    nnhs[i] = count ;
                }
            }

            // compute exteded point size 
            // compute offsets ( where to look for the first neighbor of point )
            {
                size_t accum = 0 ;
                
                for( size_t i=0; i<nnhs.size(); ++i )
                {
                    offs[i] = nnhs[i] != 0 ? accum : size_t( -1 ) ;
                    accum += nnhs[i] ;
                }
                exts.resize( accum ) ;
                nhs.resize( accum ) ;
                norms.resize( accum ) ;
                num_sexts.resize( accum ) ;
                split_points.resize( accum << 1 ) ;
            }

            // compute full angles size and offsets
            {
                size_t accum = 0 ;
                for( size_t i=0; i<nnhs.size(); ++i )
                {
                    size_t const na = nnhs[i] - 1 ;
                    fa_offs[i] = nnhs[i] != 0 && na != 0 ? accum : size_t(-1) ;
                    accum += nnhs[i] != 0 ? na : 0 ;
                }
                fas.resize( accum ) ;
            }

            // make neighbor list for each point
            {
                size_t offset = 0 ;
                for( size_t i=0; i<pts.size(); ++i )
                {
                    if( nnhs[i] == 0 ) continue ;

                    nhs[offset] = size_t(-1) ;

                    for( size_t j=0; j<inds.size(); ++j )
                    {
                        if( i == inds[j] )
                        {
                            // j&size_t(-2) <=> (j>>1)<<1 : need the first point of segment
                            size_t const idx = j & size_t(-2) ;
                            size_t const off = (j + 1) % 2 ;
                            nhs[offset] = inds[ idx + off ] ;
                            ++offset ;
                        }
                    }
                }
            }

            // compute full angles
            {
                for( size_t i=0; i<pts.size(); ++i )
                {
                    size_t const o = offs[i] ;
                    size_t const fao = fa_offs[i] ;
                    size_t const nh = nnhs[i] ;
                    size_t const na = nh - 1 ;

                    if( nh == 0 || na == 0 ) continue ;
                    
                    auto const dir0 = (pts[ nhs[o] ] - pts[i]).normalize() ;

                    for( size_t j=1; j<nh; ++j )
                    {
                        size_t const idx = o + j ;
                        
                        auto const dir1 = (pts[ nhs[idx] ] - pts[i]).normalize() ;

                        fas[ fao + j - 1 ] = motor::math::vec2fe_t::full_angle( dir0, dir1 ) ;
                    }
                }
            }

            // inplace sort neighbor list based on full angles
            // @note the first neighbor in the nh list is the base,
            // so this one is staying in place
            {
                for( size_t i=0; i<pts.size(); ++i )
                {
                    size_t const o = offs[i] ;
                    size_t const fao = fa_offs[i] ;
                    size_t const nh = nnhs[i] ;
                    size_t const na = nh - 1 ;

                    // no sort required
                    if( nh < 3 ) continue ;

                    // based on the swap of insertion_sort, 
                    // the neighbors need to be swapped too.
                    {
                        motor::mstd::insertion_sort<angles_t::value_type>::in_range(
                            fao, fao+na, fas, [&]( size_t const a, size_t const b )
                        {
                            size_t const idx0 = o + a + 1 ;
                            size_t const idx1 = o + b + 1 ;

                            auto const tmp = nhs[ idx0 ] ;
                            nhs[ idx0 ] = nhs[ idx1 ] ;
                            nhs[ idx1 ] = tmp ;
                        } ) ;
                    }
                }
            }
            
            // compute extended and normal
            {
                float_t const a90 = motor::math::angle<float_t>::degree_to_radian(90.0f) ;
                motor::math::mat2f_t const rotl = motor::math::mat2f_t::make_rotation_matrix( +a90 ) ;
                motor::math::mat2f_t const rotr = motor::math::mat2f_t::make_rotation_matrix( -a90 ) ;

                for( size_t i=0; i<pts.size(); ++i )
                {
                    size_t const o = offs[i] ;
                    size_t const nh = nnhs[i] ;

                    for( size_t j = 0; j<nh; ++j )
                    {
                        size_t const idx = o + j ;
                        size_t const nidx = o + ((j+1)%nh) ;

                        auto const dir0 = (pts[ nhs[idx] ] - pts[i]).normalize() ;
                        auto const dir1 = (pts[ nhs[nidx] ] - pts[i]).normalize() ;

                        // super extended
                        {
                            auto const fa = motor::math::vec2fe_t::full_angle( dir0, dir1 ) ;
                            num_sexts[idx] = fa > split_angle ? 2 : 1 ;
                        }

                        if( nh == 1 )
                        {
                            norms[idx] = -dir0 ;
                            exts[idx] = pts[i] + norms[idx] * thickness ;
                            num_sexts[idx] = 2 ;
                        }
                        else
                        {
                            auto const n0 = rotl * dir0 ;
                            auto const n1 = rotr * dir1 ; 

                            auto const n2 = (n0 + n1) * 0.5f ;

                            norms[idx] = n2.normalized() ;
                            exts[idx] = pts[i] + norms[idx] * thickness ;
                        }
                    }
                }
            }

            // compute split points
            {
                for( size_t i=0; i<pts.size(); ++i )
                {
                    size_t const o = offs[i] ;
                    size_t const fao = fa_offs[i] ;
                    size_t const nh = nnhs[i] ;

                    if( nh <=0 ) continue ;

                    for( size_t j=0; j<nh; ++j )
                    {
                        size_t const idx = o + j ;

                        auto const seg = pts[ nhs[idx] ] - pts[i] ;
                        auto const nrm = seg.normalized() ;
                        auto const tan = motor::math::vec2f_t( nrm.y(), -nrm.x() ).normalize() ;

                        size_t const idx2 = idx << 1 ;

                        split_points[idx2 + 0] = pts[i] + seg * 0.5f + tan * thickness ; // one side 
                        split_points[idx2 + 1] = pts[i] + seg * 0.5f - tan * thickness ; // other side
                    }
                }
            }

            // draw segments
            {
                float_t const radius = 5.0f ;

                motor::math::vec4f_t color0( 1.0f,1.0f,1.0f,1.0f) ;
                size_t const num_segs = inds.size() >> 1 ;
                for( size_t i=0; i<num_segs; ++i )
                {
                    size_t const idx = i << 1 ;
                    
                    size_t const id0 = inds[ idx+0 ] ;
                    size_t const id1 = inds[ idx+1 ] ;

                    pr.draw_circle( 1,10, pts[id0], radius, color0, color0 ) ;
                    pr.draw_line( 0, pts[id0], pts[id1], color0 ) ;
                    pr.draw_circle( 1,10, pts[id1], radius, color0, color0 ) ;
                }
            }

            #if 0
            // draw neighbor dirs
            {
                motor::math::vec4f_t color0( 1.0f,1.0f,1.0f,1.0f) ;
                
                for( size_t i=0; i<pts.size(); ++i )
                {
                    size_t const o = offs[ i ] ;
                    size_t const nh = nnhs[i] ;
                    for( size_t j=0; j<nh; ++j )
                    {
                        motor::math::vec2f_t const dir = (pts[ nhs[o+j] ] - pts[i]).normalize() * 30.0f ;

                        _pr.draw_line( 2 , pts[i], pts[i]+dir  , motor::math::vec4f_t(1.0f,0.0f,0.0f,1.0f) ) ;
                    }
                }
            }
            #endif

            // draw extended
            if( draw_extended )
            {
                motor::math::vec4f_t color0( 0.0f,1.0f,0.0f,1.0f) ;
                motor::math::vec4f_t color1( 1.0f,0.0f,0.0f,1.0f) ;

                for( size_t i=0; i<exts.size(); ++i )
                {
                    auto const color = num_sexts[i] > 1 ? color1 : color0 ;
                    pr.draw_circle( 1,10, exts[i], 2.0f, color, color ) ;
                }
            }
            
            // draw normals
            if( draw_normal )
            {
                motor::math::vec4f_t color0( 0.0f,1.0f,0.0f,1.0f) ;
                for( size_t i=0; i<exts.size(); ++i )
                {
                    auto const p0 = exts[i] ;
                    auto const p1 = p0 + norms[i] * thickness ;
                    pr.draw_line( 2 , p0, p1 , color0 ) ;
                }
            }

            // Quick TEST : draw tangent to normal
            if( draw_tangent )
            {
                motor::math::vec4f_t color0( 0.0f,1.0f,0.0f,1.0f) ;
                for( size_t i=0; i<exts.size(); ++i )
                {
                    auto const p0 = exts[i] ;
                    auto const p1 = p0 + motor::math::vec2f_t( norms[i].y(), -norms[i].x() ) * thickness ;
                    pr.draw_line( 2 , p0, p1 , color0 ) ;
                }
            }

            // Quick TEST 2 : draw super extended
            if( draw_sex )
            {
                motor::math::vec4f_t color0( 0.0f,1.0f,0.0f,1.0f) ;
                for( size_t i=0; i<exts.size(); ++i )
                {
                    if( num_sexts[i] <= 1 ) continue ;

                    auto const tang = motor::math::vec2f_t( norms[i].y(), -norms[i].x() ) ;

                    auto const p0 = exts[i] ;
                    auto const p1 = p0 + tang * thickness - norms[i] * sex_offset ;
                    auto const p2 = p0 - tang * thickness - norms[i] * sex_offset ;

                    pr.draw_circle( 1, 10, p1, 2.0f, color0, color0 ) ;
                    pr.draw_circle( 1, 10, p2, 2.0f, color0, color0 ) ;
                }
            }
            
            // Quick TEST 3 : draw split points 
            if( draw_split_points )
            {
                motor::math::vec4f_t color0( 0.0f,1.0f,0.0f,1.0f) ;
                for( size_t i=0; i<split_points.size(); ++i )
                {
                    auto const p0 = split_points[i] ;
                    pr.draw_circle( 1, 10, p0, 2.0f, color0, color0 ) ;
                }
            }

            // Quick TEST 4 : draw outlines
            if( draw_outlines )
            {
                motor::math::vec2f_t points[4] ;

                motor::math::vec4f_t color0( 0.0f,1.0f,0.0f,1.0f) ;
                for( size_t i=0; i<pts.size(); ++i )
                {
                    size_t const o = offs[ i ] ;
                    size_t const nh = nnhs[i] ;

                    if( nh == 0 ) continue ;

                    if( nh == 1 )
                    {
                        for( size_t j=0; j<nh; ++j )
                        {
                            size_t const idx = o + j ;

                            auto const tang = motor::math::vec2f_t( norms[idx].y(), -norms[idx].x() ) ;

                            auto const p0 = exts[ idx  ] ;
                            points[0] = p0 + tang * thickness - norms[idx] * sex_offset ;
                            points[1] = p0 - tang * thickness - norms[idx] * sex_offset ;
                            points[2] = split_points[ (idx << 1) + 0 ] ;
                            points[3] = split_points[ (idx << 1) + 1 ] ;
                        }

                        for( size_t i2=0; i2<4; ++i2 )
                        {
                            auto const p0 = points[i2] ;
                            auto const p1 = points[(i2+1)%4] ;
                            pr.draw_line( 2 , p0, p1 , color0 ) ;
                        }
                    }
                    else
                    {
                        for( size_t j=0; j<nh; ++j )
                        {
                            // the index computation is based on how the extended points are computed 
                            // using the full angles. So one neighbor and the last(not the next) neighbor
                            // are sharing a face.
                            size_t const idx0 = (o + ((j + 0)%nh)) ; // can be written: o+j
                            size_t const idx1 = (o + ((j + nh-1)%nh)) ; // means (j - 1)%nh

                            auto const tang0 = motor::math::vec2f_t( norms[idx0].y(), -norms[idx0].x() ) ;
                            auto const tang1 = motor::math::vec2f_t( norms[idx1].y(), -norms[idx1].x() ) ;
                                
                            auto const p0 = exts[ idx0 ] ;
                            auto const p1 = exts[ idx1 ] ;

                            auto const sex0 = p0 + tang0 * thickness - norms[idx0] * sex_offset ;
                            auto const sex1 = p0 - tang0 * thickness - norms[idx0] * sex_offset ;

                            auto const sex2 = p1 + tang1 * thickness - norms[idx1] * sex_offset ;
                            auto const sex3 = p1 - tang1 * thickness - norms[idx1] * sex_offset ;

                            // do the quad
                            {
                                points[0] = num_sexts[idx0] == 1 ? exts[ idx0 ] : sex0 ;
                                points[1] = num_sexts[idx1] == 1 ? exts[ idx1 ] : sex3 ;
                                points[2] = split_points[ (idx0 << 1) + 0 ] ;
                                points[3] = split_points[ (idx0 << 1) + 1 ] ;
                            }

                            for( size_t i2=0; i2<4; ++i2 )
                            {
                                auto const lp0 = points[i2] ;
                                auto const lp1 = points[(i2+1)%4] ;
                                pr.draw_line( 2 , lp0, lp1 , color0 ) ;
                            }

                            // connect hole between super extended and extended
                            {
                                points[0] = num_sexts[idx0] == 1 ? exts[ idx0 ] : sex0 ;
                                points[1] = num_sexts[idx0] == 1 ? exts[ idx0 ] : sex1 ;
                                points[2] = num_sexts[idx1] == 1 ? exts[ idx1 ] : sex2 ;
                                points[3] = num_sexts[idx1] == 1 ? exts[ idx1 ] : sex3 ;
                            }

                            for( size_t i2=0; i2<4; ++i2 )
                            {
                                auto const lp0 = points[i2] ;
                                auto const lp1 = points[(i2+1)%4] ;
                                pr.draw_line( 2 , lp0, lp1 , color0 ) ;
                            }
                        }
                    }
                }
            }

            // test pick points
            {
                static size_t idx = size_t(-1) ;
                auto const ray = camera.create_ray_norm( _cur_mouse ) ;
                auto const plane = motor::math::vec3f_t(0.0f,0.0f,-1.0f) ;
                float_t const lambda = - ray.get_origin().dot( plane ) / ray.get_direction().dot( plane ) ;

                // point on plane
                motor::math::vec2f_t const pop = ray.point_at( lambda ).xy() ;
                //_pr.draw_circle( 4, 10, pop, radius, 
                 //           motor::math::vec4f_t(1.0f,1.0f,0.0f,1.0f), motor::math::vec4f_t(1.0f) ) ;

                float_t const radius = 5.0f ;
                for( size_t i=0; i<pts.size(); ++i )
                {
                    auto const p = pts[i] ;
                    auto const b = (p - pop).length() < radius ;  
                    if( b )
                    {
                        //pr.draw_circle( 4, 10, p, radius, 
                          //  motor::math::vec4f_t(1.0f,1.0f,0.0f,1.0f), motor::math::vec4f_t(1.0f) ) ;
                        if( _left_down && idx == size_t(-1) )
                            idx = i ;
                    }
                }

                if( idx != size_t(-1) ) pts[idx] += _cur_mouse_dif * motor::math::vec2f_t( _extend >> 1 ) ;
                if( !_left_down ) idx = size_t(-1) ;
            }

            {
                pr.set_view_proj( camera.mat_view(), camera.mat_proj() ) ;
                pr.prepare_for_rendering() ;
            }
        } 
       
        //***************************************************************************************************
        virtual void_t on_render( this_t::window_id_t const wid, motor::graphics::gen4::frontend_ptr_t fe, 
            motor::application::app::render_data_in_t rd ) noexcept 
        {
            if( rd.first_frame )
            {
                pr.configure( fe ) ;
            }

            {
                pr.prepare_for_rendering( fe ) ;
                for( size_t i=0; i<100; ++i )
                {
                    pr.render( fe, i ) ;
                }
            }
        }

        //***************************************************************************************************
        virtual bool_t on_tool( this_t::window_id_t const wid, motor::application::app::tool_data_ref_t ) noexcept 
        { 
            if( wid != 0 ) return false ;

            ImGui::Begin( "Control and Info" ) ;
            
            {
                ImGui::SliderFloat( "thickness", &thickness, 1.0f, 30.0f ) ;
            }
            {
                split_angle = motor::math::angle<float_t>::radian_to_degree( split_angle ) ;
                ImGui::SliderFloat( "split angle", &split_angle, 0.0f, 360.0f ) ;
                split_angle = motor::math::angle<float_t>::degree_to_radian( split_angle ) ;
            }
            {
                ImGui::SliderFloat( "super extended offset", &sex_offset, 0.0f, thickness ) ;
            }
            {
                ImGui::Checkbox( "draw extended", &draw_extended ) ;
                ImGui::Checkbox( "draw outline", &draw_outlines ) ;
                ImGui::Checkbox( "draw super ext", &draw_sex ) ;
                ImGui::Checkbox( "draw normals", &draw_normal ) ;
                ImGui::Checkbox( "draw tangent", &draw_tangent ) ;
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

    motor::concurrent::global::deinit() ;
    motor::log::global::deinit() ;
    motor::memory::global::dump_to_std() ;


    return ret ;
}
