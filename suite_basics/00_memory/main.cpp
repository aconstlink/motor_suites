
#include <motor/memory/global.h>

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

        

    public:

        allocator( void_t ) throw( ) {}
        allocator( this_cref_t rhv ) throw( ) {}
        allocator( this_rref_t rhv ) throw( )  {}
        allocator( char_cptr_t purpose ) throw( )  {}

            
        template< typename U >
        allocator( allocator<U> const& rhv ) throw( ) {}
            
        ~allocator( void_t ) {}

    public:

        pointer allocate( size_type n/*, void_ptr_t hint = nullptr*/ )
        {
            std::cout << "alloc of n = " << std::to_string(n) << std::endl ;
            return static_cast<T*>( std::malloc(n * sizeof(T)) ) ;
        }

        void_t deallocate( pointer ptr, size_type n )
        {
            std::cout << "dealloc of n = " << std::to_string(n) << std::endl ;
            std::free( ptr );
        }

        #if 0
        size_type max_size( void_t ) const throw( )
        {
            return size_type( -1 ) ;
        }
        #endif
    };

    #if 0
    #define motor_used 0
    using string_t = std::basic_string< char, std::char_traits<char>, allocator< char > > ;
    
    template< typename T >
    using vector = std::vector< T, allocator<T> > ;

    #else
    #define motor_used 1
    using string_t = motor::core::string_t ;

    template< typename T >
    using vector = motor::core::vector< T > ;

    #endif
}

int main( int argc, char ** argv )
{
    {
        this_file::string_t s( "Hello" ) ;
    }
    #if motor_used
    motor::memory::global_t::dump_to_std() ;
    #endif
    {
        this_file::vector< this_file::string_t > v ;
        v.resize( 2 ) ;
    }
    #if motor_used
    motor::memory::global_t::dump_to_std() ;
    #endif
    return 0 ;
}
