

#include <motor/profiling/global.h>
#include <motor/std/vector>
#include <motor/log/global.h>

#include <future>

void some_funk( void )
{
    #if 0
    motor::profiling::global_t::push("some_funk") ;

    motor::profiling::global_t::pop() ;
    #endif
}

int main( int argc, char ** argv )
{
    #if 0
    {
        motor::profiling::global_t::push( "main #1" ) ;

        motor::profiling::global_t::pop() ;
    }

    {
        motor::profiling::global_t::push( "main #2" ) ;

        motor::profiling::global_t::pop() ;
    }

    {
        motor::profiling::global_t::push( "main #3" ) ;

        motor::profiling::global_t::pop() ;
    }

    {
        motor::vector< std::future<void> > asyncs ;

        motor::profiling::global_t::push( "main #4" ) ;
        for ( size_t i = 0; i < 100; ++i )
        {
            asyncs.emplace_back( std::async( std::launch::async, [=] ( void )
            {
                motor::profiling::global_t::push("async " + motor::to_string(i) ) ;
                some_funk() ;
                motor::profiling::global_t::pop() ;
            } ) ) ;
        }
        motor::profiling::global_t::pop() ;

        for ( auto & fut : asyncs ) fut.wait() ;
    }
    #endif

    #if MOTOR_PROFILING

    auto dps = motor::profiling::global_t::manager().swap_and_clear() ;
    
    std::chrono::microseconds ms(0)  ;
    for ( auto & dp : dps )
    {
        ms += std::chrono::duration_cast<std::chrono::microseconds>( dp.dur ) ;
    }

    motor::log::global_t::status( "time spend (micro seconds) : " + motor::to_string( ms.count() ) ) ;
    #endif

    return 0 ;
}