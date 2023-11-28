
#include <motor/math/vector/vector3.hpp>
#include <motor/math/vector/vector4.hpp>
#include <motor/math/matrix/matrix4.hpp>

#include <motor/device/layouts/xbox_controller.hpp>
#include <motor/device/global.h>
#include <motor/log/global.h>

using namespace motor::core::types ;

void_t test_device( motor::device::xbc_device_mtr_t dev ) 
{
    motor::device::layouts::xbox_controller_t ctrl( dev ) ;

    if( dev )
    {
        // buttons
        {
            auto button_funk = [&] ( motor::device::layouts::xbox_controller_t::button const button )
            {
                static bool_t pressing = false ;

                if( ctrl.is( button, motor::device::components::button_state::pressed ) )
                {
                    motor::log::global_t::status( "button pressed: " + motor::device::layouts::xbox_controller_t::to_string( button ) ) ;
                }
                else if( ctrl.is( button, motor::device::components::button_state::pressing ) )
                {
                    pressing = true ;
                    motor::log::global_t::status( "button pressing: " + motor::device::layouts::xbox_controller_t::to_string( button ) ) ;
                }
                else if( ctrl.is( button, motor::device::components::button_state::released ) )
                {
                    pressing = false ;
                    motor::log::global_t::status( "button released: " + motor::device::layouts::xbox_controller_t::to_string( button ) ) ;
                }
            } ;

            using part_t = motor::device::layouts::xbox_controller_t::button ;

            button_funk( part_t::start ) ;
            button_funk( part_t::back ) ;
            button_funk( part_t::a ) ;
            button_funk( part_t::b ) ;
            button_funk( part_t::x ) ;
            button_funk( part_t::y ) ;
        }

        // dpad
        {
            auto button_funk = [&] ( motor::device::layouts::xbox_controller_t::dpad const button )
            {
                static bool_t pressing = false ;

                if( ctrl.is( button, motor::device::components::button_state::pressed ) )
                {
                    motor::log::global_t::status( "dpad pressed: " + motor::device::layouts::xbox_controller_t::to_string( button ) ) ;
                }
                else if( ctrl.is( button, motor::device::components::button_state::pressing ) )
                {
                    pressing = true ;
                    motor::log::global_t::status( "dpad pressing: " + motor::device::layouts::xbox_controller_t::to_string( button ) ) ;
                }
                else if( ctrl.is( button, motor::device::components::button_state::released ) )
                {
                    pressing = false ;
                    motor::log::global_t::status( "dpad released: " + motor::device::layouts::xbox_controller_t::to_string( button ) ) ;
                }
            } ;

            using part_t = motor::device::layouts::xbox_controller_t::dpad ;

            button_funk( part_t::left ) ;
            button_funk( part_t::right ) ;
            button_funk( part_t::up ) ;
            button_funk( part_t::down ) ;
        }

        // shoulder
        {
            auto button_funk = [&] ( motor::device::layouts::xbox_controller_t::shoulder const button )
            {
                static bool_t pressing = false ;

                if( ctrl.is( button, motor::device::components::button_state::pressed ) )
                {
                    motor::log::global_t::status( "shoulder pressed: " + motor::device::layouts::xbox_controller_t::to_string( button ) ) ;
                }
                else if( ctrl.is( button, motor::device::components::button_state::pressing ) )
                {
                    pressing = true ;
                    motor::log::global_t::status( "shoulder pressing: " + motor::device::layouts::xbox_controller_t::to_string( button ) ) ;
                }
                else if( ctrl.is( button, motor::device::components::button_state::released ) )
                {
                    pressing = false ;
                    motor::log::global_t::status( "shoulder released: " + motor::device::layouts::xbox_controller_t::to_string( button ) ) ;
                }
            } ;

            using part_t = motor::device::layouts::xbox_controller_t::shoulder ;

            button_funk( part_t::left ) ;
            button_funk( part_t::right ) ;
        }

        // thumb
        {
            auto button_funk = [&] ( motor::device::layouts::xbox_controller_t::thumb const button )
            {
                static bool_t pressing = false ;

                if( ctrl.is( button, motor::device::components::button_state::pressed ) )
                {
                    motor::log::global_t::status( "thumb pressed: " + motor::device::layouts::xbox_controller_t::to_string( button ) ) ;
                }
                else if( ctrl.is( button, motor::device::components::button_state::pressing ) )
                {
                    pressing = true ;
                    motor::log::global_t::status( "thumb pressing: " + motor::device::layouts::xbox_controller_t::to_string( button ) ) ;
                }
                else if( ctrl.is( button, motor::device::components::button_state::released ) )
                {
                    pressing = false ;
                    motor::log::global_t::status( "thumb released: " + motor::device::layouts::xbox_controller_t::to_string( button ) ) ;
                }
            } ;

            using part_t = motor::device::layouts::xbox_controller_t::thumb ;

            button_funk( part_t::left ) ;
            button_funk( part_t::right ) ;
        }

        // trigger
        {
            auto button_funk = [&] ( motor::device::layouts::xbox_controller_t::trigger const button )
            {
                static bool_t pressing = false ;

                float_t value = 0.0f ;

                if( ctrl.is( button, motor::device::components::button_state::pressed, value ) )
                {
                    motor::log::global_t::status( "thumb pressed: " + motor::device::layouts::xbox_controller_t::to_string( button ) + 
                        " [" + motor::to_string(value) + "]") ;
                }
                else if( ctrl.is( button, motor::device::components::button_state::pressing, value ) )
                {
                    pressing = true ;
                    motor::log::global_t::status( "thumb pressing: " + motor::device::layouts::xbox_controller_t::to_string( button ) + 
                        " [" + motor::to_string(value) + "]") ;
                }
                else if( ctrl.is( button, motor::device::components::button_state::released, value ) )
                {
                    pressing = false ;
                    motor::log::global_t::status( "thumb released: " + motor::device::layouts::xbox_controller_t::to_string( button ) + 
                        " [" + motor::to_string(value) + "]") ;
                }
            } ;

            using part_t = motor::device::layouts::xbox_controller_t::trigger ;

            button_funk( part_t::left ) ;
            button_funk( part_t::right ) ;
        }

        // sticks
        {
            auto funk = [&] ( motor::device::layouts::xbox_controller_t::stick const s )
            {
                motor::math::vec2f_t value ;

                if( ctrl.is( s, motor::device::components::stick_state::tilted, value ) )
                {
                    motor::log::global_t::status( "stick tilted: " + motor::device::layouts::xbox_controller_t::to_string( s ) + 
                        " [" + motor::to_string( value.x() ) + "," + motor::to_string( value.y() ) + "]" ) ;
                }
                else if( ctrl.is( s, motor::device::components::stick_state::tilting, value ) )
                {
                    motor::log::global_t::status( "stick tilting: " + motor::device::layouts::xbox_controller_t::to_string( s ) + 
                        " [" + motor::to_string( value.x() ) + "," + motor::to_string( value.y() ) + "]" ) ;
                }
                else if( ctrl.is( s, motor::device::components::stick_state::untilted, value ) )
                {
                    motor::log::global_t::status( "stick untiled: " + motor::device::layouts::xbox_controller_t::to_string( s ) + 
                        " [" + motor::to_string( value.x() ) + "," + motor::to_string( value.y() ) + "]" ) ;
                }
            } ;

            using part_t = motor::device::layouts::xbox_controller_t::stick ;

            funk( part_t::left ) ;
            funk( part_t::right ) ;
        }

        // motor
        {
            float_t value = 0.0f ;
            {
                if( ctrl.is( motor::device::layouts::xbox_controller_t::trigger::left, motor::device::components::button_state::pressing, value ) )
                {
                    ctrl.set_motor( motor::device::layouts::xbox_controller_t::vibrator::left, value ) ;
                }
                else if( ctrl.is( motor::device::layouts::xbox_controller_t::trigger::left, motor::device::components::button_state::released, value ) )
                {
                    ctrl.set_motor( motor::device::layouts::xbox_controller_t::vibrator::left, value ) ;
                }
            }

            {
                if( ctrl.is( motor::device::layouts::xbox_controller_t::trigger::right, motor::device::components::button_state::pressing, value ) )
                {
                    ctrl.set_motor( motor::device::layouts::xbox_controller_t::vibrator::right, value ) ;
                }
                else if( ctrl.is( motor::device::layouts::xbox_controller_t::trigger::right, motor::device::components::button_state::released, value ) )
                {
                    ctrl.set_motor( motor::device::layouts::xbox_controller_t::vibrator::right, value ) ;
                }
            }
        }
    }
}

