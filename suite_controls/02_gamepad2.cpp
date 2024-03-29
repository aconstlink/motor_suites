
#include "main.h"

#include <natus/application/global.h>
#include <natus/application/app.h>
#include <natus/gfx/camera/pinhole_camera.h>
#include <natus/graphics/variable/variable_set.hpp>
#include <natus/math/vector/vector3.hpp>
#include <natus/math/vector/vector4.hpp>
#include <natus/math/matrix/matrix4.hpp>

#include <natus/device/layouts/xbox_controller.hpp>
#include <natus/device/layouts/game_controller.hpp>
#include <natus/device/layouts/ascii_keyboard.hpp>
#include <natus/device/layouts/three_mouse.hpp>
#include <natus/device/mapping.hpp>
#include <natus/device/global.h>

#include <thread>

namespace this_file
{
    using namespace motor::core::types ;

    class test_app : public motor::application::app
    {
        motor_this_typedefs( test_app ) ;

    private:

        app::window_async_t _wid_async ;
        
        // always exists! We just need some mappings.
        motor::controls::game_device_res_t _game_dev ;

        motor::vector< motor::controls::imapping_res_t > _mappings ;

    public:

        test_app( void_t ) 
        {
            motor::application::app::window_info_t wi ;
            _wid_async = this_t::create_window( "A Render Window", wi ) ;
        }
        test_app( this_cref_t ) = delete ;
        test_app( this_rref_t rhv ) : app( std::move( rhv ) ) 
        {
            _wid_async = std::move( rhv._wid_async ) ;
            _game_dev = std::move( rhv._game_dev ) ;
            _mappings = std::move( rhv._mappings ) ;
        }
        virtual ~test_app( void_t ) 
        {}

    private:

        virtual motor::application::result on_init( void_t ) noexcept
        { 
            motor::controls::global_t::system()->search( [&] ( motor::controls::idevice_res_t dev_in )
            {
                if( motor::controls::game_device_res_t::castable( dev_in ) )
                {
                    if( _game_dev.is_valid() ) return ;

                    _game_dev = dev_in ;
                }
            } ) ;

            return motor::application::result::ok ; 
        }

        void_t test_button( motor::controls::types::game_controller_t::button const btn )
        {
            using ctrl_t = motor::controls::types::game_controller_t ;
            ctrl_t ctrl( _game_dev ) ;

            float_t value = 0.0f ;
            if( ctrl.is( btn, motor::controls::components::button_state::pressed, value ) )
            {
                motor::log::global_t::status( "pressed: " + ctrl_t::to_string( btn ) +
                    " [" + std::to_string( value ) + "]" ) ;
            }
            else if( ctrl.is( btn, motor::controls::components::button_state::pressing, value ) )
            {
                motor::log::global_t::status( "pressing: " + ctrl_t::to_string( btn ) +
                    " [" + std::to_string( value ) + "]" ) ;
            }
            else if( ctrl.is( btn, motor::controls::components::button_state::released, value ) )
            {
                motor::log::global_t::status( "released: " + ctrl_t::to_string( btn ) +
                    " [" + std::to_string( value ) + "]" ) ;
            }
        }

        void_t test_device( void_t ) 
        {

            using ctrl_t = motor::controls::types::game_controller_t ;
            for( size_t i=0; i<size_t(ctrl_t::button::num_buttons); ++i )
            {
                this_t::test_button( ctrl_t::button(i) ) ;
            }

            ctrl_t ctrl( _game_dev ) ;

            motor::math::vec2f_t value ;
            if( ctrl.is( ctrl_t::directional::aim, motor::controls::components::stick_state::tilting, value ) )
            {
                motor::log::global_t::status( "aiming: [" + std::to_string( value.x() ) + "," + std::to_string( value.y() ) + "]" ) ;
            }

            if( ctrl.is( ctrl_t::directional::movement, motor::controls::components::stick_state::tilting, value ) )
            {
                motor::log::global_t::status( "movement: [" + std::to_string( value.x() ) + "," + std::to_string( value.y() ) + "]" ) ;
            }
            
        }

        virtual motor::application::result on_update( motor::application::app_t::update_data_in_t ) noexcept 
        { 
            this_t::test_device() ;

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) ) ;

            return motor::application::result::ok ; 
        }

        virtual motor::application::result on_graphics( motor::application::app_t::render_data_in_t ) noexcept 
        { 
            return motor::application::result::ok ; 
        }

        virtual motor::application::result on_shutdown( void_t ) noexcept 
        { return motor::application::result::ok ; }
    };
    motor_res_typedef( test_app ) ;
}

int main( int argc, char ** argv )
{
    return motor::application::global_t::create_and_exec_application( 
        this_file::test_app_res_t( this_file::test_app_t() ) ) ;
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
