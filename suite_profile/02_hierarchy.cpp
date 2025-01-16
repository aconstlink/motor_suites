

#include <motor/profiling/global.h>
#include <motor/profiling/probe_guard.hpp>

#include <motor/std/vector>
#include <motor/log/global.h>

#include <sstream>
#include <thread>

namespace this_file
{
    using namespace motor::core::types ;

    void_t funk_child_01( void_t ) noexcept
    {
        MOTOR_PROBE( "test", "funk_child_01" ) ;
    }

    void_t funk_child_00( void_t ) noexcept
    {
        MOTOR_PROBE( "test", "funk_child_00" ) ;
        funk_child_01() ;
    }

    void_t funk_parent( void_t ) noexcept
    {
        MOTOR_PROBE( "test", "funk_parent") ;
        funk_child_00() ;
    }
}

int main( int argc, char ** argv )
{
    auto funk = [=] ( void )
    {
        std::thread::id const id = std::this_thread::get_id() ;

        {
            std::stringstream ss ;
            ss << id ;
            motor::log::global::status( "Hello from thread : " + motor::from_std( ss.str() ) ) ;
        }

        for ( size_t i = 0; i < 10; ++i )
        {
            std::this_thread::sleep_for( std::chrono::milliseconds(10) ) ;
            this_file::funk_parent() ;
        }
    } ;

    std::vector< std::thread > threads ;
    threads.emplace_back( std::thread( funk ) ) ; 
    threads.emplace_back( std::thread( funk ) ) ; 
    threads.emplace_back( std::thread( funk ) ) ; 
    threads.emplace_back( std::thread( funk ) ) ; 

    for( auto & t : threads ) t.join() ;


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