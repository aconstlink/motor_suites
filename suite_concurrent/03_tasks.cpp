
#include <motor/concurrent/task/loose_thread_scheduler.hpp>
#include <motor/log/global.h>


using namespace motor::core::types ;

int main( int argc, char ** argv )
{
    bool_t run_loop = true ;

    motor::concurrent::task_mtr_t t0 = motor::memory::create( motor::concurrent::task_t( [=]( motor::concurrent::task_mtr_t )
    {
        motor::log::global_t::status( "t0" ) ;
    } ), "t0 task" ) ;
    

    motor::concurrent::task_mtr_t t1 = motor::memory::create( motor::concurrent::task_t( [=]( motor::concurrent::task_mtr_t )
    {
        motor::log::global_t::status( "t1" ) ;
    } ), "t1 task" ) ;

    motor::concurrent::task_mtr_t t2 = motor::memory::create( motor::concurrent::task_t( [=]( motor::concurrent::task_mtr_t )
    {
        motor::log::global_t::status( "t2" ) ;
    } ), "t2 task" ) ;

    motor::concurrent::task_mtr_t t3 = motor::memory::create( motor::concurrent::task_t( [&]( motor::concurrent::task_mtr_t )
    {
        run_loop = false ;
        motor::log::global_t::status( "good bye" ) ;
    } ), "t3 task" ) ;

    motor::concurrent::task_mtr_t t4 = motor::memory::create( motor::concurrent::task_t( [=]( motor::concurrent::task_mtr_t )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds(100) ) ;
        motor::log::global_t::status( "t4" ) ;
    } ), "t4 task" ) ;

    motor::concurrent::task_mtr_t t5 = motor::memory::create( motor::concurrent::task_t( [=]( motor::concurrent::task_mtr_t )
    {
        motor::log::global_t::status( "t5" ) ;
    } ), "t5 task" ) ;

    // build graph
    {
        t0->then( motor::move( t1 ) )->then( motor::move( t2 ) )->then( motor::move( t3 ) ) ;
        t0->in_between( motor::move( t4 ) ) ;
        t0->in_between( motor::move( t5 ) ) ;
    }

    motor::memory::global_t::dump_to_std() ;

    {
        motor::concurrent::loose_thread_scheduler lts ;
        lts.init() ;

        lts.schedule( motor::move( t0 ) ) ;

        while( run_loop )
        {
            lts.update() ;
            std::this_thread::sleep_for( std::chrono::milliseconds(100) ) ;
        }

        lts.deinit() ;
    }

    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;
    return 0 ;
}
