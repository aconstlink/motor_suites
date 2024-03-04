
#pragma once

#include <motor/math/vector/vector4.hpp>
#include <motor/math/vector/vector2.hpp>
#include <motor/math/vector/vector3.hpp>
#include <motor/std/vector>
#include <motor/core/types.hpp>
#include <functional>

namespace world
{
    using namespace motor::core::types ;
    class ij_id
    {
        motor_this_typedefs( ij_id ) ;

    private: 

        // x,y,z : i, j, id
        motor::math::vec3ui_t _ij_id = motor::math::vec3ui_t( uint_t(-1) ) ;

    public:

        ij_id( void_t ) noexcept {}
        ij_id( uint_t const i, uint_t const j, uint_t const id ) { _ij_id = motor::math::vec3ui_t( i, j, id ) ; }
        ij_id( motor::math::vec2ui_t const ij, uint_t const id ) { _ij_id = motor::math::vec3ui_t( ij, id ) ; }
        ij_id( motor::math::vec3ui_t const ij_id_ ) { _ij_id = ij_id_ ; }
        ij_id( this_cref_t rhv ) noexcept { _ij_id = rhv._ij_id ; }
        ij_id( this_rref_t rhv ) noexcept { _ij_id = rhv._ij_id ; }
        ~ij_id( void_t ) noexcept {}

        motor::math::vec2ui_t get_ij( void_t ) const noexcept
        {
            return _ij_id.xy() ;
        }

        size_t get_id( void_t ) const noexcept
        {
            return _ij_id.z() ;
        }

        bool_t operator == ( motor::math::vec2ui_t const ij ) const noexcept
        {
            return this_t::get_ij().equal( ij ).all() ;

        }

        bool_t operator == ( size_t const id ) const noexcept
        {
            return this_t::get_id() == id ;
        }

    };
    motor_typedef( ij_id ) ;
}

namespace world
{
    using namespace motor::core::types ;

    struct dimensions
    {
        motor_this_typedefs( dimensions ) ;

    private:

        /// in regions per grid
        motor::math::vec2ui_t regions_per_grid ;

        /// in cells per region
        motor::math::vec2ui_t cells_per_region ;

        /// in pixels per cell
        motor::math::vec2ui_t pixels_per_cell ;

    public:

        dimensions( void_t ) noexcept {}

        dimensions( motor::math::vec2ui_cref_t pre_grid, 
            motor::math::vec2ui_cref_t per_region, motor::math::vec2ui_cref_t per_cell ) noexcept
        {
            regions_per_grid = pre_grid ;
            cells_per_region = per_region ;
            pixels_per_cell = per_cell ;
        }

        dimensions( this_cref_t rhv ) noexcept
        {
            regions_per_grid = rhv.regions_per_grid ;
            cells_per_region = rhv.cells_per_region ;
            pixels_per_cell = rhv.pixels_per_cell ;
        }

        this_ref_t operator = ( this_cref_t rhv ) noexcept
        {
            regions_per_grid = rhv.regions_per_grid ;
            cells_per_region = rhv.cells_per_region ;
            pixels_per_cell = rhv.pixels_per_cell ;
            return *this ;
        }

        motor::math::vec2ui_t get_regions_per_grid( void_t ) const noexcept
        {
            return regions_per_grid ;
        }

        motor::math::vec2ui_t get_cells_per_region( void_t ) const noexcept
        {
            return cells_per_region ;
        }

        motor::math::vec2ui_t get_pixels_per_cell( void_t ) const noexcept
        {
            return pixels_per_cell ;
        }

        motor::math::vec2ui_t get_pixels_per_region( void_t ) const noexcept
        {
            return cells_per_region * pixels_per_cell ;
        }

        motor::math::vec2ui_t get_cells_global( void_t ) const noexcept
        {
            return regions_per_grid * cells_per_region ;
        }

        ij_id calc_cell_ij_id( motor::math::vec2ui_cref_t ij ) const noexcept
        {
            auto const x = ij.x() % cells_per_region.x() ;
            auto const y = ij.y() % cells_per_region.y() ;

            return ij_id( motor::math::vec2ui_t(x, y), x + y * cells_per_region.x() ) ;
        }

        ij_id calc_region_ij_id( motor::math::vec2ui_cref_t ij ) const noexcept
        {
            auto const x = ij.x() % regions_per_grid.x() ;
            auto const y = ij.y() % regions_per_grid.y() ;

            return ij_id( motor::math::vec2ui_t(x, y), x + y * regions_per_grid.x() ) ;
        }

        motor::math::vec2ui_t relative_to_absolute( motor::math::vec2i_cref_t v ) const noexcept
        {
            auto const dims = pixels_per_cell * cells_per_region * regions_per_grid ;
            auto const dims_half = dims >> motor::math::vec2ui_t( 1 ) ;

            return dims_half + v ;
        }

