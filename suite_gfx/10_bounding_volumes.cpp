// SPDX-FileCopyrightText: 2023-2026 Alex Link
// SPDX-License-Identifier: MIT

#include <motor/gfx/util/bounding_volume.hpp>
#include <motor/log/global.h>
#include <motor/memory/global.h>

#include <algorithm>
#include <cmath>
#include <type_traits>

namespace
{
    using namespace motor::core::types;
    namespace volumes = motor::gfx::volumes;

    bool_t near( motor::math::vec3f_cref_t a, motor::math::vec3f_cref_t b ) noexcept
    {
        return ( a - b ).length2() < 0.000001f;
    }

    bool_t matches( volumes::bounding_volume_cref_t volume,
        motor::math::vec3f_cref_t min, motor::math::vec3f_cref_t max ) noexcept
    {
        auto const bounds = volume.enclosing_aabb();
        return near( bounds.get_min(), min ) && near( bounds.get_max(), max );
    }
}

int main( void_t )
{
    static_assert( std::is_abstract_v< volumes::bounding_volume_t > );
    static_assert( std::has_virtual_destructor_v< volumes::bounding_volume_t > );

    bool_t passed = true;
    {
        auto const check = [&]( char const * message, bool_t const result )
        {
            motor::log::global_t::test( motor::string_t( "[gfx volumes] " ) + message, result );
            passed = result && passed;
        };

        {
            volumes::aabb_t const a;
            volumes::obb_t const o;
            volumes::sphere_t const s;
            motor::math::vec3f_t const zero( 0.0f );

            check( "defaults are point volumes at the origin",
                matches( a, zero, zero ) && matches( o, zero, zero ) && matches( s, zero, zero ) );
            check( "default OBB has identity orientation",
                near( o.get_bounds().orientation().get_column( 0 ), motor::math::vec3f_t( 1.0f, 0.0f, 0.0f ) ) &&
                near( o.get_bounds().orientation().get_column( 1 ), motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ) ) &&
                near( o.get_bounds().orientation().get_column( 2 ), motor::math::vec3f_t( 0.0f, 0.0f, 1.0f ) ) );
        }

        {
            motor::math::vec3f_t const min( 2.0f, -4.0f, 6.0f );
            motor::math::vec3f_t const max( 5.0f, -1.0f, 10.0f );
            motor::collide::aabb3f_t const bounds( max, min );
            volumes::aabb_t const a( bounds );
            volumes::aabb_t const b( max, min );
            volumes::aabb_t const copy( a );
            volumes::bounding_volume_cref_t base = a;

            check( "AABB runtime and static type", base.get_volume_type() == volumes::volume_type::aabb &&
                base.get_volume_type() == volumes::aabb_t::get_volume_type_static() );
            check( "AABB constructors and copy preserve bounds",
                matches( a, min, max ) && matches( b, min, max ) && matches( copy, min, max ) );
            check( "AABB exposes its collide value", near( a.get_bounds().get_min(), min ) &&
                near( a.get_bounds().get_max(), max ) );
        }

        {
            motor::math::vec3f_t const center( -4.0f, 3.0f, 7.0f );
            float_t const radius = 2.5f;
            motor::math::vec3f_t const extent( radius );
            volumes::sphere_t const a( center, radius );
            volumes::sphere_t const b( motor::collide::sphere3f_t( center, radius ) );
            volumes::sphere_t const copy( a );
            volumes::bounding_volume_cref_t base = a;

            check( "sphere runtime and static type", base.get_volume_type() == volumes::volume_type::sphere &&
                base.get_volume_type() == volumes::sphere_t::get_volume_type_static() );
            check( "sphere enclosure uses center plus/minus radius", matches( a, center - extent, center + extent ) &&
                matches( b, center - extent, center + extent ) && matches( copy, center - extent, center + extent ) );
            check( "sphere exposes its collide value", near( a.get_bounds().get_center(), center ) &&
                a.get_bounds().get_radius() == radius );
            check( "zero-radius sphere retains its center", matches( volumes::sphere_t( center, 0.0f ), center, center ) );
        }

        {
            motor::math::vec3f_t const center( 3.0f, -2.0f, 5.0f );
            motor::math::vec3f_t const half_extents( 1.0f, 2.0f, 0.5f );
            bool_t all_match = true;
            for( size_t i = 0; i != 32; ++i )
            {
                float_t const angle = float_t( i ) * 0.2f;
                float_t const c = std::cos( angle ), s = std::sin( angle );
                motor::math::mat3f_t orientation = motor::math::mat3f_t::make_identity();
                orientation.set_column( 0, motor::math::vec3f_t( c, s, 0.0f ) );
                orientation.set_column( 1, motor::math::vec3f_t( -s, c, 0.0f ) );
                motor::collide::obb3f_t const bounds( center, half_extents, orientation );
                volumes::obb_t const a( bounds );
                volumes::obb_t const b( center, half_extents, orientation );
                volumes::obb_t const copy( a );
                volumes::bounding_volume_cref_t base = a;

                auto const points = bounds.points();
                auto min = points[0];
                auto max = points[0];
                for( auto const & point : points )
                {
                    for( size_t axis = 0; axis != 3; ++axis )
                    {
                        min[axis] = std::min( min[axis], point[axis] );
                        max[axis] = std::max( max[axis], point[axis] );
                    }
                }

                all_match = all_match && base.get_volume_type() == volumes::volume_type::obb &&
                    base.get_volume_type() == volumes::obb_t::get_volume_type_static() &&
                    matches( a, min, max ) && matches( b, min, max ) && matches( copy, min, max ) &&
                    near( a.get_bounds().origin(), center ) && near( a.get_bounds().scaling(), half_extents );
            }
            check( "OBB types, constructors and enclosures agree with eight corners for 32 rotations", all_match );
        }
    }
    motor::log::global_t::deinit();
    motor::memory::global_t::dump_to_std();
    return passed ? 0 : 1;
}
