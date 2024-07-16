

#include <motor/wire/node/node.h>
#include <motor/log/global.h>

#include <motor/concurrent/task/loose_thread_scheduler.hpp>
#include <motor/concurrent/global.h>

using namespace motor::core::types ;

int main( int argc, char ** argv )
{
    bool_t run_loop = true ;

    auto start = motor::shared( motor::wire::node( [=] ( motor::wire::node_ptr_t ) 
    {
        motor::log::global::status( "start node" ) ;
    } ), "start node" ) ;

    auto a = motor::shared( motor::wire::node( [=] ( motor::wire::node_ptr_t )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds( 19 ) ) ;
        motor::log::global::status( "node a" ) ;
    } ), "node" ) ;

    auto b = motor::shared( motor::wire::node( [=] ( motor::wire::node_ptr_t )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) ) ;
        motor::log::global::status( "node b" ) ;
    } ), "node" ) ;

    auto c = motor::shared( motor::wire::node( [=] ( motor::wire::node_ptr_t )
    {
        motor::log::global::status( "node c" ) ;
    } ), "node" ) ;

    auto d = motor::shared( motor::wire::node( [=] ( motor::wire::node_ptr_t )
    {
        motor::log::global::status( "node d" ) ;
    } ), "node" ) ;

    auto e = motor::shared( motor::wire::node( [&] ( motor::wire::node_ptr_t )
    {
        run_loop = false ;
        motor::log::global::status( "node e" ) ;
    } ), "node" ) ;

    // #graph
    //         .-(a)-.
    // (start)-|     |-(c)-(d)-.
    //         '-(b)-'---------'-(e)
    #if 1
    {
        start->then( motor::share( a ) )->then( motor::share( c ) )->then( motor::share( d ) )->then( motor::share( e ) ) ;
        start->then( motor::share( b ) )->then( motor::share( c ) ) ;
        b->then( motor::share( e ) ) ;

        // make cycle 
        //e->then( motor::share( start ) ) ;
    }
    #else
    // #graph
    //         
    // (start)-(a)-(e)
    //
    {
        start->then( motor::share( a ) )->then( motor::share( e ) ) ;
    }
    #endif

    // test tier builder
    // #graph
    //         .-(a)-.
    // (start)-|     |-(c)-(d)-.
    //         '-(b)-'---------'-(e)

    // #tiers for this graph
    // tier #1 | tier #2 | tier #3 | tier #4  | tier #5 
    //  start  |  a, b   |   c     |    d     |   e
    {
        motor::concurrent::task::tier_builder_t::build_result_t br ;
        motor::concurrent::task::tier_builder_t tb ;
        tb.build( start->get_task(), br ) ;

        size_t idx = 0 ;
        for( auto t : br.tiers )
        {
            motor::log::global_t::status(
                "Tier #" + motor::to_string(idx) + " has " + motor::to_string( t.tasks.size() ) + " tasks" ) ;
        }

        if( br.has_cylce ) motor::log::global_t::status("graph has a cycle.") ;
        assert( br.has_cylce == false ) ;
    }

    // #execute
    {
        motor::concurrent::loose_thread_scheduler lts ;
        lts.init() ;

        lts.schedule( start->get_task() ) ;

        while ( run_loop )
        {
            lts.update() ;
            //std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) ) ;
        }

        lts.deinit() ;
    }

    {
        start->disconnect() ;
        a->disconnect() ;
        b->disconnect() ;
        c->disconnect() ;
        d->disconnect() ;
        e->disconnect() ;
    }
    motor::release( start ) ;
    motor::release( a ) ;
    motor::release( b ) ;
    motor::release( c ) ;
    motor::release( d ) ;
    motor::release( e ) ;

    motor::concurrent::global::deinit() ;
    motor::log::global::deinit() ;
    motor::memory::global_t::dump_to_std() ;
    return 0 ;
}