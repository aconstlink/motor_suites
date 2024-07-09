

#include <motor/wire/slot/input_slot.h>
#include <motor/wire/slot/output_slot.h>

#include <motor/wire/slot/sheet.hpp>

#include <limits>

using namespace motor::core::types ;

int main( int argc, char ** argv )
{
    {
        auto sig = motor::shared( motor::wire::output_slot< float_t >(), "signal" ) ;
        auto slot = motor::shared( motor::wire::input_slot< float_t >(), "slot" ) ;
        slot->connect( motor::share( sig ) ) ;


        {
            auto slot2 = motor::shared( motor::wire::input_slot< float_t >(), "slot" ) ;
            sig->connect( motor::move( slot2 ) ) ;
        }
        
        {
            auto slot2 = motor::shared( motor::wire::input_slot< float_t >(), "slot2" ) ;
            sig->connect( motor::share( slot2 ) ) ;
            sig->disconnect( slot2 ) ;

            motor::release( motor::move( slot2 ) ) ;
        }
        sig->disconnect() ;

        motor::release( motor::move( sig ) ) ;
        motor::release( motor::move( slot ) ) ;
    }

    {
        motor::wire::sheet< motor::wire::ioutput_slot > outputs ;
        motor::wire::sheet< motor::wire::iinput_slot > inputs ;

        {
            outputs.add( "a signal", motor::shared( motor::wire::output_slot<float_t>( 1.0f ), "signal" ) ) ;
            inputs.add( "a slot", motor::shared( motor::wire::input_slot<float_t>( 0.0f ) ) ) ;

            assert( inputs.borrow( "a slot" )->connect( outputs.get( "a signal" ) ) ) ;
        }

        int bp = 0 ;
    }

    {
        motor::wire::outputs_t outputs ;
        motor::wire::inputs_t inputs ;

        {
            outputs.add( "a signal", motor::shared( motor::wire::output_slot<float_t>( 1.0f ), "signal" ) ) ;
            inputs.add( "a slot", motor::shared( motor::wire::input_slot<float_t>( 0.0f ) ) ) ;

            assert( inputs.borrow( "a slot" )->connect( outputs.get( "a signal" ) ) ) ;
        }
    }

    {

    }

    motor::memory::global::dump_to_std() ;
    return 0 ;
}