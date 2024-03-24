

#include <motor/platform/global.h>

#include <motor/math/vector/vector3.hpp>
#include <motor/math/vector/vector4.hpp>
#include <motor/math/matrix/matrix4.hpp>

#include <motor/controls/system.h>
#include <motor/controls/midi/layouts/korg_layouts.hpp>

#include <motor/log/global.h>

using namespace motor::core::types ;

//**********************************************************
bool_t test_nanokontrol2( motor::controls::midi_device_mtr_t dev ) 
{
    if( dev->name() != "nanoKONTROL2" ) return false ;

    motor::controls::midi::layouts::korg_nanokontrol2 midi( dev ) ;

    using button = motor::controls::midi::layouts::korg_nanokontrol2::button ;
    using slider = motor::controls::midi::layouts::korg_nanokontrol2::slider ;
    using knob = motor::controls::midi::layouts::korg_nanokontrol2::knob ;
    using led = motor::controls::midi::layouts::korg_nanokontrol2::led ;

    for( size_t i = size_t(button::button_1); i<size_t(button::num_buttons); ++i )
    {
        button const b = (button)i ;
        if( midi.get_state( b ) != motor::controls::components::button_state::none )
        {
            motor::log::global_t::status( dev->name() + " : Button ["+motor::to_string(i)+"] : " + 
                motor::controls::components::to_string(midi.get_state(b) ) ) ;

            // set led state
            // if any button is pressed, all leds should be on
            {
                bool_t const on_off = midi.get_state( b ) == 
                    motor::controls::components::button_state::pressing ;

                for( size_t j = size_t(led::led_1); j<size_t(led::num_leds); ++j )
                {
                    midi.set( led( j ) , on_off ) ;
                }
            }
        }
    }

    for( size_t i = size_t(slider::slider_1); i<size_t(slider::num_sliders); ++i )
    {
        slider const b = (slider)i ;
        if( midi.has_changed( b ) )
        {
            motor::log::global_t::status( dev->name() + " : Slider ["+motor::to_string(i)+"] : " + 
                motor::to_string( midi.get_value(b) ) ) ;
        }
    }

    for( size_t i = size_t(knob::knob_1); i<size_t(knob::num_knobs); ++i )
    {
        knob const b = (knob)i ;
        if( midi.has_changed( b ) )
        {
            motor::log::global_t::status( dev->name() + " : Knob["+motor::to_string(i)+"] : " + 
                motor::to_string( midi.get_value(b) ) ) ;
        }
    }

    {
        auto [changed,plugged] =  midi.plug_state() ;
        if( changed ) 
        {
            motor::log::global_t::status( dev->name() + " : Plug is : " + (plugged ? "in" : "out") ) ;
        }
    }

    return true ;
}

//**********************************************************
bool_t test_nanopad2( motor::controls::midi_device_mtr_t dev ) 
{
    if( dev->name() != "nanoPAD2" ) return false ;

    motor::controls::midi::layouts::korg_nanopad2 midi( dev ) ;

    using button = motor::controls::midi::layouts::korg_nanopad2::button ;
    using pad = motor::controls::midi::layouts::korg_nanopad2::pad ;

    for( size_t i = size_t(button::button_1); i<size_t(button::num_buttons); ++i )
    {
        button const b = (button)i ;
        if( midi.get_state( b ) != motor::controls::components::button_state::none )
        {
            motor::log::global_t::status( dev->name() + " : Button ["+motor::to_string(i)+"] : " + 
                motor::controls::components::to_string(midi.get_state(b) ) ) ;
        }
    }

    // test pad
    {
        if( midi.get_state( pad::pad_1 ) != motor::controls::components::touch_state::none )
        {
            auto const v = midi.get_value( pad::pad_1 ) ;

            motor::log::global_t::status( dev->name() + " Pad : [x,y] : ["
                + motor::to_string(v.x()) + " , " + motor::to_string( v.y() ) + "]" ) ;
        }
    }

    {
        auto [changed,plugged] =  midi.plug_state() ;
        if( changed ) 
        {
            motor::log::global_t::status( dev->name() + " : Plug is : " + (plugged ? "in" : "out") ) ;
        }
    }
    return true ;
}

int main( int argc, char ** argv )
{
    motor::controls::midi_device_ptr_t dev = nullptr ;

    motor::application::carrier_mtr_t carrier = motor::platform::global_t::create_carrier() ;

    size_t i=0;
    while( i++ < 5000 )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds(20) ) ;

        carrier->device_system()->search( [&] ( motor::controls::device_mtr_t dev_in )
        {
            if( auto * ptr1 = dynamic_cast<motor::controls::midi_device_ptr_t>(dev_in); ptr1 != nullptr )
            {
                dev = ptr1 ;
            }
        } ) ;

        carrier->device_system()->update() ;

        if( dev != nullptr ) 
        {
            test_nanopad2( dev ) ;
            test_nanokontrol2( dev ) ;
        }
    }

    motor::memory::release_ptr( carrier ) ;
    motor::log::global::deinit() ;
    motor::memory::global_t::dump_to_std() ;

    return 0 ;
}