        //**************************************************************************
        // the incoming vector will be min/max to the grids bounds and returned.
        motor::math::vec2i_t keep_in_range( motor::math::vec2i_t v ) const noexcept
        {
            auto const pixels = pixels_per_cell * cells_per_region * regions_per_grid ;
            auto const half = pixels >> motor::math::vec2ui_t( 1 ) ;

            return v.min( half ).max( motor::math::vec2i_t( half ).negated() ) ;
        }

        //**************************************************************************
        motor::math::vec2i_t transform_to_center( motor::math::vec2ui_cref_t pos ) const noexcept
        {
            auto const dims = pixels_per_cell * cells_per_region * regions_per_grid ;
            auto const dims_half = dims >> motor::math::vec2ui_t( 1 ) ;

            return pos - dims_half ;
        }

        //**************************************************************************
        motor::math::vec2i_t transform_cell_to_center( motor::math::vec2ui_cref_t pos ) const noexcept
        {
            auto const dims = cells_per_region * regions_per_grid ;
            auto const dims_half = dims >> motor::math::vec2ui_t( 1 ) ;

            return pos - dims_half ;
        }

        //**************************************************************************
        /// @param pos in pixels from the center of the field
        motor::math::vec2ui_t calc_region_ij( motor::math::vec2i_cref_t pos ) const noexcept
        {
            auto const cdims = pixels_per_cell * cells_per_region ;
            auto const dims = cdims * regions_per_grid ;
            auto const dims_half = dims >> motor::math::vec2ui_t( 1 ) ;

            motor::math::vec2ui_t const pos_global = dims_half + pos ;

            return pos_global / cdims ;
        }

        //**************************************************************************
        /// @param pos in pixels from the center of the field
        motor::math::vec2ui_t calc_cell_ij_global( motor::math::vec2i_cref_t pos ) const noexcept
        {
            auto const all_cells = cells_per_region * regions_per_grid ;
            auto const dims = pixels_per_cell * all_cells ;
            auto const dims_half = dims >> motor::math::vec2ui_t( 1 ) ;

            motor::math::vec2ui_t const pos_global = dims_half + pos ;

            return pos_global / pixels_per_cell ;
        }

        //**************************************************************************
        /// @param pos in pixels from the center of the field
        motor::math::vec2ui_t calc_cell_ij_local( motor::math::vec2i_cref_t pos ) const noexcept
        {
            return this_t::calc_cell_ij_global( pos ) % cells_per_region ;
        }

        //**************************************************************************
        /// pass global cell ij and get region ij and local cell ij
        void global_to_local( motor::math::vec2ui_in_t global_ij, motor::math::vec2ui_out_t region_ij, 
            motor::math::vec2ui_out_t local_ij ) const noexcept
        {
            region_ij = global_ij / cells_per_region ;
            local_ij = global_ij % cells_per_region ;
        }

        //**************************************************************************
        /// world space region bounding box
        motor::math::vec4f_t calc_region_bounds( uint_t const ci, uint_t const cj ) const noexcept
        {
            motor::math::vec2ui_t const ij( ci, cj ) ;

            auto const cdims = pixels_per_cell * cells_per_region ;
            auto const cdims_half = cdims / motor::math::vec2ui_t( 2 ) ;

            auto const dims = cdims * regions_per_grid ;
            auto const dims_half = dims / motor::math::vec2ui_t(2) ;

            motor::math::vec2i_t const start = motor::math::vec2i_t(dims_half).negated() ;

            motor::math::vec2f_t const min = start + cdims * ( ij + motor::math::vec2ui_t( 0 ) ) ;
            motor::math::vec2f_t const max = start + cdims * ( ij + motor::math::vec2ui_t( 1 ) ) ;

            return motor::math::vec4f_t( min, max ) ;
        }

        //**************************************************************************
        motor::math::vec4f_t calc_cell_bounds( uint_t const ci, uint_t const cj ) const noexcept
        {
            return motor::math::vec4f_t() ;
        }

        //**************************************************************************
        /// returns the amount of cells for passed pixels
        motor::math::vec2ui_t pixels_to_cells( motor::math::vec2ui_cref_t v ) const noexcept
        {
            return v / pixels_per_cell ;
        }

        motor::math::vec2ui_t cells_to_pixels( motor::math::vec2ui_cref_t cells ) const noexcept
        {
            return cells * pixels_per_cell ;
        }

        motor::math::vec2i_t cells_to_pixels( motor::math::vec2i_cref_t cells ) const noexcept
        {
            return cells * pixels_per_cell ;
        }

        motor::math::vec2ui_t regions_to_pixels( motor::math::vec2ui_cref_t regions ) const noexcept
        {
            return regions * cells_per_region * pixels_per_cell ;
        }

