

#include <motor/noise/method/value_noise.h>

#include <motor/log/global.h>

using namespace motor::core::types ;

int main( int argc, char ** argv )
{
    // test 1d -> 1d
    {
        uint_t const seed = 127436 ;
        uint_t const bit = 8 ;
        uint_t const mixes = 3 ;

        motor::noise::value_noise_t vn( seed, bit, mixes ) ;

        motor::log::global_t::status("value noise 1d: ") ;
        motor::log::global_t::status("seed : " + motor::to_string( seed ) ) ;
        motor::log::global_t::status("bit : " + motor::to_string( bit ) ) ;
        motor::log::global_t::status("mixes : " + motor::to_string( mixes ) ) ;

        motor::string_t entries ;
        size_t i = 0 ;
        for( float_t x=0; x<100.0f; x+= 0.1f )
        {
            if( ++i % 8 == 0  ) entries += "\n" ;
            entries += motor::to_string( vn.noise( x ) ) + " ";
        }

        motor::log::global_t::status( entries ) ;
    }

    // test 2d -> 1d
    {
        uint_t const seed = 836456 ;
        uint_t const bit = 8 ;
        uint_t const mixes = 3 ;

        motor::noise::value_noise_t vn( seed, bit, mixes ) ;

        motor::log::global_t::status("value noise 2d: ") ;
        motor::log::global_t::status("seed : " + motor::to_string( seed ) ) ;
        motor::log::global_t::status("bit : " + motor::to_string( bit ) ) ;
        motor::log::global_t::status("mixes : " + motor::to_string( mixes ) ) ;

        motor::string_t entries ;
        size_t i = 0 ;
        for( float_t x=0; x<10.0f; x+= 0.1f )
        {
            for( float_t y=0; y<10.0f; y+= 0.1f )
            {
                if( ++i % 8 == 0  ) entries += "\n" ;
                entries += motor::to_string( vn.noise( x, y ) ) + " ";
            }
        }

        motor::log::global_t::status( entries ) ;
    }
   
    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;

    return 0 ;
}
