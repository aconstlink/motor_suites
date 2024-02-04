
#include <motor/concurrent/global.h>
#include <motor/log/global.h>


using namespace motor::core::types ;

// 
// testing the loose tasks. This is a task that 
// run on a rouge(outside thread pool) thread of
// execution.
//
int main( int argc, char ** argv )
{
    bool_t run_loop = true ;

    motor::concurrent::task_mtr_t t0 = motor::memory::create_ptr(  motor::concurrent::task_t( [=]( motor::concurrent::task_mtr_t )
    {
        motor::log::global_t::status( "t0" ) ;
    } ) ) ;

    motor::concurrent::task_mtr_t t1 = motor::memory::create_ptr( motor::concurrent::task_t( [=]( motor::concurrent::task_mtr_t )
    {
        motor::log::global_t::status( "t1" ) ;
    } ) ) ;

    motor::concurrent::task_mtr_t t2 = motor::memory::create_ptr(   motor::concurrent::task_t( [=]( motor::concurrent::task_mtr_t )
    {
        motor::log::global_t::status( "t2" ) ;
    } ) );

    motor::concurrent::task_mtr_t t3 = motor::memory::create_ptr(   motor::concurrent::task_t( [&]( motor::concurrent::task_mtr_t )
    {
        run_loop = false ;
        motor::log::global_t::status( "good bye" ) ;
    } ) );

    motor::concurrent::task_mtr_t t4 = motor::memory::create_ptr(  motor::concurrent::task_t( [=]( motor::concurrent::task_mtr_t )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds(100) ) ;
        motor::log::global_t::status( "t4" ) ;
    } ) );

    motor::concurrent::task_mtr_t t5 = motor::memory::create_ptr(  motor::concurrent::task_t( [=]( motor::concurrent::task_mtr_t )
    {
        motor::log::global_t::status( "t5" ) ;
    } ) ) ;

    // build graph
    {
        t0->then( motor::move(t1) )->then( motor::move(t2) )->then( motor::move(t3) ) ;
        t0->in_between( motor::move(t4) ) ;
        t0->in_between( motor::move(t5) ) ;
    }

    motor::concurrent::global_t::schedule( motor::move( t0 ), motor::concurrent::schedule_type::loose ) ;

    while( run_loop )
    {
        motor::concurrent::global_t::update() ;
        std::this_thread::sleep_for( std::chrono::milliseconds(100) ) ;
    }

    motor::concurrent::global_t::deinit() ;
    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;

    return 0 ;
}
