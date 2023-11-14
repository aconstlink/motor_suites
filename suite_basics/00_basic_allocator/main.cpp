
#include <motor/memory/global.h>
#include <motor/log/global.h>

#include <motor/std/string>
#include <motor/std/vector>

namespace this_file
{
    using namespace motor::core::types ;
    
    template< typename T >
    class allocator
    {
        typedef allocator< T > __this_t ;
        motor_this_typedefs( __this_t ) ;

    public:

        typedef T value_type ;
        typedef T* pointer ;
        typedef T& reference ;
        typedef const T* const_pointer ;
        typedef const T& const_reference ;
        typedef size_t size_type ;
        typedef ptrdiff_t difference_type ;

        template <class U> struct rebind { typedef allocator<U> other; };

        char_cptr_t _purpose = "[motor::memory] : allocator" ;

    public:

        allocator( void_t ) throw( ) {}
        allocator( this_cref_t rhv ) throw( ) : _purpose( rhv._purpose ) {}
        allocator( this_rref_t rhv ) throw( ) : _purpose( std::move( rhv._purpose ) ) {}
        allocator( char_cptr_t purpose ) throw( ) : _purpose( purpose ) {}

        template< typename U >
        allocator( allocator<U> const& rhv ) throw( ) : _purpose( rhv._purpose ) {}

        ~allocator( void_t ) {}

    public:

        bool_t operator == ( allocator const& rhv ) const
        {
            return _purpose == rhv._purpose ;
        }

        bool_t operator != ( allocator const& rhv ) const
        {
            return _purpose != rhv._purpose ;
        }

        template <class U>     
        this_ref_t operator=( allocator<U> const &) throw()
        {
            _purpose = rhv._purpose ;
            return *this ;
        }


        this_ref_t operator = ( this_cref_t rhv ) throw()
        {
            _purpose = rhv._purpose ;
            return *this ;
        }

        this_ref_t operator = ( this_rref_t rhv ) throw()
        {
            _purpose = std::move( rhv._purpose ) ;
            return *this ;
        }

    public:

        #if 0
        pointer address( reference x ) const
        {
            mem_t::address( x ) ;
        }

        const_pointer address( const_reference x ) const
        {
            mem_t::address( x ) ;
        }
        #endif

        pointer allocate( size_type n/*, void_ptr_t hint = nullptr*/ )
        {
            std::cout << "alloc " << _purpose << std::endl ;
            return static_cast<T*>(std::malloc(n * sizeof(T))) ;
        }

        void_t deallocate( pointer ptr, size_type /*n*/ )
        {
            std::cout << "dealloc " << _purpose << std::endl ;
            std::free(ptr);
        }

        size_type max_size( void_t ) const throw( )
        {
            return size_type( -1 ) ;
        }

            
        void_t construct( pointer p, const_reference val )
        {
            new( p )T( val ) ;
        }
        #if 0
        void_t destroy( pointer p )
        {
            ( *p ).~T() ;
        }
        #endif
    };

    using string_t = std::basic_string< char, std::char_traits<char>, allocator< char > > ;
    
    template< typename T >
    using vector = std::vector< T, allocator<T> > ;

    struct my_data
    {
        int i ;
        string_t msg ;
    };

    using data_vector_t = vector< my_data > ;

    class some_class
    {
        data_vector_t _data ;

    public:

        some_class( void_t ) : _data( data_vector_t::allocator_type("some_class data") )
        {
            _data.resize(5) ;
        }
    };
}


int main( int argc, char ** argv )
{
    {
        this_file::some_class sc ;
    }
    #if 0
    {
        this_file::data_vector_t my_array( this_file::data_vector_t::allocator_type("my purpose") ) ;
        my_array.resize( 2 ) ;

        {
            this_file::my_data d ;
            d.msg = "some message" ;
            my_array[0] = d ;
        }
    }
    #endif
    return 0 ;
}
