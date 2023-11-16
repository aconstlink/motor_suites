
#include <motor/memory/global.h>
#include <motor/log/global.h>
#include <motor/concurrent/parallel_for.hpp>

#include <motor/std/string>
#include <motor/std/vector>

#include <atomic>

int main( int argc, char ** argv )
{
    {
        auto t = motor::concurrent::global_t::make_task( [&]( motor::concurrent::task_mtr_t ){} ) ;

        //motor::memory::release_ptr( t ) ;
        motor::memory::global_t::release( t ) ;
    }

    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;

    return 0 ;
}