        /// stores the area of involved regions and cells
        /// the area is determined by the four corners
        struct regions_and_cells
        {
            motor::math::vec2ui_t regions[4] ;
            motor::math::vec2ui_t cells[4] ;
            motor::math::vec2i_t ocells[4] ;

            motor::math::vec2ui_t cell_dif( void_t ) const noexcept
            {
                return cells[ 2 ] - cells[ 0 ] + motor::math::vec2ui_t( 1 ) ;
            }

            motor::math::vec2ui_t cell_min( void_t ) const noexcept
            {
                return cells[ 0 ] ;
            }

            motor::math::vec2ui_t cell_max( void_t ) const noexcept
            {
                return cells[ 2 ] ;
            }

            motor::math::vec2ui_t region_dif( void_t ) const noexcept
            {
                return regions[ 2 ] - regions[ 0 ] + motor::math::vec2ui_t( 1 ) ;
            }

            motor::math::vec2ui_t region_min( void_t ) const noexcept 
            {
                return regions[ 0 ] ;
            }

            motor::math::vec2ui_t region_max( void_t ) const noexcept
            {
                return regions[ 2 ] ;
            }

            motor::math::vec2i_t ocell_min( void_t ) const noexcept
            {
                return ocells[0] ;
            }

            motor::math::vec2i_t ocell_max( void_t ) const noexcept
            {
                return ocells[2] ;
            }

            typedef std::function< void_t ( motor::math::vec2i_cref_t ) > for_each_ocell_funk_t ;
            void_t for_each( for_each_ocell_funk_t funk ) const noexcept
            {
                for( auto x = ocell_min().x(); x < ocell_max().x(); ++x )
                {
                    for( auto y = ocell_min().y(); y < ocell_max().y(); ++y )
                    {
                        funk( motor::math::vec2i_t( x, y ) ) ;
                    }
                }
            }

            bool_t is_region_inside( motor::math::vec2ui_cref_t r ) const noexcept
            {
                return 
                    r.greater_equal_than( region_min() ).all() && 
                    r.less_equal_than( region_max() ).all() ;
            }
        };
        motor_typedef( regions_and_cells ) ;

        //**************************************************************************
        // arguments are in pixels
        regions_and_cells_t calc_regions_and_cells( motor::math::vec2i_cref_t pos, motor::math::vec2ui_cref_t radius ) const noexcept
        {
            regions_and_cells_t od ;
            
            // cells
            {
                auto const dims = pixels_per_cell * cells_per_region * regions_per_grid ;
                auto const dims_half = dims / motor::math::vec2ui_t( 2 ) ;

                auto const min = this_t::calc_cell_ij_global( this_t::keep_in_range( pos - radius ) ) ;
                auto const max = this_t::calc_cell_ij_global( this_t::keep_in_range( pos + radius ) ) ;

                od.cells[ 0 ] = min ;
                od.cells[ 1 ] = motor::math::vec2ui_t( min.x(), max.y() ) ;
                od.cells[ 2 ] = max ;
                od.cells[ 3 ] = motor::math::vec2ui_t( max.x(), min.y() ) ;
            }

            // regions
            {
                auto const min = od.cells[ 0 ] ;
                auto const max = od.cells[ 2 ] ;

                od.regions[0] = min / cells_per_region ;
                od.regions[1] = motor::math::vec2ui_t( min.x(), max.y() ) / cells_per_region ;
                od.regions[2] = max / cells_per_region ;
                od.regions[3] = motor::math::vec2ui_t( max.x(), min.y() ) / cells_per_region ;
            }

            // offset cells (ocells)
            {
                od.ocells[0] = transform_cell_to_center( od.cells[0] ) ;
                od.ocells[1] = transform_cell_to_center( od.cells[1] ) ;
                od.ocells[2] = transform_cell_to_center( od.cells[2] ) ;
                od.ocells[3] = transform_cell_to_center( od.cells[3] ) ;
            }

            return od ;
        }
    };
    motor_typedef( dimensions ) ;
}



namespace world
{
    using namespace motor::core::types ;

    template< class T >
    struct data
    {
        /// from cells ij index
        ij_id index ;
        T t ;
    };

    class cell
    {
        // material id
        // collision boxes
    };

    class region
    {
        // pre-rendered texture 
        motor::vector< data< cell > > cells ;
    };

    class grid
    {
        motor_this_typedefs( grid ) ;

        motor::vector< data< region > > _regions ;
        world::dimensions_t _dims ;

    public:

        grid( void_t ) noexcept {}
        grid( world::dimensions_cref_t dims ) noexcept : _dims( dims ){}
        ~grid( void_t ) noexcept {}

        world::dimensions_cref_t get_dims( void_t ) const noexcept
        {
            return _dims ;
        }


    };
    motor_typedef( grid ) ;
}