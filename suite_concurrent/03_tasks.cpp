
#include <motor/concurrent/task/loose_thread_scheduler.hpp>
#include <motor/concurrent/task/task_disconnector.hpp>
#include <motor/log/global.h>


using namespace motor::core::types ;

int main( int argc, char ** argv )
{
    bool_t run_loop = true ;

    auto t0 = motor::shared( motor::concurrent::task_t( [=]( motor::concurrent::task_t::task_funk_param_in_t )
    {
        motor::log::global_t::status( "t0" ) ;
    } ), "t0 task" ) ;
    

    auto t1 = motor::shared( motor::concurrent::task_t( [=]( motor::concurrent::task_t::task_funk_param_in_t )
    {
        motor::log::global_t::status( "t1" ) ;
    } ), "t1 task" ) ;

    auto t2 = motor::shared( motor::concurrent::task_t( [=]( motor::concurrent::task_t::task_funk_param_in_t )
    {
        motor::log::global_t::status( "t2" ) ;
    } ), "t2 task" ) ;

    auto t3 = motor::shared( motor::concurrent::task_t( [&]( motor::concurrent::task_t::task_funk_param_in_t )
    {
        run_loop = false ;
        motor::log::global_t::status( "good bye" ) ;
    } ), "t3 task" ) ;

    auto t4 = motor::shared( motor::concurrent::task_t( [=]( motor::concurrent::task_t::task_funk_param_in_t )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds(100) ) ;

        motor::log::global_t::status( "t4" ) ;
    } ), "t4 task" ) ;

    auto t5 = motor::shared( motor::concurrent::task_t( [=]( motor::concurrent::task_t::task_funk_param_in_t )
    {
        motor::log::global_t::status( "t5" ) ;
    } ), "t5 task" ) ;

    // build graph
    // (t0)-.---(t1)------.-(t2)-(t3)
    //      '-(t4)-.-(t5)-'
    {
        t0->then( motor::move( t1 ) )->then( motor::share( t2 ) )->then( motor::move( t3 ) ) ;
        t0->then( motor::move( t4 ) )->then( motor::move( t5 ) )->then( motor::move( t2 ) ) ;
    }

    motor::memory::global_t::dump_to_std() ;

    {
        motor::concurrent::loose_thread_scheduler lts ;
        lts.init() ;
        
        {
            lts.schedule( motor::share( t0 ) ) ;

            while ( run_loop )
            {
                lts.update() ;
            }
        }

        run_loop = true ;

        {
            lts.schedule( motor::share( t0 ) ) ;

            while ( run_loop )
            {
                lts.update() ;
            }
        }

        lts.deinit() ;
    }

    // it is required to disconnect all unknown or ref lost nodes which
    // are in the graph network for proper resource deallocation.
    motor::concurrent::task_disconnector_t::disconnect_everyting( motor::move( t0 ) ) ;

    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;
    return 0 ;
}