int main( int argc, char ** argv )
{
    motor::device::xbc_device_mtr_t dev = nullptr ;

    motor::device::global_t::init() ;

    {
        motor::device::global_t::system()->search( [&] ( motor::device::idevice_mtr_t dev_in )
        {
            if( auto * ptr1 = dynamic_cast<motor::device::xbc_device_mtr_t>(dev_in); ptr1 != nullptr )
            {
                dev = ptr1 ;
            }
        } ) ;

        if( !dev )
        {
            motor::log::global_t::status( "no xbox controller found" ) ;
        }

        test_device( dev ) ;
    }
    return 0 ;
}

#if 0 // can be used for gamepads for example

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <istream>
#include <unistd.h>
#include <linux/input.h>

int main( int argc, char ** argv )
{
    int fd ;
    struct input_event ie ;

    if( (fd=open( "/dev/input/event6", O_RDONLY )) == -1 )
    {
        perror( "opening device" ) ;
        exit( EXIT_FAILURE ) ;
    }
    
printf( "starting" ) ;
    while( read(fd, &ie, sizeof(struct input_event) ) )
    {
        unsigned char * ptr = (unsigned char * ) &ie ;
        for( int i=0; i<sizeof(ie); ++i )
        {
            printf( "%02X ", *ptr++ ) ;
        }
        printf("\n") ;
    }

}

#endif
