
#include <motor/memory/global.h>
#include <motor/log/global.h>
#include <motor/concurrent/parallel_for.hpp>

#include <motor/std/string>
#include <motor/std/vector>

#include <atomic>

int main( int argc, char ** argv )
{

    {
        size_t o =0;
        std::atomic_size_t ia = 0 ;

        motor::concurrent::parallel_for<size_t>( motor::concurrent::range_1d<size_t>( 100000 ), 
            [&]( motor::concurrent::range_1d< size_t > const & r )
            {
                for( size_t i=r.begin(); i<r.end(); ++i )
                {
                    ++o ;
                    ++ia ;
                }
            } ) ;

        motor::memory::global_t::dump_to_std() ;
    }

    {
        size_t o =0;
        std::atomic_size_t ia = 0 ;

        motor::concurrent::parallel_for<size_t>( motor::concurrent::range_1d<size_t>( 100000 ), 
            [&]( motor::concurrent::range_1d< size_t > const & r )
            {
                for( size_t i=r.begin(); i<r.end(); ++i )
                {
                    ++o ;
                    ++ia ;
                }
            } ) ;

        motor::memory::global_t::dump_to_std() ;
    }

    {
        size_t o =0;
        std::atomic_size_t ia = 0 ;

        motor::concurrent::parallel_for<size_t>( motor::concurrent::range_1d<size_t>( 100000 ), 
            [&]( motor::concurrent::range_1d< size_t > const & r )
            {
                for( size_t i=r.begin(); i<r.end(); ++i )
                {
                    ++o ;
                    ++ia ;
                }
            } ) ;

        motor::memory::global_t::dump_to_std() ;
    }

    motor::memory::global_t::dump_to_std() ;

    motor::concurrent::global_t::deinit() ;
    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;

    return 0 ;
}
