
#include <motor/memory/global.h>
#include <motor/log/global.h>

#include <motor/math/vector/vector2.hpp>

#include <motor/std/string>
#include <motor/std/vector>

#include <motor/memory/global.h>

//
// This is a test for double dispatching in a single global object
// it resolves the run-time type of a caller and calls functions based on the
// run-time type of a callee.
//
#define dd_id_vfn() \
    public:\
        virtual size_t id( void_t ) const noexcept = 0 ; \
    private:

#define dd_id_fn()                               \
    private:                                    \
        static size_t _id ;  \
    public:                                     \
        virtual size_t id( void_t ) const noexcept final { return sid() ; }                     \
        static size_t sid( void_t ) noexcept { return _id ; } \
        static void_t assign_id( size_t const id_ ) noexcept { _id = id_ ; }    \
    private:

#define dd_id_init( class_name ) size_t class_name::_id = size_t(-1) ;

namespace double_dispatch
{
    template< class icaller_t, class icallee_t, typename funk_set_t >
    class double_dispatcher
    {
    private:

        size_t _caller_id = size_t(-1) ;
        size_t _callee_id = size_t(-1) ;

        std::vector< std::vector< funk_set_t > > _table ;

    public:

        template< class caller_t, class callee_t >
        void register_funk_set( funk_set_t fs ) noexcept
        {
            if ( caller_t::sid() == size_t( -1 ) )
            {
                caller_t::assign_id( ++_caller_id ) ;
                _table.resize( _caller_id + 1 ) ;
            }

            if( callee_t::sid() == size_t(-1) )
            {
                callee_t::assign_id( ++_callee_id ) ;
                _table[ caller_t::sid() ].resize( _callee_id + 1 ) ;
            }

            _table[caller_t::sid()][callee_t::sid()] = fs ;
        }

        funk_set_t resolve( icaller_t * c1, icallee_t * c2 ) const noexcept
        {
            size_t const caller_id = c1->id() ;
            size_t const callee_id = c2->id() ;

            assert( caller_id < _table.size() ) ;
            assert( callee_id < _table[caller_id].size() ) ;

            return _table[caller_id][callee_id] ;
        }
    };
}

namespace this_file
{
    class caller_base ;
    class callee_base ;

    struct funk_types
    {
        using funk_a_t = std::function< void ( caller_base * ) > ;
        using funk_b_t = std::function< bool ( caller_base *, int const ) > ;
        using funk_c_t = std::function< bool ( caller_base *, callee_base *, bool const ) > ;

        funk_a_t _funk_a = [=] ( caller_base * ) {} ;
        funk_b_t _funk_b = [=] ( caller_base *, int const i ) { return i < 10 ; } ;
        funk_c_t _funk_c = [=] ( caller_base *, callee_base *, bool const ) { return false ; } ;

    };
}

namespace this_file
{
    using namespace motor::core::types ;

    class callee_base ;
    class callee ;
    
    class caller_base
    {
        dd_id_vfn() ;

    public:

        virtual ~caller_base( void_t ) noexcept {}

    };

    class callee_base
    {
        dd_id_vfn() ;
    };

    class callee : public callee_base
    {
        dd_id_fn() ;

    public:

        std::string name( void_t ) const noexcept { return "callee" ; }
    };
    dd_id_init( this_file::callee ) ;


    

    class caller : public caller_base
    {
    private: // private dd stuff

        dd_id_fn() ;

    public:

        void_t funk_a( void ) noexcept 
        {
            std::cout << "i am double dispatched! funk_a" << std::endl ;
        }

        bool funk_b( int const i ) noexcept
        {
            std::cout << "i am double dispatched! funk_b" << std::endl ;return i < 10 ;
        }

        bool funk_c( callee * ptr, bool const b ) noexcept
        {
            std::cout << "i am double dispatched! funk_c" << std::endl ;return false ;
        }
    };
    dd_id_init( this_file::caller ) ;
}

int main( int argc, char ** argv )
{
    double_dispatch::double_dispatcher< this_file::caller_base, this_file::callee_base, this_file::funk_types > dd ;

    // register dispatch conversion functions
    // convert from base class to concrete class and call functions 
    {
        dd.register_funk_set< this_file::caller, this_file::callee >(
        { 
            [=]( this_file::caller_base * ptr ){ dynamic_cast<this_file::caller*>(ptr)->funk_a() ; }, 
            [=]( this_file::caller_base * ptr, int const i ){ return dynamic_cast<this_file::caller*>(ptr)->funk_b(i); },
            [=]( this_file::caller_base * ptr, this_file::callee_base * other, bool const b )
                { return dynamic_cast<this_file::caller*>(ptr)->funk_c( dynamic_cast<this_file::callee*>(other), b ) ; }
        }) ;
    }

    this_file::caller_base * a ;
    this_file::callee_base * b ;

    {
        a = new this_file::caller() ;
        b = new this_file::callee() ;
    }

    {
        dd.resolve( a, b )._funk_a( a ) ;
        dd.resolve( a, b )._funk_b( a, 10 ) ;
        dd.resolve( a, b )._funk_c( a, b, true ) ;
    }

    return 0 ;
}
