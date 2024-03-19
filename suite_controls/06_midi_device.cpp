

#include <motor/platform/global.h>

#include <motor/math/vector/vector3.hpp>
#include <motor/math/vector/vector4.hpp>
#include <motor/math/matrix/matrix4.hpp>

#include <motor/controls/system.h>
#include <motor/controls/midi_observer.hpp>
#include <motor/controls/layouts/midi_controller.hpp>
#include <motor/controls/components/button.hpp>
#include <motor/controls/components/slider.hpp>
#include <motor/controls/components/knob.hpp>

#include <motor/log/global.h>

using namespace motor::core::types ;

namespace this_file
{
    class my_observer : public motor::controls::midi_observer
    {
        virtual void_t on_message( motor::string_cref_t dname, motor::controls::midi_message_cref_t msg ) noexcept
        {
            motor::controls::midi_observer::on_message( dname, msg ) ;
            
            
        }
    };
}
void_t test_device( motor::controls::midi_device_mtr_t dev ) 
{
    for( size_t id=0; id<100; ++id )
    {
        motor::string_t name = "[id " + motor::to_string( id )+"]" ;

        {
            auto ptr = dynamic_cast< motor::controls::components::button_ptr_t>( dev->get_in_component( id ) ) ;
            if( ptr != nullptr && ptr->state() != motor::controls::components::button_state::none )
            {
                motor::log::global_t::status( "button : " + name + " is " + motor::controls::components::to_string(ptr->state() ) ) ;
                continue ;
            }
        }
            
        {
            auto ptr = dynamic_cast< motor::controls::components::slider_ptr_t>( dev->get_in_component( id ) ) ;
            if( ptr != nullptr && ptr->has_changed() )
            {
                motor::log::global_t::status( "slider: " + name + " is " + motor::to_string( ptr->value()  ) ) ;
                continue ;
            }
        }

        {
            auto ptr = dynamic_cast< motor::controls::components::knob_ptr_t>( dev->get_in_component( id ) ) ;
            if( ptr != nullptr && ptr->has_changed() )
            {
                motor::log::global_t::status( "knob: " + name + " is " + motor::to_string( ptr->value()  ) ) ;
                continue ;
            }
        }
    }
}

int main( int argc, char ** argv )
{
    motor::controls::midi_device_ptr_t dev = nullptr ;

    motor::application::carrier_mtr_t carrier = motor::platform::global_t::create_carrier() ;

    this_file::my_observer * obs = motor::memory::create_ptr<this_file::my_observer>() ;
    //carrier->device_system()->install( obs ) ;

    size_t i=0;
    while( i++ < 5000 )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds(20) ) ;

        carrier->device_system()->search( [&] ( motor::controls::device_mtr_t dev_in )
        {
            if( auto * ptr1 = dynamic_cast<motor::controls::midi_device_ptr_t>(dev_in); ptr1 != nullptr )
            {
                if( ptr1->name() == "MIDI Mix" )
                {
                    int bp = 0 ;
                }
                // just take the first one
                // or test for the name ptr1->name()
                dev = ptr1 ;
            }
        } ) ;

        carrier->device_system()->update() ;

        if( dev != nullptr ) test_device( dev ) ;
    }

    motor::memory::release_ptr( obs ) ;
    motor::memory::release_ptr( carrier ) ;
    motor::log::global::deinit() ;
    motor::memory::global_t::dump_to_std() ;

    return 0 ;
}
