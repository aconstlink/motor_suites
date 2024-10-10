


#include <motor/math/utility/3d/transformation.hpp>
#include <motor/math/vector/vector3.hpp>
#include <motor/wire/variable.hpp>

#include <motor/log/global.h>



namespace motor
{
    namespace wire
    {
        template< typename T >
        class variable< motor::math::vector3< T > > : public motor::wire::any
        {
            using base_t = motor::wire::any ;

            motor_this_typedefs( variable< motor::math::vector3< T > > ) ;
            motor_typedefs(motor::math::vector3< T >, value ) ;

        private:

            using in_t = motor::wire::input_slot< value_t > ;
            using out_t = motor::wire::output_slot< value_t > ;

            using x_t = motor::wire::variable< T > ; 
            using y_t = motor::wire::variable< T > ; 
            using z_t = motor::wire::variable< T > ; 

            in_t * _in = motor::shared( in_t() ) ;
            out_t * _out = motor::shared( out_t() ) ;

            x_t _x = x_t( "x" ) ;
            y_t _y = y_t( "y" ) ;
            z_t _z = z_t( "z" ) ;

        public:

            variable( char const * const name ) noexcept : base_t( name ) {} 
            variable( this_cref_t ) = delete ;
            variable( this_rref_t rhv ) noexcept :
                _in( motor::move( rhv._in ) ), _out( motor::move( rhv._out ) ),
                _x( std::move( rhv._x ) ), _y( std::move( rhv._y ) ),
                _z( std::move( rhv._z ) ) {}
            virtual ~variable( void_t ) noexcept 
            {
                motor::release( motor::move( _in ) ) ;
                motor::release( motor::move( _out ) ) ;
            }

        public:

            virtual void_t update( void_t ) noexcept
            {
                _in->exchange() ;
                
                _x.update() ;
                _y.update() ;
                _z.update() ;

                // @todo need some "changed flag" in the slots
                auto v = _in->get_value() ;

                v.x( _x.get_value() ) ;
                v.y( _y.get_value() ) ;
                v.z( _z.get_value() ) ;

                *_out = v ;
            }

            

            T const & get_value( void_t ) const noexcept
            {
                return _out->get_value() ;
            }

            virtual motor::wire::iinput_slot_mtr_safe_t get_input( void_t ) noexcept
            {
                return motor::share( _in ) ;
            }

        private:

            virtual void_t for_each_member( for_each_funk_t f, motor::wire::any::member_info_in_t ifo ) noexcept
            {
                f( _x, { ifo.level + 1, ifo.full_name + "." + _x.name() } ) ;
                f( _y, { ifo.level + 1, ifo.full_name + "." + _y.name() } ) ;
                f( _z, { ifo.level + 1, ifo.full_name + "." + _z.name() } ) ;
            }
        };
        template< typename T >
        using vec3v = motor::wire::variable< motor::math::vector3< T > > ;

        motor_typedefs( variable< motor::math::vector3< float_t > >, vec3fv ) ;
    }
}

namespace motor
{
    namespace wire
    {
        template< typename T >
        class variable< motor::math::m3d::transformation< T > > : public motor::wire::any
        {
            using base_t = motor::wire::any ;

            motor_this_typedefs( variable< motor::math::m3d::transformation< T > > ) ;
            motor_typedefs( motor::math::m3d::transformation< T >, value ) ;

        private:

            using pos_t = motor::wire::vec3v< T > ;
            using axis_t = motor::wire::vec3v< T > ;
            using angle_t = motor::wire::variable< T > ;

            using in_t = motor::wire::input_slot< value_t > ;
            using out_t = motor::wire::output_slot< value_t > ;

        private:

            in_t * _in = motor::shared( in_t() ) ;
            out_t * _out = motor::shared( out_t() ) ;

            pos_t _pos = pos_t( "position" ) ;
            axis_t _axis = axis_t( "axis" ) ;
            angle_t _angle = angle_t( "angle" ) ;

        public:

            variable( char const * name ) noexcept : base_t( name ) {}
            variable( this_cref_t ) = delete ;
            variable( this_rref_t rhv ) noexcept : 
                _in( motor::move( rhv._in ) ), _out( motor::move( rhv._out) ),
                _pos( std::move( rhv._pos ) ), _axis( std::move( rhv._axis ) ),
                _angle( std::move( rhv._angle ) ){}

            virtual ~variable( void_t ) noexcept 
            {
                motor::release( motor::move( _in ) ) ;
                motor::release( motor::move( _out ) ) ;
            }
        
        public:

            virtual void_t update( void_t ) noexcept
            {
                //_in.exchange() ;

                _pos.update() ;
                _axis.update() ;
                _angle.update() ;

                // @todo need some "changed flag" in the slots
                auto v = _in->get_value() ;

                // set all the transformation values in a 
                // meaningful way.

                *_out = v ;
            }

            virtual motor::wire::iinput_slot_mtr_safe_t get_input( void_t ) noexcept
            {
                return motor::share( _in ) ;
            }

        private:

            virtual void_t for_each_member( for_each_funk_t f, motor::wire::any::member_info_in_t ifo ) noexcept
            {
                {
                    motor::string_t const full_name = ifo.full_name + "." + _pos.name() ;
                    f( _pos, { ifo.level, full_name } ) ;
                    motor::wire::any::derived_accessor( _pos ).for_each_member( f, { ifo.level + 1, full_name } ) ;
                }

                {
                    motor::string_t const full_name = ifo.full_name + "." + _axis.name() ;
                    f( _axis, { ifo.level, full_name } ) ;
                    motor::wire::any::derived_accessor(_axis).for_each_member( f, { ifo.level + 1, full_name } ) ;
                }
                
                {
                    motor::string_t const full_name = ifo.full_name + "." + _angle.name() ;
                    f( _angle, { ifo.level, full_name } ) ;
                    motor::wire::any::derived_accessor( _angle ).for_each_member( f, { ifo.level + 1, full_name } ) ;
                }
            }

        } ;
        motor_typedefs( variable< motor::math::m3d::transformation< float_t > >, trafo3fv ) ;
    }
}

namespace this_file
{

}

int main( int argc, char ** argv )
{
    {
        motor::wire::floatv_t my_float("some_name")  ;
    }


    {
        motor::wire::vec3fv_t var( "pos" ) ;
        var.inspect( [&]( motor::wire::any_ref_t, motor::wire::any::member_info_in_t info )
        {
            motor::log::global_t::status(info.full_name) ;
        } ) ;
    }

    {
        motor::wire::trafo3fv_t var( "trafo" ) ;
        var.inspect( [&] ( motor::wire::any_ref_t, motor::wire::any::member_info_in_t info )
        {
            motor::log::global_t::status( info.full_name ) ;
        } ) ;
    }

    // test inputs
    {
        motor::wire::trafo3fv_t var( "trafo" ) ;
        auto inputs = var.inputs() ;
        int bp = 0 ;
    }

    motor::log::global::deinit() ;
    motor::memory::global::dump_to_std() ;

    return 0 ;
}