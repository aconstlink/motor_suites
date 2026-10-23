
#include <motor/core/id_registry.hpp>

#include <motor/memory/global.h>
#include <motor/log/global.h>

using namespace motor::core::types ;

int main( int argc, char ** argv )
{
    motor::core::id_registry< 0, 10, false > tidm_0;
    motor::core::id_registry< 10, uint64_t(-1), false > tidm_1;

    bool_t res = true ;

    {
        uint64_t const id0 = tidm_0.register_type( "myclass" );
        uint64_t const id1 = tidm_0.register_type( "myclass" );

        res = motor::log::global_t::test( "[t0] : type ids should be the same", id0 == id1 ) && res ;
    }

    {
        uint64_t const id0 = tidm_0.register_type( "myclass" );
        uint64_t const id1 = tidm_0.register_type( "myclass2" );

        res = motor::log::global_t::test( "[t1] : type ids should not be the same", id0 != id1 ) && res ;
    }

    // testing the same name on two different registries.
    // the second registry starts at another index
    {
        uint64_t const id0 = tidm_0.register_type( "myclass" );
        uint64_t const id1 = tidm_1.register_type( "myclass" );

        res = motor::log::global_t::test( "[t2] : type ids should not be the same", id0 != id1 ) && res;

        res = motor::log::global_t::test( "[t3] : second id should be 10", id1 == 10 ) && res ;
    }
    return res ? 0 : 1;
}
