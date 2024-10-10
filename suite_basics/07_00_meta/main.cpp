
#include <motor/memory/global.h>
#include <motor/log/global.h>

#include <motor/meta/variable.hpp>
#include <motor/math/utility/3d/transformation.hpp>
#include <motor/math/vector/vector2.hpp>

#include <motor/std/string>
#include <motor/std/vector>

namespace motor
{
    namespace meta
    {
        template<>
        class nameof< motor::math::vec3f_t >
        {
        public:

            static char * const  name( void_t ) noexcept { return "vec3f" ; }
        } ;
    }
}

namespace motor
{
    namespace meta
    {
        using namespace motor::core::types ;

        template< typename T >
        class data< motor::math::vector3< T > > : public motor::meta::any
        {
            using base_t = motor::meta::any ;
            using this_t = data< motor::math::vector3< T > > ;

        public:

            using value_t = motor::math::vector3< T > ;
            using value_cref_t = value_t const & ;
            using value_ref_t = value_t & ;
            using value_ptr_t = value_t * ;
            using value_inout_t = value_ref_t ;
            using value_out_t = value_ref_t ;
            using value_in_t = value_cref_t ;

            using x_t = motor::meta::data< T > ;
            using y_t = motor::meta::data< T > ;
            using z_t = motor::meta::data< T > ;

            static inline const x_t x = x_t( "x" ) ;
            static inline const y_t y = y_t( "y" ) ;
            static inline const z_t z = z_t( "z" ) ;

        public:

            data( char const * const name ) noexcept : base_t( name ) {}

            virtual char const * const type_name( void_t ) const noexcept
            {
                return this_t::type_name_s() ;
            }

            static char const * const type_name_s( void_t ) noexcept
            {
                return motor::meta::nameof< value_t >::name() ;
            }

            using setter_t = std::function< void_t ( value_inout_t, value_in_t ) > ;
            using getter_t = std::function< void_t ( value_out_t, value_in_t ) > ;

            setter_t setter = [] ( value_inout_t a, value_in_t b ) { a = b ; } ;
            getter_t getter = [] ( value_out_t a, value_in_t b ) { a = b ; } ;

        private:

            virtual void_t for_each_member( motor::meta::any::for_each_funk_t f, motor::meta::any::for_each_info_in_t info ) const noexcept 
            {
                {
                    motor::meta::any::for_each_info_t ifo 
                    {
                        info.level, info.full_name + "." + motor::string_t( x.name() )
                    };
                    
                    f( x, ifo ) ; 
                    
                    //ifo.level = ifo.level + 1  ;
                    //x.for_each_member( f, l + 1 ) ;
                }
                
                {
                    motor::meta::any::for_each_info_t ifo
                    {
                        info.level, info.full_name + "." + motor::string_t( y.name() )
                    };

                    f( y, ifo ) ;

                    //ifo.level = ifo.level + 1  ;
                    //y.for_each_member( f, l + 1 ) ;
                }
                
                
                {
                    motor::meta::any::for_each_info_t ifo
                    {
                        info.level, info.full_name + "." + motor::string_t( z.name() )
                    };

                    f( z, ifo ) ;

                    //ifo.level = ifo.level + 1  ;
                    //z.for_each_member( f, l + 1 ) ;
                }
            }
        };

        using vec3im_t = data< motor::math::vec3i_t > ;
        using vec3uim_t = data< motor::math::vec3ui_t > ;
        using vec3dm_t = data< motor::math::vec3d_t > ;
        using vec3fm_t = data< motor::math::vec3f_t > ;
    }
}

namespace this_file
{
    class complex_class
    {
    public:

        motor::math::vec3f_t my_vec ;
        //this_file::math::meta::vec3fm_t my_vec_meta ;

    };
}

int main( int argc, char ** argv )
{
    #if 0
    motor::math::m3d::trafof_t orig_trafo ;
    this_file::math::var::trafof_t my_trafo ;
    #endif


    {
        motor::meta::vec3fm_t v3("pos") ;

        
        motor::log::global_t::status("-------------------------") ;
        motor::log::global_t::status("vec3f_t meta has:") ;
        motor::log::global_t::status("var name: " + motor::string_t( v3.name() ) ) ;
        motor::log::global_t::status("is of type: " + motor::string_t( v3.type_name() ) ) ;
        motor::log::global_t::status("vec3f_t has sub-variables: " ) ;
        motor::log::global_t::status("var1: " + motor::string_t( v3.x.type_name() ) + " " + motor::string_t( v3.x.name() ) ) ;
        motor::log::global_t::status("var2: " + motor::string_t( v3.y.type_name() ) + " " + motor::string_t( v3.y.name() ) ) ;
        motor::log::global_t::status("var3: " + motor::string_t( v3.z.type_name() ) + " " + motor::string_t( v3.x.name() ) ) ;
        motor::log::global_t::status("*************************") ;

        //this_file::math::var::vec3fm_t::x_t::type_t x ;

        int bp = 0 ;
    }

    {
        motor::meta::vec3fm_t v3("axis") ;
        motor::meta::floatm_t f("angle") ;
    }

    //
    {
       
    }

    // test for each sub component
    {
        motor::log::global_t::status( "testing for each" ) ;

        motor::meta::vec3fm_t v3("pos") ;

        v3.start( [&]( motor::meta::any_in_t comp, motor::meta::any::for_each_info_in_t ifo )
        {
            
            {
                motor::string_t const outp = motor::string_t( comp.type_name() ) + " : " +
                    motor::string_t( ifo.full_name ) ;

                motor::log::global_t::status( outp ) ;
            }

            {
                auto [a, b] = motor::meta::boolm_t::do_cast( comp );
                if ( a )
                {
                    // do some bool stuff
                    return ;
                }
            }


            {
                auto [a, b] = motor::meta::floatm_t::do_cast( comp );
                if ( a )
                {
                    // do some float stuff
                    return ;
                }
            }

            {
                auto [a, b] = motor::meta::intm_t::do_cast( comp );
                if ( a )
                {
                    // do some int stuff
                    return ;
                }
            }
            
        } ) ;
    }
    return 0 ;
}
