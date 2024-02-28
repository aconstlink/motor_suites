

#include <motor/platform/global.h>

#include <motor/platform/audio/audio_capture_helper.h>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>
#include <motor/concurrent/parallel_for.hpp>

#include <future>

int main( int argc, char ** argv )
{
    using namespace motor::core::types ;

    motor::vector< float_t > samples ;

    motor::platform::audio_capture_helper_mtr_t hlp = 
        motor::platform::audio_capture_helper_t::create() ;

    if( hlp == nullptr ) return 1 ;

    hlp->init( motor::audio::channels::mono, motor::audio::frequency::freq_48k ) ;

    {
        hlp->start() ;

        typedef std::chrono::high_resolution_clock __clock_t ;
        __clock_t::time_point tp = __clock_t::now() ;

        while( std::chrono::duration_cast<std::chrono::seconds>( __clock_t::now() - tp ) < std::chrono::seconds( 10 ) )
        {
            if( hlp->capture( samples ) )
            {
                // print data
                {
                    float_t accum = 0.0f ;
                    for( auto const s : samples ) accum += s ;

                    motor::log::global_t::status( motor::to_string(accum / float_t(samples.size()))  ) ;
                }
                samples.clear() ;
            }
        }

        hlp->stop() ;
    }

    motor::memory::release_ptr( hlp ) ;
    motor::memory::global_t::dump_to_std() ;

    return 0 ;
}