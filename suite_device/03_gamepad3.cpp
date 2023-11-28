
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
    using namespace natus::core::types ;

    class test_app : public natus::application::app
    {
        natus_this_typedefs( test_app ) ;

    private:

        app::window_async_t _wid_async ;
        
        // always exists! We just need some mappings.
        natus::device::game_device_res_t _game_dev = natus::device::game_device_t() ;

        natus::ntd::vector< natus::device::imapping_res_t > _mappings ;

    public:

        test_app( void_t ) 
        {
            natus::application::app::window_info_t wi ;
            _wid_async = this_t::create_window( "A Render Window", wi ) ;
        }
        test_app( this_cref_t ) = delete ;
        test_app( this_rref_t rhv ) : app( ::std::move( rhv ) ) 
        {
            _wid_async = ::std::move( rhv._wid_async ) ;
            _game_dev = ::std::move( rhv._game_dev ) ;
            _mappings = ::std::move( rhv._mappings ) ;
        }
        virtual ~test_app( void_t ) 
        {}

    private:

        virtual natus::application::result on_init( void_t ) noexcept
        { 
            natus::device::xbc_device_res_t xbc_dev ;
            natus::device::ascii_device_res_t ascii_dev ;
            natus::device::three_device_res_t three_dev ;

            natus::device::global_t::system()->search( [&] ( natus::device::idevice_res_t dev_in )
            {
                if( natus::device::xbc_device_res_t::castable( dev_in ) )
                {
                    if( xbc_dev.is_valid() ) return ;
                    xbc_dev = dev_in ;
                }
                if( natus::device::ascii_device_res_t::castable( dev_in ) )
                {
                    if( ascii_dev.is_valid() ) return ;
                    ascii_dev = dev_in ;
                }
                if( natus::device::three_device_res_t::castable( dev_in ) )
                {
                    if( three_dev.is_valid() ) return ;
                    three_dev = dev_in ;
                }
            } ) ;

            // do mappings for xbox
            {
                using a_t = natus::device::game_device_t ;
                using b_t = natus::device::xbc_device_t ;

                using ica_t = a_t::layout_t::input_component ;
                using icb_t = b_t::layout_t::input_component ;

                using mapping_t = natus::device::mapping< a_t, b_t > ;
                mapping_t m( "xbc->gc", _game_dev, xbc_dev ) ;

                {
                    auto const res = m.insert( ica_t::jump, icb_t::button_a ) ;
                    natus::log::global_t::warning( natus::core::is_not(res), "can not do mapping." ) ;
                }
                {
                    auto const res = m.insert( ica_t::shoot, icb_t::button_b ) ;
                    natus::log::global_t::warning( natus::core::is_not( res ), "can not do mapping." ) ;
                }
                {
                    auto const res = m.insert( ica_t::action_a, icb_t::button_x ) ;
                    natus::log::global_t::warning( natus::core::is_not( res ), "can not do mapping." ) ;
                }
                {
                    auto const res = m.insert( ica_t::action_b, icb_t::button_y ) ;
                    natus::log::global_t::warning( natus::core::is_not( res ), "can not do mapping." ) ;
                }
                {
                    auto const res = m.insert( ica_t::aim, icb_t::stick_right ) ;
                    natus::log::global_t::warning( natus::core::is_not( res ), "can not do mapping." ) ;
                }
                {
                    auto const res = m.insert( ica_t::movement, icb_t::stick_left ) ;
                    natus::log::global_t::warning( natus::core::is_not( res ), "can not do mapping." ) ;
                }

                _mappings.emplace_back( natus::memory::res_t<mapping_t>(m) ) ;
            }

            // do mappings for ascii
            {
                using a_t = natus::device::game_device_t ;
                using b_t = natus::device::ascii_device_t ;

                using ica_t = a_t::layout_t::input_component ;
                using icb_t = b_t::layout_t::input_component ;

                using mapping_t = natus::device::mapping< a_t, b_t > ;
                mapping_t m( "ascii->gc", _game_dev, ascii_dev ) ;

                {
                    auto const res = m.insert( ica_t::jump, 
                        b_t::layout_t::ascii_key_to_input_component(b_t::layout_t::ascii_key::space ) ) ;
                    natus::log::global_t::warning( natus::core::is_not( res ), "can not do mapping." ) ;
                }

                {
                    auto const res = m.insert( ica_t::movement,
                        b_t::layout_t::ascii_key_to_input_component( b_t::layout_t::ascii_key::a ),
                        natus::device::mapping_detail::negative_x ) ;
                    natus::log::global_t::warning( natus::core::is_not( res ), "can not do mapping." ) ;
                }

                {
                    auto const res = m.insert( ica_t::movement,
                        b_t::layout_t::ascii_key_to_input_component( b_t::layout_t::ascii_key::d ),
                        natus::device::mapping_detail::positive_x ) ;
                    natus::log::global_t::warning( natus::core::is_not( res ), "can not do mapping." ) ;
                }

                {
                    auto const res = m.insert( ica_t::movement,
                        b_t::layout_t::ascii_key_to_input_component( b_t::layout_t::ascii_key::w ),
                        natus::device::mapping_detail::positive_y ) ;
                    natus::log::global_t::warning( natus::core::is_not( res ), "can not do mapping." ) ;
                }

                {
                    auto const res = m.insert( ica_t::movement,
                        b_t::layout_t::ascii_key_to_input_component( b_t::layout_t::ascii_key::s ),
                        natus::device::mapping_detail::negative_y ) ;
                    natus::log::global_t::warning( natus::core::is_not( res ), "can not do mapping." ) ;
                }
                

                _mappings.emplace_back( natus::memory::res_t<mapping_t>( m ) ) ;
            }

            // do mappings for mouse
            {
                using a_t = natus::device::game_device_t ;
                using b_t = natus::device::three_device_t ;

                using ica_t = a_t::layout_t::input_component ;
                using icb_t = b_t::layout_t::input_component ;

                using mapping_t = natus::device::mapping< a_t, b_t > ;
                mapping_t m( "mouse->gc", _game_dev, three_dev ) ;

                {
                    auto const res = m.insert( ica_t::aim, icb_t::local_coords ) ;
                    natus::log::global_t::warning( natus::core::is_not( res ), "can not do mapping." ) ;
                }

                _mappings.emplace_back( natus::memory::res_t<mapping_t>( m ) ) ;
            }

            return natus::application::result::ok ; 
        }

        void_t test_button( natus::device::layouts::game_controller_t::button const btn )
        {
            using ctrl_t = natus::device::layouts::game_controller_t ;
            ctrl_t ctrl( _game_dev ) ;

            float_t value = 0.0f ;
            if( ctrl.is( btn, natus::device::components::button_state::pressed, value ) )
            {
                natus::log::global_t::status( "pressed: " + ctrl_t::to_string( btn ) +
                    " [" + ::std::to_string( value ) + "]" ) ;
            }
            else if( ctrl.is( btn, natus::device::components::button_state::pressing, value ) )
            {
                natus::log::global_t::status( "pressing: " + ctrl_t::to_string( btn ) +
                    " [" + ::std::to_string( value ) + "]" ) ;
            }
            else if( ctrl.is( btn, natus::device::components::button_state::released, value ) )
            {
                natus::log::global_t::status( "released: " + ctrl_t::to_string( btn ) +
                    " [" + ::std::to_string( value ) + "]" ) ;
            }
        }

        void_t test_device( void_t ) 
        {
            for( auto & m : _mappings )
            {
                m->update() ;
            }

            using ctrl_t = natus::device::layouts::game_controller_t ;
            for( size_t i=0; i<size_t(ctrl_t::button::num_buttons); ++i )
            {
                this_t::test_button( ctrl_t::button(i) ) ;
            }

            ctrl_t ctrl( _game_dev ) ;

            natus::math::vec2f_t value ;
            if( ctrl.is( ctrl_t::directional::aim, natus::device::components::stick_state::tilting, value ) )
            {
                natus::log::global_t::status( "aiming: [" + ::std::to_string( value.x() ) + "," + ::std::to_string( value.y() ) + "]" ) ;
            }

            if( ctrl.is( ctrl_t::directional::movement, natus::device::components::stick_state::tilting, value ) )
            {
                natus::log::global_t::status( "movement: [" + ::std::to_string( value.x() ) + "," + ::std::to_string( value.y() ) + "]" ) ;
            }
            
        }

        virtual natus::application::result on_update( natus::application::app_t::update_data_in_t ) noexcept 
        { 
            this_t::test_device() ;


            // tmp here. This needs to go into the gamepad module
            _game_dev->update() ;

            ::std::this_thread::sleep_for( ::std::chrono::milliseconds( 1 ) ) ;

            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_graphics( natus::application::app_t::render_data_in_t ) noexcept 
        { 
            return natus::application::result::ok ; 
        }

        virtual natus::application::result on_shutdown( void_t ) noexcept 
        { return natus::application::result::ok ; }
    };
    natus_res_typedef( test_app ) ;
}

int main( int argc, char ** argv )
{
    return natus::application::global_t::create_and_exec_application( 
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
