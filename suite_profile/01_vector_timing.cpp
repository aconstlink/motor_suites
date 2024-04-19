
#include <motor/core/macros/typedef.h>

#include <motor/memory/global.h>
#include <motor/std/vector>
#include <motor/log/global.h>

#include <motor/math/vector/vector2.hpp>
#include <motor/math/vector/vector3.hpp>
#include <motor/math/vector/vector4.hpp>

#include <chrono>

namespace this_file
{
    using namespace motor::core::types ;

    template< typename T >
    class vector_pod
    {
        motor_this_typedefs( vector_pod< T > ) ;

        size_t _capacity = 0 ;
        size_t _size = 0 ;

        T * _ptr = nullptr ;

    public: 

        vector_pod( void_t ) noexcept{}

        vector_pod( size_t const num_elems ) noexcept
        {
            _ptr = motor::memory::global_t::alloc_raw< T >( num_elems ) ;
        }

        vector_pod( this_cref_t rhv ) noexcept
        {
            _capacity = rhv._capacity ;
            _size = rhv._size ;
        }

        vector_pod( this_rref_t rhv ) noexcept
        {
            _capacity = rhv._capacity ;
            _size = rhv._size ;

            rhv._capacity = 0 ;
            rhv._size = 0 ;
        }

        size_t size( void_t ) const noexcept
        {
            return _size ;
        }

        size_t capacity( void_t ) const noexcept
        {
            return _capacity ;
        }

        void_t clear( void_t ) noexcept
        {
            _size = 0 ;
        }

        void_t resize( size_t const num_elems ) noexcept
        {
            if ( _capacity < num_elems )
            {
                motor::memory::global::dealloc_raw( _ptr ) ;
                _ptr = motor::memory::global_t::alloc_raw<T>( num_elems ) ;
                _capacity = num_elems ;
            }
            _size = num_elems ;
        }

        void_t resize_by( size_t const num_elems ) noexcept
        {
            this_t::resize( _size + num_elems ) ;
        }

        T const & operator [] ( size_t const i ) const noexcept
        {
            assert( i < _capacity ) ;
            return _ptr[ i ] ;
        }

        T & operator [] ( size_t const i ) noexcept
        {
            assert( i < _capacity ) ;
            return _ptr[ i ] ;
        }
    };

    static size_t ctor_calls = 0  ;
    static size_t dtor_calls = 0 ;

    struct test_class
    {
        motor::math::vec2f_t vec2 ;
        motor::math::vec3f_t vec3 ;
        motor::math::vec4f_t vec4 ;

        bool_t flag = false ;

        #if 1
        test_class( void_t ) noexcept 
        {
            ++ctor_calls ;
        }

        ~test_class( void_t ) noexcept
        {
            ++dtor_calls ;
        }
        
        test_class( test_class const & rhv ) noexcept
        {
            vec2 = rhv.vec2 ;
            vec3 = rhv.vec3 ;
            vec4 = rhv.vec4 ;
            flag = rhv.flag ;
        }

        test_class( test_class && rhv ) noexcept
        {
            vec2 = std::move( rhv.vec2 ) ;
            vec3 = std::move( rhv.vec3 ) ;
            vec4 = std::move( rhv.vec4 ) ;
            flag = rhv.flag ;
        }

        

        test_class & operator = ( test_class && rhv ) noexcept
        {
            vec2 = std::move( rhv.vec2 ) ;
            vec3 = std::move( rhv.vec3 ) ;
            vec4 = std::move( rhv.vec4 ) ;
            flag = rhv.flag ;
            return *this ;
        }
        #endif
        
    };
}

#define WITH_ASSIGNMENT 0

std::mutex mtx ;

