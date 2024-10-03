
#include <motor/memory/global.h>
#include <motor/log/global.h>

#include <motor/math/vector/vector2.hpp>

#include <motor/std/string>
#include <motor/std/vector_pod>

#include <motor/memory/global.h>

#define make_id_vfn() \
    public:\
        virtual size_t id( void_t ) const noexcept = 0 ; \
    private:

#define make_id()                               \
    private:                                    \
        static size_t _id ;  \
    public:                                     \
        virtual size_t id( void_t ) const noexcept final { return _id ; }                     \
        static void_t assign_id( size_t const id_ ) noexcept { _id = id_ ; }    \
    private:

#define init_id( class_name, number ) size_t class_name::_id = number ;

namespace this_file
{
    using namespace motor::core::types ;

    class base
    {
        make_id_vfn() ;
    };

    class class_a : public base
    {
        make_id() ;
    };
    init_id( this_file::class_a, 0 ) ;
    

    class class_b : public base
    {
        //make_id() ;
    };
    //init_id( this_file::class_b, 1 ) ;

    class class_c : public class_b
    {
        make_id() ;
    };
    init_id( this_file::class_c, 2 ) ;
}

int main( int argc, char ** argv )
{
    // at initialization time 
    {
        this_file::class_c::assign_id( 5 ) ;
    }

    {
        this_file::base * a = new this_file::class_a() ;
        std::cout << "class_a has: " << std::to_string( a->id() ) << std::endl ;
    }
    #if 0
    {
        this_file::base * b = new this_file::class_b() ;
        std::cout << "class_b has: " << std::to_string( b->id() ) << std::endl ;
    }
    #endif
    {
        this_file::base * c = new this_file::class_c() ;
        std::cout << "class_c has: " << std::to_string( c->id() ) << std::endl ;
    }

    {
        this_file::base * c = new this_file::class_c() ;
        std::cout << "class_b from class_c has: " << std::to_string( ((this_file::class_b*)c)->id() ) << std::endl ;
    }

    return 0 ;
}
