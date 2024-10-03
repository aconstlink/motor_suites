
#include <motor/memory/global.h>
#include <motor/log/global.h>

#include <motor/math/vector/vector2.hpp>

#include <motor/std/string>
#include <motor/std/vector>

#include <motor/memory/global.h>

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

#define dd_id_init( class_name, number ) size_t class_name::_id = number ;

#define dd_static_funks( type ) static motor::vector< type > _dd_funks ;

namespace double_dispatch
{
    template< typename funk_set_t >
    class dd_caller
    {
    private:

        motor::vector< funk_set_t > _dd_funks ;

    public:

        void resize_by( size_t const num_funk_sets ) noexcept
        {
            _dd_funks.resize( _dd_funks.size() + num_funk_sets ) ;
        }

        void store_funk_set( size_t const callee_id, funk_set_t fs ) noexcept
        {
            if( _dd_funks.size() < callee_id ) _dd_funks.resize( callee_id + 10 ) ;

            _dd_funks.emplace_back( fs ) ;
        }

        funk_set_t do_dispatch( size_t const callee_id ) const noexcept
        {
            if( callee_id >= _dd_funks.size() ) return funk_set_t() ;
            return _dd_funks[callee_id] ;
        }

    };
    #define static_dd_caller( type ) static dd_caller< type > __dd_caller__ ;
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

        virtual void_t funk_a( callee_base * ptr ) noexcept = 0 ;
        virtual bool funk_b( callee_base * ptr, int const i ) noexcept = 0 ;
        virtual bool funk_c( callee_base * ptr, bool const ) noexcept = 0 ;

        
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
    dd_id_init( this_file::callee, 0 ) ;


    struct funk_types
    {
        using funk_a_t = std::function< void ( caller_base * ) > ;
        using funk_b_t = std::function< bool ( caller_base *, int const ) > ;
        using funk_c_t = std::function< bool ( caller_base *, callee_base *, bool const ) > ;

        funk_a_t _funk_a = [=] ( caller_base * ) {} ;
        funk_b_t _funk_b = [=] ( caller_base *, int const i ) { return i < 10 ; } ;
        funk_c_t _funk_c = [=] ( caller_base *, callee_base *, bool const ) { return false ; } ;

    };

    class caller : public caller_base
    {
    private: // private dd stuff

        static double_dispatch::dd_caller< funk_types > __dd_caller__ ;

    public: // public dd stuff

        template< class callee_type >
        static void dd_register_funk_set( funk_types ft ) noexcept 
        {
            __dd_caller__.store_funk_set( callee_type::sid(), ft ) ;
        }

    public:

        virtual void_t funk_a( callee_base * ptr ) noexcept 
        {
            return __dd_caller__.do_dispatch( ptr->id() )._funk_a( this ) ;
        }

        virtual bool funk_b( callee_base * ptr, int const i ) noexcept 
        {
            return __dd_caller__.do_dispatch( ptr->id() )._funk_b( this, i ) ;
        }

        virtual bool funk_c( callee_base * ptr, bool const b ) noexcept
        {
            return __dd_caller__.do_dispatch( ptr->id() )._funk_c( this, ptr, b ) ;
        }

    private:

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
    dd_id_init( this_file::caller, 0 ) ;
    double_dispatch::dd_caller< funk_types > caller::__dd_caller__ ;
}

int main( int argc, char ** argv )
{
    // register dispatch conversion functions
    // convert from base class to concrete class and call functions 
    {
        this_file::caller::dd_register_funk_set<this_file::callee>( 
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
        a->funk_a( b ) ;
        a->funk_b( b, 10 ) ;
        a->funk_c( b, true ) ;
    }

    return 0 ;
}
