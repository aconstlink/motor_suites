

#include <motor/noise/method/gradient_noise.h>
#include <motor/noise/method/fbm.hpp>

#include <motor/math/vector/vector2.hpp>

#include <motor/log/global.h>

using namespace motor::core::types ;

int main( int argc, char ** argv )
{
    // test 1d -> 1d
    {
        uint_t const seed = 127436 ;
        uint_t const bit = 8 ;
        uint_t const mixes = 3 ;

        using noise_method_t = motor::noise::gradient_noise_t ;
        using fbm_t = motor::noise::fbm< noise_method_t > ;

        noise_method_t noise_method( seed, bit, mixes ) ;

        motor::log::global_t::status("fbm gradient noise 1d: ") ;
        motor::log::global_t::status("seed : " + motor::to_string( seed ) ) ;
        motor::log::global_t::status("bit : " + motor::to_string( bit ) ) ;
        motor::log::global_t::status("mixes : " + motor::to_string( mixes ) ) ;

        float_t const h = 0.4f ;
        float_t const lacunarity = 0.4f ;
        float_t const octaves = 3.1345f ;

        motor::string_t entries ;
        size_t i = 0 ;
        for( float_t x=-5.0f; x<5.0f; x+= 0.1f )
        {
            if( ++i % 8 == 0  ) entries += "\n" ;
            entries += motor::to_string( fbm_t::noise( x, h, lacunarity, octaves, noise_method  ) ) + " ";
        }

        motor::log::global_t::status( entries ) ;
    }

    // test 2d -> 1d
    {
        uint_t const seed = 866456 ;
        uint_t const bit = 8 ;
        uint_t const mixes = 3 ;

        using noise_method_t = motor::noise::gradient_noise_t ;
        using fbm_t = motor::noise::fbm< noise_method_t > ;

        noise_method_t noise_method( seed, bit, mixes ) ;

        motor::log::global_t::status("fbm gradient noise 2d: ") ;
        motor::log::global_t::status("seed : " + motor::to_string( seed ) ) ;
        motor::log::global_t::status("bit : " + motor::to_string( bit ) ) ;
        motor::log::global_t::status("mixes : " + motor::to_string( mixes ) ) ;

        float_t const h = 0.4f ;
        float_t const lacunarity = 0.4f ;
        float_t const octaves = 3.1345f ;

        motor::string_t entries ;
        size_t i = 0 ;
        for( float_t x=-5.0f; x<5.0f; x+= 0.1f )
        {
            for( float_t y=-5.0f; y<5.0f; y+= 0.1f )
            {
                if( ++i % 8 == 0  ) entries += "\n" ;
                entries += motor::to_string( fbm_t::noise( motor::math::vec2f_t(x, y), 
                    h, lacunarity, octaves, noise_method  ) ) + " ";
            }
        }

        motor::log::global_t::status( entries ) ;
    }
   
    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;

    return 0 ;
}