int main( int argc, char ** argv )
{
    using __clock_t = std::chrono::high_resolution_clock ;
    
    // vector
    motor::log::global::status( "************* Test std::vector *************" ) ;
    {
        std::lock_guard< std::mutex > lk( mtx ) ;

        auto tp = __clock_t::now() ;

        motor::vector< this_file::test_class > vec( 100000 ) ;

        // test 1
        {
            vec.clear() ;
            vec.resize( 5000 ) ;
            vec.clear() ;
            vec.resize( 100000 ) ;
            vec.clear() ;
            vec.resize( 130000 ) ;
        }

        {
            size_t const micro = std::chrono::duration_cast<std::chrono::microseconds>( __clock_t::now() - tp ).count() ;
            motor::log::global_t::status( "[std::vector] : " + motor::to_string( micro ) + " microsecs" ) ;

            motor::log::global::status( "Test 1 : ctor calls : " + motor::to_string( this_file::ctor_calls ) ) ;
            motor::log::global::status( "Test 1 : dtor calls : " + motor::to_string( this_file::dtor_calls ) ) ;

            motor::log::global::status( "+++++++++++++++++++++++++++++++++++++++++++++++++" ) ;
        }

        // test 2
        {
            tp = __clock_t::now() ;

            vec.resize( 5000 ) ;
            for ( size_t i = 0; i < vec.size(); ++i )
            {
                #if WITH_ASSIGNMENT
                vec[ i ] = this_file::test_class
                {
                    motor::math::vec2f_t(),
                    motor::math::vec3f_t(),
                    motor::math::vec4f_t(),
                    true
                } ;
                #else
                vec[ i ].vec2 = motor::math::vec2f_t() ;
                vec[ i ].vec3 = motor::math::vec3f_t() ;
                vec[ i ].vec4 = motor::math::vec4f_t() ;
                vec[ i ].flag = true ;
                #endif
            }
        }

        {
            size_t const micro = std::chrono::duration_cast<std::chrono::microseconds>( __clock_t::now() - tp ).count() ;
            motor::log::global_t::status( "[std::vector] : " + motor::to_string( micro ) + " microsecs" ) ;

            motor::log::global::status( "Test 2 : ctor calls : " + motor::to_string( this_file::ctor_calls ) ) ;
            motor::log::global::status( "Test 2 : dtor calls : " + motor::to_string( this_file::dtor_calls ) ) ;

            motor::log::global::status( "+++++++++++++++++++++++++++++++++++++++++++++++++" ) ;
        }

        // would be 
        size_t const cur_pos = vec.size() ;

        // test 3
        {
            tp = __clock_t::now() ;

            vec.resize( 130000 ) ;
            for ( size_t i = 0; i < vec.size(); ++i )
            {
                #if WITH_ASSIGNMENT
                vec[ i ] = this_file::test_class
                {
                    motor::math::vec2f_t(),
                    motor::math::vec3f_t(),
                    motor::math::vec4f_t(),
                    true
                } ;
                #else
                vec[ i ].vec2 = motor::math::vec2f_t() ;
                vec[ i ].vec3 = motor::math::vec3f_t() ;
                vec[ i ].vec4 = motor::math::vec4f_t() ;
                vec[ i ].flag = true ;
                #endif
            }
        }

        {
            size_t const micro = std::chrono::duration_cast<std::chrono::microseconds>( __clock_t::now() - tp ).count() ;
            motor::log::global_t::status( "[std::vector] : " + motor::to_string( micro ) + " microsecs" ) ;

            motor::log::global::status( "Test 3 : ctor calls : " + motor::to_string( this_file::ctor_calls ) ) ;
            motor::log::global::status( "Test 3 : dtor calls : " + motor::to_string( this_file::dtor_calls ) ) ;
            
            motor::log::global::status( "+++++++++++++++++++++++++++++++++++++++++++++++++" ) ;
            motor::log::global::status( "+++++++++++++++++++++++++++++++++++++++++++++++++" ) ;
            tp = __clock_t::now() ;
        }
    }

    this_file::ctor_calls = 0 ;
    this_file::dtor_calls = 0 ;

    // custom vector
    motor::log::global::status( "************* Test vector_pod *************" ) ;
    {
        std::lock_guard< std::mutex > lk( mtx ) ;

        auto tp = __clock_t::now() ;

        this_file::vector_pod< this_file::test_class > vec( 100000 ) ;
        
        // test 1
        {
            vec.clear() ;
            vec.resize( 5000 ) ;
            vec.clear() ;
            vec.resize( 100000 ) ;
            vec.clear() ;
            vec.resize( 130000 ) ;
        }

        {
            size_t const micro = std::chrono::duration_cast<std::chrono::microseconds>( __clock_t::now() - tp ).count() ;
            motor::log::global_t::status( "[std::vector] : " + motor::to_string( micro ) + " microsecs" ) ;

            motor::log::global::status( "Test 1 : ctor calls : " + motor::to_string( this_file::ctor_calls ) ) ;
            motor::log::global::status( "Test 1 : dtor calls : " + motor::to_string( this_file::dtor_calls ) ) ;

            motor::log::global::status( "+++++++++++++++++++++++++++++++++++++++++++++++++" ) ;
        }

        // test 2
        {
            tp = __clock_t::now() ;

            vec.resize( 5000 ) ;
            for ( size_t i = 0; i < vec.size(); ++i )
            {
                #if WITH_ASSIGNMENT
                vec[ i ] = this_file::test_class
                {
                    motor::math::vec2f_t(),
                    motor::math::vec3f_t(),
                    motor::math::vec4f_t(),
                    true
                } ;
                #else
                vec[ i ].vec2 = motor::math::vec2f_t() ;
                vec[ i ].vec3 = motor::math::vec3f_t() ;
                vec[ i ].vec4 = motor::math::vec4f_t() ;
                vec[ i ].flag = true ;
                #endif
            }
        }

        motor::log::global::status( "ctor calls : " + motor::to_string( this_file::ctor_calls ) ) ;
        motor::log::global::status( "dtor calls : " + motor::to_string( this_file::dtor_calls ) ) ;

        // would be
        size_t const cur_pos = vec.size() ;

        // test 2
        {
            vec.resize( 130000 ) ;

            for ( size_t i = 0; i < vec.size(); ++i )
            {
                #if WITH_ASSIGNMENT
                vec[ i ] = this_file::test_class
                {
                    motor::math::vec2f_t(),
                    motor::math::vec3f_t(),
                    motor::math::vec4f_t(),
                    true
                } ;
                #else
                vec[ i ].vec2 = motor::math::vec2f_t() ;
                vec[ i ].vec3 = motor::math::vec3f_t() ;
                vec[ i ].vec4 = motor::math::vec4f_t() ;
                vec[ i ].flag = true ;
                #endif
            }
        }

        size_t const micro = std::chrono::duration_cast<std::chrono::microseconds>( __clock_t::now() - tp ).count() ;
        motor::log::global_t::status( "[std::vector] : " + motor::to_string( micro ) + " microsecs" ) ;

        motor::log::global::status( "ctor calls : " + motor::to_string( this_file::ctor_calls ) ) ;
        motor::log::global::status( "dtor calls : " + motor::to_string( this_file::dtor_calls ) ) ;
    }
}