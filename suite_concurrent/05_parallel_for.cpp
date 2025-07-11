
#include <motor/concurrent/parallel_for.hpp>
#include <motor/log/global.h>
#include <algorithm>

using namespace motor::core::types ;

//
// testing the parallel_for
//
int main( int argc, char ** argv )
{
    {
        motor::concurrent::mutex_t mtx ;
        typedef std::pair< std::thread::id, size_t > count_t ;
        motor::vector< count_t > counts( motor::vector< count_t >::allocator_type( "counts" ) ) ;
        motor::concurrent::semaphore_t task_counter ;


        auto inc_thread_counter = [&] ( void_t )
        {
            auto const id = std::this_thread::get_id() ;

            motor::concurrent::lock_guard_t lk( mtx ) ;
            auto iter = std::find_if( counts.begin(), counts.end(), [&] ( count_t const & c )
            {
                return c.first == id ;
            } ) ;
            if ( iter != counts.end() ) iter->second++ ;
            else counts.emplace_back( count_t( id, 1 ) ) ;
        } ;

        motor::log::global_t::status( "About to start concurrency. Check you CPU usage! Should be at 100% !!!" ) ;

        // single parallel_for
        {
            motor::log::global_t::status( "Entering parallel_for " ) ;
            size_t const n = 10000000000 ;
            motor::concurrent::semaphore_t loop_counter ;

            motor::concurrent::parallel_for<size_t>(
                motor::concurrent::range_1d<size_t>( 0, n ),
                [&] ( motor::concurrent::range_1d<size_t> const & r )
            {
                inc_thread_counter() ;
                ++task_counter ;
                for ( size_t i = r.begin(); i < r.end(); ++i )
                {
                }
                loop_counter.increment_by( r.difference() ) ;
            } ) ;

            assert( loop_counter.value() == n ) ;
            loop_counter = 0 ;
            motor::log::global_t::status( "Leaving parallel_for" ) ;
        }
        motor::memory::global_t::dump_to_std() ;

        // nested parallel_for
        {
            motor::log::global_t::status( "Entering nested parallel_for" ) ;

            size_t const n1 = 13710 ;
            size_t const n2 = 10000 ;

            motor::concurrent::semaphore_t loop_counter ;

            motor::concurrent::parallel_for<size_t>( motor::concurrent::range_1d<size_t>( 0, n1 ),
                [&] ( motor::concurrent::range_1d<size_t> const & r0 )
            {
                inc_thread_counter() ;
                ++task_counter ;

                for ( size_t i0 = r0.begin(); i0 < r0.end(); ++i0 )
                {
                    motor::concurrent::parallel_for<size_t>(
                        motor::concurrent::range_1d<size_t>( 0, n2 ),
                        [&] ( motor::concurrent::range_1d<size_t> const & r )
                    {
                        inc_thread_counter() ;
                        ++task_counter ;
                        for ( size_t i = r.begin(); i < r.end(); ++i )
                        {
                        }
                        loop_counter.increment_by( r.difference() ) ;
                    } ) ;
                }
            } ) ;

            assert( loop_counter.value() == n1 * n2 ) ;
            loop_counter = 0 ;

            motor::log::global_t::status( "Leaving nested parallel_for" ) ;
        }
        motor::log::global_t::status( "Good Bye" ) ;
        task_counter = 0 ;
    }

    motor::memory::global_t::dump_to_std() ;
    motor::concurrent::global_t::deinit() ;
    motor::log::global_t::deinit() ;
    return motor::memory::global_t::dump_to_std() != 0 ? 1 : 0;
}
