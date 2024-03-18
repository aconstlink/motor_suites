

#include <motor/platform/global.h>

#include <motor/math/vector/vector3.hpp>
#include <motor/math/vector/vector4.hpp>
#include <motor/math/matrix/matrix4.hpp>

#include <motor/controls/system.h>
#include <motor/controls/midi_observer.hpp>
#include <motor/controls/layouts/midi_controller.hpp>

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

}

int main( int argc, char ** argv )
{
    motor::controls::midi_device_ptr_t dev = nullptr ;

    motor::application::carrier_mtr_t carrier = motor::platform::global_t::create_carrier() ;

    this_file::my_observer * obs = motor::memory::create_ptr<this_file::my_observer>() ;
    carrier->device_system()->install( obs ) ;

    size_t i=0;
    while( i++ < 5000 )
    {
        std::this_thread::sleep_for( std::chrono::milliseconds(20) ) ;

        carrier->device_system()->search( [&] ( motor::controls::device_mtr_t dev_in )
        {
            if( auto * ptr1 = dynamic_cast<motor::controls::midi_device_ptr_t>(dev_in); ptr1 != nullptr )
            {
                // just take the first one
                // or test for the name ptr1->name()
                dev = ptr1 ;
            }
        } ) ;

        carrier->device_system()->update() ;

        test_device( dev ) ;
    }

    motor::memory::release_ptr( obs ) ;
    motor::memory::release_ptr( carrier ) ;
    motor::log::global::deinit() ;
    motor::memory::global_t::dump_to_std() ;

    return 0 ;
}
