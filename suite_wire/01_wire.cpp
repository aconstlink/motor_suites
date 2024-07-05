

#include <motor/wire/slot/sheet.hpp>

using namespace motor::core::types ;

int main( int argc, char ** argv )
{
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

    motor::memory::global_t::dump_to_std() ;
    return 0 ;
}