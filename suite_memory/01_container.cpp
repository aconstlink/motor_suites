
#include <motor/memory/global.h>
#include <motor/log/global.h>

#include <motor/std/string>
#include <motor/std/vector>


int main( int argc, char ** argv )
{
    // those std objects causes the custom allocator
    // to trigger std::_Container_proxy allocations on the heap.
    // This should be fixed somehow in terms of performance.
    {
        motor::string_t test("test string") ;
        motor::vector< int > test_v ;
        motor::memory::global_t::dump_to_std() ;
    }
    motor::memory::global_t::dump_to_std() ;
    
    return 0 ;
}
