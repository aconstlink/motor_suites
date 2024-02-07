
#include <motor/math/spline/linear_bezier_spline.hpp>
#include <motor/math/spline/cubic_hermit_spline.hpp>

#include <motor/math/animation/keyframe_sequence.hpp>

#include <motor/math/vector/vector3.hpp>

#include <motor/std/string>
#include <motor/log/global.h>

namespace this_file
{
    void print( motor::string_cref_t label, motor::math::vec3f_cref_t v ) noexcept
    {
        motor::log::global_t::status( label + " : ["+motor::to_string(v.x())+", "+motor::to_string(v.y())+", "+motor::to_string(v.z())+"]" ) ;
    }
}

int main( int argc, char ** argv )
{
    motor::log::global_t::status( "Testing linear bezier" ) ;

    {
        typedef motor::math::linear_bezier_spline< motor::math::vec3f_t > spline_t ;

        spline_t sp
            ( {
                motor::math::vec3f_t(0.0f,0.0f,0.0f), motor::math::vec3f_t(1.0f,0.0f,0.0f), 
                motor::math::vec3f_t(0.0f,1.0f,0.0f), motor::math::vec3f_t(1.0f,1.0f,0.0f)
            } ) ;

        float_t const step = 0.1f ;
        for( float_t f = -step; f<=(1.0f+step); f+=step )
        {
            this_file::print( "at "+motor::to_string(f), sp(f) );
        }
    }

    motor::log::global_t::status( "Testing cubic hermit keyframe sequence" ) ;

    {
        typedef motor::math::cubic_hermit_spline< motor::math::vec3f_t > spline_t ;
        typedef motor::math::keyframe_sequence< spline_t > keyframe_sequence_t ;

        keyframe_sequence_t kf ;

        kf.insert( keyframe_sequence_t::keyframe_t( 500, motor::math::vec3f_t( -1.0f, -1.0f, -1.0f ) ) ) ;
        kf.insert( keyframe_sequence_t::keyframe_t( 1000, motor::math::vec3f_t( 1.0f, 0.0f, 0.0f ) ) ) ;
        kf.insert( keyframe_sequence_t::keyframe_t( 4000, motor::math::vec3f_t( 1.0f, 1.0f, 1.0f ) ) ) ;
        kf.insert( keyframe_sequence_t::keyframe_t( 2000, motor::math::vec3f_t( 0.0f, 0.0f, 1.0f ) ) ) ;
        kf.insert( keyframe_sequence_t::keyframe_t( 3000, motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ) ) ) ;

        size_t const step = 100 ;
        for( size_t f = 0; f<=(kf.back().get_time()+step); f+=step )
        {
            this_file::print( "at "+motor::to_string(f), kf(f) );
        }
    }

    return 0 ;
}