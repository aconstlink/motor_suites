
#include <motor/concurrent/global.h>
#include <motor/concurrent/semaphore.hpp>
#include <motor/concurrent/sync_object.hpp>
#include <motor/concurrent/thread_pool.hpp>
#include <motor/log/global.h>
#include <algorithm>

//
// the thread pool is not particularly suited for 
// the engines' user. This is used within the engine
// in order to drive the task system. 
//
int main( int argc, char ** argv )
{
    using namespace motor::core::types ;

    motor::concurrent::mutex_t mtx ;
    typedef std::pair< std::thread::id, size_t > count_t ;
    motor::vector< count_t > counts( motor::vector< count_t >::allocator_type("count vector") ) ;
    
    auto inc_thread_counter = [&]( void_t )
    {
        auto const id = std::this_thread::get_id() ;

        motor::concurrent::lock_guard_t lk( mtx ) ;
        auto iter = std::find_if( counts.begin(), counts.end(), [&]( count_t const & c )
        {
            return c.first == id ;
        } ) ;
        if( iter != counts.end() ) iter->second++ ;
        else counts.emplace_back( count_t( id, 1 ) ) ;
    } ;

    
    {
        motor::concurrent::thread_pool_t tp ;

        tp.init() ;

        // add 1 task
        {
            motor::log::global_t::status("[SECTION 1] : adding one task") ;
            auto const task_funk = [&]( motor::concurrent::task_mtr_t )
            {
                inc_thread_counter() ;

                for( size_t i=0; i<100000; ++i ) 
                {
                    // busy
                }
            } ;

            tp.schedule( motor::memory::create_ptr( motor::concurrent::task_t( task_funk ) ) ) ;
        }

        tp.shutdown() ;
    }
    //motor::memory::global_t::dump_to_std() ;


    {
        motor::concurrent::thread_pool_t tp ;

        tp.init() ;

        // add n tasks
        {
            motor::log::global_t::status("[SECTION 2] : adding n task linearly") ;
            size_t const n = 30000 ;

            motor::concurrent::semaphore_t task_counter(n) ;

            auto const task_funk = [&]( motor::concurrent::task_mtr_t )
            {
                inc_thread_counter() ;

                for( size_t i=0; i<100000; ++i ) 
                {
                    // busy
                }

                task_counter.decrement() ;
            } ;

            motor::vector< motor::concurrent::task_mtr_t > tasks( n ) ;
            for( size_t i=0; i<n; ++i )
            {
                tasks[i] = motor::concurrent::global_t::make_task( task_funk ) ;
            }

            for( size_t i=0; i<n; ++i )
            {
                tp.schedule( motor::move( tasks[i] ) ) ;
            }

            // wait until semaphore == 0
            task_counter.wait(0) ;
        }

        tp.shutdown() ;
    }
    //motor::memory::global_t::dump_to_std() ;

        
    // test dynamic task insertion
    // the root task inserts n tasks during
    // it is being executed
    {
        motor::concurrent::thread_pool_t tp ;

        tp.init() ;

        motor::log::global_t::status("[SECTION 3] : adding n inbetweeners") ;

        size_t const num_in_betweens = 10000 ;
        motor::concurrent::semaphore_t sem_counter(num_in_betweens) ;

        motor::concurrent::sync_object_mtr_t so =  motor::memory::create_ptr( motor::concurrent::sync_object_t() ) ;

        motor::concurrent::task_mtr_t root = motor::concurrent::global_t::make_task( [&]( motor::concurrent::task_mtr_t this_task )
        {
            inc_thread_counter() ;

            for( size_t i=0; i<num_in_betweens; ++i ) 
            {
                this_task->in_between( motor::concurrent::global_t::make_task( [&]( motor::concurrent::task_mtr_t )
                {
                    // make it busy
                    for( size_t i=0; i<1000000; ++i ) 
                    {}

                    --sem_counter ;
                }) ) ;
            }
        });

        motor::concurrent::task_mtr_t merge = motor::concurrent::global_t::make_task( [=]( motor::concurrent::task_mtr_t )
        {
            so->set_and_signal() ;
        } );

        root->then( motor::move(merge) ) ;

        tp.schedule( motor::move(root) ) ;

        // wait until the sync object is 
        // being signaled in the merge task
        so->wait() ;

        assert( sem_counter == 0 ) ;

        motor::memory::release_ptr( so ) ;

        tp.shutdown() ;
    }
    //motor::memory::global_t::dump_to_std() ;

    // parallel for with yield and nested
    // this will serve as the prototype for the natus in-engine parallel_for
    {
        motor::concurrent::thread_pool_t tp ;

        tp.init() ;

        motor::log::global_t::status("[SECTION 4] : nested parallel for") ;
        
        motor::concurrent::semaphore_t sem_counter(1) ;
        motor::concurrent::semaphore_t task_counter ;

        size_t const num_splits = std::thread::hardware_concurrency() ;
        motor::log::global_t::status( motor::from_std( "[SECTION 4] : there should be " + std::to_string(num_splits*num_splits) + " tasks" ) ) ;

        motor::concurrent::task_mtr_t root = motor::concurrent::global_t::make_task( [&]( motor::concurrent::task_mtr_t this_task )
        {
            inc_thread_counter() ;
            
            sem_counter.increment_by(num_splits) ;

            --sem_counter ;

            // outer for: create num_splits tasks that will yield
            for( size_t i=0; i<num_splits; ++i ) 
            {
                tp.schedule( motor::concurrent::global_t::make_task( [&]( motor::concurrent::task_mtr_t )
                {
                    inc_thread_counter() ;

                    motor::concurrent::semaphore_t inner_sem(num_splits) ;

                    // inner for
                    for( size_t i=0; i<num_splits; ++i ) 
                    {
                        tp.schedule( motor::concurrent::global_t::make_task( [&]( motor::concurrent::task_mtr_t )
                        {
                            inc_thread_counter() ;

                            // make it busy
                            for( size_t i=0; i<100000000; ++i ) { }

                            --inner_sem ;
                            ++task_counter ;
                        }) ) ;
                    }

                    tp.yield( [&]( void_t )
                    {
                        return inner_sem != 0 ;
                    } ) ;

                    --sem_counter ;
                }) ) ;

                
            }

            tp.yield( [&]( void_t )
            {
                return sem_counter != 0 ;
            } ) ;
        });
        
        tp.schedule( motor::move(root) ) ;
        sem_counter.wait( 0 ) ;

        motor::log::global_t::status( motor::from_std("[SECTION 4] : tasks executed " + std::to_string(task_counter.value())) ) ;
        task_counter = 0 ;
    }

    motor::memory::global_t::dump_to_std() ;
    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;
    return 0 ;
}
