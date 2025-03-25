
#include <motor/platform/global.h>

#include <motor/gfx/primitive/primitive_render_2d.h>
#include <motor/gfx/font/text_render_2d.h>

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

    class mesh
    {
        motor_this_typedefs( mesh ) ;

    private:

        using edge_t = std::pair< size_t, size_t > ;
        using edges_t = motor::vector< edge_t > ;

        using points_t = motor::vector< motor::math::vec2f_t > ;
        points_t _points ;
        edges_t _edges ;
        edges_t _tri ;

        static edge_t flip( edge_t const & e ) noexcept
        {
            return std::make_pair( e.second, e.first ) ;
        }

    public:

        void_t add_point( motor::math::vec2f_cref_t p ) noexcept
        {
            _points.emplace_back( p ) ;

            if( _points.size() > 1 )
            {
                _edges.resize( _points.size()  ) ;
                _edges[_edges.size()-2] = ( std::make_pair( _points.size()-2, _points.size()-1 ) ) ;
                _edges[_edges.size()-1] = ( std::make_pair( _points.size()-1, 0 ) ) ;
            }
            
        }

        void_t remove_last_point_from_polygon( void_t ) noexcept
        {
            if( _edges.size() <= 1  )
            {
                _edges.clear() ;
                _points.clear() ;
                return ;
            }

            _edges[_edges.size()-2] = std::make_pair( _points.size()-2, 0 ) ;
            _edges.resize( _edges.size() - 1 ) ;
            _points.resize( _points.size() - 1 ) ;
        }

        edges_t const & get_polygon_edges( void_t ) const noexcept
        {
            return _edges ;
        }

        bool_t has_polygon_edges( void_t ) const noexcept
        {
            return _edges.size() > 0 ;
        }

        points_t const & get_points( void_t ) const noexcept
        {
            return _points ;
        }

        points_t const & points( void_t ) const noexcept
        {
            return _points ;
        }

        void_t clear( void_t ) noexcept
        {
            _edges.clear() ;
            _points.clear() ;
        }

    public:

        struct polygon_info
        {
            size_t _start ;
            size_t num_edges ;

            size_t start( void_t ) const noexcept
            {
                return _start ;
            }

            size_t last( void_t ) const noexcept
            {
                return (_start + num_edges)-1 ;
            }

            size_t end( void_t ) const noexcept
            {
                return (_start + num_edges) ;
            }
        };

        struct storage
        {
            motor::vector< polygon_info > poly_infos ;
            edges_t edges ;
        };
        motor_typedef( storage ) ;

        struct ping_pong
        {
            motor_this_typedefs( ping_pong ) ;

        private:

            size_t idx = 0 ;
            storage ping_pong[2] ;

            size_t read_idx( void_t ) const noexcept
            {
                return idx % 2 ;
            }

            size_t write_idx( void_t ) const noexcept
            {
                return (idx+1) % 2 ;
            }

        public:

            bool_t swap_last_poly_info_in_buffer_to( size_t const i, motor::vector< polygon_info > & polys ) noexcept
            {
                if( polys.size() == 0 || i > polys.size() - 1 ) return false ;
                
                polys[i] = polys.back() ;
                polys.resize( polys.size()-1 ) ;

                return true ;
            }

            bool_t swap_last_poly_info_in_read_to( size_t const i ) noexcept
            {
                return this_t::swap_last_poly_info_in_buffer_to( i, 
                    ping_pong[ this_t::read_idx() ].poly_infos ) ;
            }

            bool_t swap_last_poly_info_in_write_to( size_t const i ) noexcept
            {
                return this_t::swap_last_poly_info_in_buffer_to( i, 
                    ping_pong[ this_t::write_idx() ].poly_infos ) ;
            }

            storage_cref_t read( void_t ) const noexcept
            {
                return ping_pong[ this_t::read_idx() ] ;
            }

            storage_ref_t write( void_t ) noexcept
            {
                return ping_pong[ this_t::write_idx() ] ;
            }

            void_t swap_and_clear( void_t ) noexcept
            {
                idx = write_idx() ;
                this_t::write().poly_infos.clear() ;
                this_t::write().edges.clear() ;
            }
        };
       
    private:

        motor::math::vec2f_t ortho_from_edge_left( this_t::edge_t const & e ) const noexcept
        {
            auto const [a, b] = e ;
            auto const cur_dir = ( _points[ b ] - _points[ a ] ).normalize() ;
            return cur_dir.ortho_left() ;
        }

        motor::math::vec2f_t ortho_from_edge_left( size_t const e, this_t::edges_t const & edges ) const noexcept
        {
            return this_t::ortho_from_edge_left( edges[ e ] ) ;
        }

        void_t draw_normal( motor::math::vec2f_cref_t a, motor::math::vec2f_cref_t b, 
                motor::math::vec4f_cref_t color, motor::gfx::primitive_render_2d_ref_t pr ) const noexcept
        {
            auto const nrm = (b-a).normalize().ortho_left() ;
            auto const mid = a+(b-a)*0.5f ;
            pr.draw_line( 2, mid, mid+nrm*10.0f, color ) ;
        }

        void_t draw_normal( motor::math::vec2f_cref_t a, motor::math::vec2f_cref_t b, motor::math::vec2f_cref_t nrm, 
                motor::math::vec4f_cref_t color, motor::gfx::primitive_render_2d_ref_t pr ) const noexcept
        {
            auto const mid = a+(b-a)*0.5f ;
            pr.draw_line( 2, mid, mid+nrm*10.0f, color ) ;
        }

        void_t draw_normal( this_t::edge_t const & e, motor::math::vec4f_cref_t color, 
            motor::gfx::primitive_render_2d_ref_t pr ) const noexcept
        {
            return this_t::draw_normal( _points[e.first], _points[e.second], color, pr ) ;
        }

        void_t draw_normal_at( size_t const e, this_t::edges_t const & edges, motor::gfx::primitive_render_2d_ref_t pr ) const noexcept
        {
            auto const [a, b] = edges[ e ] ;
            auto const ortho = this_t::ortho_from_edge_left( e, edges ) ;
            this_t::draw_normal( _points[ a ], _points[ b ], ortho,
                motor::math::vec4f_t( 0.0f, 1.0f, 0.0f, 1.0f ), pr ) ;
        }

        void_t draw_line( this_t::edge_t const & e, motor::math::vec4f_cref_t color, motor::gfx::primitive_render_2d_ref_t pr ) const noexcept
        {
            pr.draw_line( 1, _points[e.first], _points[e.second], color  ) ;
        }

        void_t draw_diagonal( this_t::edge_t const & e, motor::gfx::primitive_render_2d_ref_t pr ) const noexcept
        {
            auto const diag_color = motor::math::vec4f_t( 1.0f, 0.0f, 1.0f, 1.0f ) ;

            this_t::draw_normal( e, diag_color, pr ) ;
            pr.draw_line( 1, _points[e.first], _points[e.second], diag_color ) ;
        }

        motor::math::vec2f_t dir_from_edge( this_t::edge_t const & e ) const noexcept
        {
            return ( _points[ e.second ] - _points[ e.first ] ).normalize() ;
        }

        // e1 follow e0
        bool_t is_convex( motor::math::vec2f_cref_t e0_dir, motor::math::vec2f_cref_t e1_dir ) const noexcept
        {
            auto const dir = e0_dir ;
            auto const nrm = dir.ortho_left() ;

            motor::math::vec2f_t const signs (
                e1_dir.dot( dir ),
                e1_dir.dot( nrm )
            ) ;

            return 
                (signs.x() >= 0.0f && signs.y() >= 0.0f) || 
                (signs.x() < 0.0f && signs.y() > 0.0f ) ;

        }

        // replaces e0 with {-1,-1} and e1 with e
        void_t replace_edges( size_t const e0, size_t const e1, this_t::edge_t const & e, this_t::edges_t & edges ) const noexcept
        {
            edges[e0] = {size_t(-1), size_t(-1) } ;
            edges[e1] = e ;
        }

        // pass eid where this func searches for next id != -1
        size_t cur_edge( size_t eid, edges_t const & edges ) const noexcept
        {
            size_t id = eid % edges.size() ;
            while( true ) 
            {
                if ( edges[ id ].first == size_t( -1 ) ) 
                {
                    id = ++eid % edges.size() ;
                    continue ;
                }
                break ;
            }
            return id ;
        }

        // pass eid where this func searches for next id != -1
        // eid should be determined by cur_edge
        size_t next_edge( size_t eid, edges_t const & edges ) const noexcept
        {
            size_t id = ++eid % edges.size() ;
            while( true ) 
            {
                if ( edges[ id ].first == size_t( -1 ) ) 
                {
                    id = ++eid % edges.size() ;
                    continue ;
                }
                break ;
            }
            return id ;
        }

    public:

        struct trig_props
        {
            // when diagonal found, advance the interation
            // index by one additionally.
            bool_t advance = false ;
        };

        // ear-clippling triangulation
        this_t::edges_t triangulate( trig_props const & props ) noexcept
        {
            this_t::edges_t ret ;
            ret.reserve( _points.size() * 3 ) ;

            if( _points.size() < 3 ) return ret ;

            auto edges = _edges ;

            size_t tris = 0 ;
            size_t const tris_need = _points.size() - 2 ;

            size_t ei = size_t(-1) ;
            while( tris != tris_need )
            {
                if( ++ei > tris_need*4 ) break ;

                size_t const e0i = this_t::cur_edge( ei, edges ) ;
                size_t const e1i = this_t::next_edge( e0i, edges ) ;

                auto const e0 = edges[ e0i ] ;
                auto const e1 = edges[ e1i ] ;

                auto const [a,b] = e0 ;
                auto const [c,d] = e1 ;

                // test if already triangle
                {
                    auto const e2 = edges[ this_t::next_edge( e1i, edges ) ] ;
                    if( a == e2.second && d == e2.first ) 
                    {
                        ret.emplace_back( edges[e0i] ) ;
                        ret.emplace_back( edges[e1i] ) ;
                        ret.emplace_back( e2 ) ;

                        this_t::replace_edges( e0i, e1i, {size_t(-1), size_t(-1)}, edges ) ;

                        ++tris ;

                        if( props.advance ) ++ei ;

                        continue ;
                    }
                }

                this_t::edge_t diag = { d, a } ;

                {
                    bool_t const is_conv = this_t::is_convex( this_t::dir_from_edge( e0 ), 
                        this_t::dir_from_edge( e1 ) ) ;

                    if( !is_conv ) continue ;
                }

                // test if point inside triangle
                {
                    size_t i = size_t(-1) ;
                    while( ++i < _points.size() )
                    {
                        if ( i == a || i == b || i == d ) continue ;

                        auto const p0 = this_t::ortho_from_edge_left( e0 ) ;
                        auto const p1 = this_t::ortho_from_edge_left( e1 ) ;
                        auto const p2 = this_t::ortho_from_edge_left( diag ) ;

                        motor::math::vec3f_t ndot(
                            p0.dot( _points[ i ] - _points[ c ] ),
                            p1.dot( _points[ i ] - _points[ c ] ),
                            p2.dot( _points[ i ] - _points[ d ] ) ) ;

                        if ( ( motor::math::vec3f_t( 0.0f ).less_than( ndot ) ).all() )
                            break ;
                    }

                    if( i != _points.size() ) continue ;
                }
                
                {
                    ret.emplace_back( edges[ e0i ] ) ;
                    ret.emplace_back( edges[ e1i ] ) ;
                    ret.emplace_back( diag ) ;

                    this_t::replace_edges( e0i, e1i, {diag.second, diag.first}, edges ) ;
                    
                    if( props.advance ) ++ei ;

                    ++tris ;
                }
            }

            return ret ;
        }

        // visualized triangulation
        this_t::edges_t triangulate( motor::gfx::primitive_render_2d_ref_t pr, 
            motor::gfx::text_render_2d_ref_t tr, motor::math::vec2f_cref_t wnd_dims, trig_props const & props )
        {
            // render outline
            if( this_t::has_polygon_edges() )
            {
                auto const & polygon = _edges ;
                auto const & points = _points ; 

                motor::math::vec4f_t const col(1.0f, 1.0f, 0.0f, 1.0f ) ;
                pr.draw_lines( 0, polygon.size(), [&]( size_t const i )
                {
                    auto const [a,b] = polygon[i] ;
                    return motor::gfx::line_render_2d::line_t{ { points[a], points[b] }, col } ;
                } ) ;
            }

            this_t::edges_t ret ;
            ret.reserve( _points.size() * 3 ) ;

            if( _points.size() < 3 ) return ret ;

            auto edges = _edges ;

            size_t tris = 0 ;
            size_t const tris_need = _points.size() - 2 ;

            size_t ei = size_t(-1) ;
            while( tris != tris_need )
            {
                ++ei ;
                size_t const e0i = this_t::cur_edge( ei, edges ) ;
                size_t const e1i = this_t::next_edge( e0i, edges ) ;

                auto const e0 = edges[ e0i ] ;
                auto const e1 = edges[ e1i ] ;

                auto const [a,b] = e0 ;
                auto const [c,d] = e1 ;

                // test if already triangle
                {
                    auto const e2 = edges[ this_t::next_edge( e1i, edges ) ] ;
                    if( a == e2.second && d == e2.first ) 
                    {
                        ret.emplace_back( edges[e0i] ) ;
                        ret.emplace_back( edges[e1i] ) ;
                        ret.emplace_back( e2 ) ;

                        this_t::replace_edges( e0i, e1i, {size_t(-1), size_t(-1)}, edges ) ;
                        auto const p0 = _points[ a ] ;
                        auto const p1 = _points[ b ] ;
                        auto const p2 = _points[ d ] ;
                        pr.draw_tri( 0, p0, p1, p2, motor::math::vec4f_t( 1.0f, 0.5f, 0.0f, 1.0f ) ) ;

                        ++tris ;
                        if( props.advance ) ++ei ;

                        

                        continue ;
                    }
                }

                this_t::edge_t diag = { d, a } ;

                {
                    this_t::draw_normal( _points[ a ], _points[ b ], motor::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ), pr ) ;
                    this_t::draw_normal( _points[ c ], _points[ d ], motor::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ), pr ) ;
                }

                {
                    bool_t const is_conv = this_t::is_convex( this_t::dir_from_edge( e0 ), 
                        this_t::dir_from_edge( e1 ) ) ;

                    tr.draw_text( 0, 0, 12, _points[ b ], 
                        motor::math::vec4f_t(1.0f), is_conv ? "cv" : "cc" ) ;

                    if( !is_conv ) continue ;
                }

                {
                    size_t i = size_t(-1) ;
                    while( ++i < _points.size() )
                    {
                        if ( i == a || i == b || i == d ) continue ;

                        auto const p0 = this_t::ortho_from_edge_left( e0 ) ;
                        auto const p1 = this_t::ortho_from_edge_left( e1 ) ;
                        auto const p2 = this_t::ortho_from_edge_left( diag ) ;

                        motor::math::vec3f_t ndot(
                            p0.dot( _points[ i ] - _points[ c ] ),
                            p1.dot( _points[ i ] - _points[ c ] ),
                            p2.dot( _points[ i ] - _points[ d ] ) ) ;

                        if ( ( motor::math::vec3f_t( 0.0f ).less_than( ndot ) ).all() )
                            break ;
                    }

                    if( i != _points.size() ) continue ;
                }

                this_t::draw_diagonal( diag, pr ) ;

                
                {
                    ret.emplace_back( edges[ e0i ] ) ;
                    ret.emplace_back( edges[ e1i ] ) ;
                    ret.emplace_back( diag ) ;

                    this_t::replace_edges( e0i, e1i, {diag.second, diag.first}, edges ) ;

                    {
                        auto const p0 = _points[ a ] ;
                        auto const p1 = _points[ b ] ;
                        auto const p2 = _points[ d ] ;
                        pr.draw_tri( 0, p0, p1, p2, motor::math::vec4f_t( 0.5f, 0.5f, 0.0f, 1.0f ) ) ;
                    }
                    
                    if( props.advance ) ++ei ;

                    ++tris ;
                }
            }

            return ret ;
        }

        void_t dual_graph( this_t::edges_t const & tris, motor::gfx::primitive_render_2d_ref_t pr, 
            motor::gfx::text_render_2d_ref_t tr, motor::math::vec2f_cref_t wnd_dims ) 
        {
            auto const mid_point = [&]( size_t const i, this_t::edges_t const & tris, motor::vector< motor::math::vec2f_t > const & points )
            {
                auto const [a, b] = tris[i * 3 + 0] ;
                auto const [c, d] = tris[i * 3 + 1] ;
                auto const [e, f] = tris[i * 3 + 2] ;

                auto const p0 = _points[a] ;
                auto const p1 = _points[b] ;
                auto const p2 = _points[d] ;

                auto const pm = ( p0 + p1 + p2 ) / 3 ;

                return pm ;
            } ;

            // draw midpoints
            {
                size_t const num_tris = tris.size() / 3 ;
                pr.draw_circles( 1, 10, num_tris, [&]( size_t const i )
                {
                    auto const pm = mid_point( i, tris, _points ) ;

                    motor::math::vec4f_t const col_a(1.0f, 1.0f, 1.0f, 1.0f ) ;
                    motor::math::vec4f_t const col_b(1.0f, 0.0f, 1.0f, 1.0f ) ;

                    return motor::gfx::primitive_render_2d::circle_t { pm, 1.0, col_a, col_b } ;
                } ) ;
            }

            {
                size_t const num_tris = tris.size() / 3 ;
                for( size_t i=0; i<num_tris; ++i )
                {
                    auto const e0 = tris[i * 3 + 0] ;
                    auto const e1 = tris[i * 3 + 1] ;
                    auto const e2 = tris[i * 3 + 2] ;

                    std::array< this_t::edge_t, 3 > const tri_edges = { e0, e1, e2 } ;

                    for( size_t j=0; j<num_tris; ++j )
                    {
                        if( i == j ) continue ;

                        auto const e0_ = tris[j * 3 + 0] ;
                        auto const e1_ = tris[j * 3 + 1] ;
                        auto const e2_ = tris[j * 3 + 2] ;

                        std::array< this_t::edge_t, 3 > const other_edges = 
                        { 
                            this_t::edge_t{ e0_.second, e0_.first }, 
                            this_t::edge_t{ e1_.second, e1_.first }, 
                            this_t::edge_t{ e2_.second, e2_.first } 
                        } ;

                        for( size_t e=0; e<3; ++e )
                        {
                            auto const b = motor::math::vec3b_t(
                                tri_edges[e] == other_edges[0],
                                tri_edges[e] == other_edges[1],
                                tri_edges[e] == other_edges[2]
                            ) ;

                            if( b.any() )
                            {
                                this_t::draw_normal( tri_edges[e], motor::math::vec4f_t(1.0f, 0.0f, 0.0f, 1.0f), pr ) ;

                                auto const mp0 = mid_point( i, tris, _points ) ;
                                auto const mp1 = mid_point( j, tris, _points ) ;

                                pr.draw_line( 2, mp0, mp1, motor::math::vec4f_t(1.0f) ) ;

                            }
                        }
                    }
                }
            }

        }

    private:

        size_t triangulate( motor::vector< motor::math::vec2f_t > const & points, this_t::edges_t & edges ) noexcept
        {
            for( size_t i=0; i<_edges.size(); ++i )
            {
                
            }

            return edges.size() ;
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
        motor::gfx::text_render_2d_t tr ;

    private: // window stuff

        size_t _wid_tool = size_t(-1) ;
        motor::math::vec2f_t _dims = motor::math::vec2f_t( 800.0f ) ;

        bool_t _mouse_down = false ;
        bool_t _lock_point_mouse = false ;
        motor::math::vec2f_t _cur_mouse ;
        size_t _cur_sel_point = size_t(-1) ;

    private: // mesh

        algorithm::mesh _mesh ;

        algorithm::mesh::trig_props _props ;

    private:

        float_t _point_radius = 1.0f ;
        
        bool_t _left_is_released = false ;
        bool_t _right_is_released = false ;

        bool_t _polygon_is_complete = false ;
        bool_t _mouse_is_near_start = false ;

        bool_t _draw_outline = false ;
        bool_t _draw_tris = false ;
        
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
            if( _mesh.points().size() > 0 )
            {
                auto const rad = (_point_radius * 5.0f) ;
                _mouse_is_near_start = ( _mesh.points()[0] - _cur_mouse ).length2() < rad*rad ;
            }

            if( _left_is_released && !_polygon_is_complete )
            {
                _polygon_is_complete = _mouse_is_near_start ;

                if( !_polygon_is_complete )
                {
                    _mesh.add_point( motor::math::vec2f_t( _cur_mouse ) ) ;
                    _left_is_released = false ;
                }
            }

            if( _right_is_released )
            {
                _mesh.remove_last_point_from_polygon() ;
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
            if( _mesh.points().size() > 1 )
            {
                motor::math::vec4f_t const col_a(1.0f, 1.0f, 1.0f, 1.0f ) ;
                motor::math::vec4f_t const col_b(1.0f, 0.0f, 1.0f, 1.0f ) ;
                pr.draw_circles( 1, 10, _mesh.points().size(), [&]( size_t const i )
                {
                    float_t radius = _point_radius ;
                    motor::math::vec4f_t color( 0.0f, 0.0f, 0.0f, 1.0f ) ;
                    return motor::gfx::primitive_render_2d::circle_t { _mesh.points()[i], radius, color, col_b } ;
                } ) ;
            }

            if( _mesh.points().size() > 0 && !_polygon_is_complete )
            {
                auto circ_col = motor::math::vec4f_t(1.0f, 0.0f, 0.0f, 1.0f) ;
                if( _mouse_is_near_start ) circ_col = motor::math::vec4f_t(0.0f, 1.0f, 0.0f, 1.0f) ;
                pr.draw_circle( 0, 10, _mesh.points()[0], _point_radius * 5.0f, motor::math::vec4f_t(0.0f), circ_col );
            }

            // draw cur mouse as point
            if( !_polygon_is_complete )
            {
                motor::math::vec4f_t const col_a(1.0f, 1.0f, 1.0f, 1.0f ) ;
                motor::math::vec4f_t const col_b(1.0f, 0.0f, 1.0f, 1.0f ) ;
                pr.draw_circle( 1, 10, _cur_mouse, _point_radius*2.0f, col_a, col_b ) ;
            }

            // draw mouse segment
            if( _mesh.points().size() > 0 && !_polygon_is_complete )
            {
                motor::math::vec4f_t const col_a(1.0f, 0.0f, 0.0f, 1.0f ) ;
                pr.draw_line( 0, _mesh.points().back(), _cur_mouse, col_a ) ;
            }
            
            #if 0
            // draw hull
            #if 1
            if( _mesh.has_polygon_edges() )
            {
                auto const & polygon = _mesh.get_polygon_edges() ;
                auto const & points = _mesh.get_points() ;

                motor::math::vec4f_t const col(1.0f, 1.0f, 0.0f, 1.0f ) ;
                pr.draw_lines( 2, polygon.size(), [&]( size_t const i )
                {
                    auto const [a,b] = polygon[i] ;
                    return motor::gfx::line_render_2d::line_t{ { points[a], points[b] }, col } ;
                } ) ;
            }
            #else
            if( _points.size() >= 2 )
            {
                size_t const num_segments = _points.size() ;
                motor::math::vec4f_t const col(1.0f, 1.0f, 0.0f, 1.0f ) ;
                pr.draw_lines( 2, num_segments, [&]( size_t const i )
                {
                    size_t const cur = i ;
                    size_t const nxt = (i + 1) % num_segments ;
                    return motor::gfx::line_render_2d::line_t{ { _points[cur], _points[nxt] }, col } ;
                } ) ;
            }
            #endif
            #endif
            {
                #if 1
                auto const edges = _mesh.triangulate( _props ) ;
                #else
                auto const edges = _mesh.triangulate( pr, tr, _dims*0.5f, _props ) ;
                #endif

                // draw outline
                if( _draw_outline)
                {
                    auto const & polygon = _mesh.get_polygon_edges() ;
                    auto const & points = _mesh.get_points() ;

                    motor::math::vec4f_t const col( 1.0f, 1.0f, 0.0f, 1.0f ) ;
                    pr.draw_lines( 2, polygon.size(), [&] ( size_t const i )
                    {
                        auto const [a, b] = polygon[ i ] ;
                        return motor::gfx::line_render_2d::line_t { { points[ a ], points[ b ] }, col } ;
                    } ) ;
                }
                
                if( _draw_tris )
                {
                    motor::math::vec4f_t const col( 0.0f, 1.0f, 1.0f, 1.0f ) ;

                    auto const & points = _mesh.get_points() ;
                    pr.draw_lines( 1, edges.size(), [&]( size_t const i )
                    {
                        auto const [a, b] = edges[ i ] ;
                        return motor::gfx::line_render_2d::line_t { { points[ a ], points[ b ] }, col } ;
                    } ) ;
                }

                // draw triangles
                {
                    pr.draw_tris( 0, edges.size() / 3, [&] ( size_t const i )
                    {
                        auto const p0 = _mesh.get_points()[ edges[ i * 3 + 0 ].first ] ;
                        auto const p1 = _mesh.get_points()[ edges[ i * 3 + 1 ].first ] ;
                        auto const p2 = _mesh.get_points()[ edges[ i * 3 + 2 ].first ] ;

                        return motor::gfx::tri_render_2d::tri_md { { p0, p1, p2 },
                            motor::math::vec4f_t( 1.0f, 1.0f, 1.0f, 0.5f ) } ;
                    } ) ;
                }

                {
                    _mesh.dual_graph( edges, pr, tr, _dims*0.5f ) ;
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
                    _mesh.clear() ;
                }
                
                ImGui::Checkbox( "Advance", &_props.advance ) ;
                ImGui::Checkbox( "Draw Outline", &_draw_outline ) ;
                ImGui::Checkbox( "Draw Triagles", &_draw_tris ) ;

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
