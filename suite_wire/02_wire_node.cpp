

#include <motor/wire/node/node.h>
#include <motor/log/global.h>

#include <motor/concurrent/task/loose_thread_scheduler.hpp>
#include <motor/concurrent/global.h>

using namespace motor::core::types ;

int main( int argc, char ** argv )
{
    motor::concurrent::loose_thread_scheduler lts ;
    lts.init() ;

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
    //         .-(a)-.-----.-(d)-.
    // (start)-|     |-(c)-'     |
    //         '-(b)-'-----------'-(e)
    
    {
        start->then( motor::share( a ) )->then( motor::share( c ) )->then( motor::share( d ) )->then( motor::share( e ) ) ;
        start->then( motor::share( b ) )->then( motor::share( c ) ) ;
        b->then( motor::share( e ) ) ;
    }

    // #execute round 1
    {
        lts.schedule( start->get_task() ) ;

        while ( run_loop )
        {
            lts.update() ;
        }
    }

    // #disconnect c from the graph
    {
        c->disconnect() ;
    }

    // #execute round 2
    {
        lts.schedule( start->get_task() ) ;

        while ( run_loop )
        {
            lts.update() ;
        }
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

    lts.deinit() ;

    motor::concurrent::global::deinit() ;
    motor::log::global::deinit() ;
    motor::memory::global_t::dump_to_std() ;
    return 0 ;
}