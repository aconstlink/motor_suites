#include <motor/math/vector/vector3.hpp>
#include <motor/math/matrix/matrix2.hpp>

#include <motor/graphics/variable/variable_set.hpp>
#include <motor/graphics/variable/variable_set_view.hpp>

#include <motor/graphics/variable/wire_variable_input_bridge.hpp>
#include <motor/graphics/variable/wire_variable_output_bridge.hpp>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>

int main( int argc, char ** argv )
{
    using namespace motor::core::types;

    // test 01
    {
        auto vs = motor::shared( motor::graphics::variable_set_t() );

        vs->data_variable< float_t >( "some_var" )->set( 1.0f );
        vs->data_variable< float_t >( "some_var2" )->set( 2.0f );
        vs->data_variable< motor::math::vec2f_t >( "some_vec2" )
            ->set( motor::math::vec2f_t( 1.0f ) );
        vs->data_variable< motor::math::mat2f_t >( "some_mat2" )->set( motor::math::mat2f_t() );

        motor::graphics::wire_variable_input_bridge_t ib( motor::move( vs ) );

        assert( ib.borrow_inputs().borrow( "some_var" ) != nullptr );
        assert( ib.borrow_inputs().borrow( "some_var2" ) != nullptr );
        assert( ib.borrow_inputs().borrow( "some_vec2" ) != nullptr );
        assert( ib.borrow_inputs().borrow( "some_mat2" ) != nullptr );

        motor::log::global_t::status( "[test 01] : all ok" );
    }

    // test 02
    {
        auto vs = motor::shared( motor::graphics::variable_set_t() );
        {
            vs->data_variable< float_t >( "some_var" )->set( 1.0f );
            vs->data_variable< float_t >( "some_var2" )->set( 2.0f );
            vs->data_variable< motor::math::vec2f_t >( "some_vec2" )
                ->set( motor::math::vec2f_t( 1.0f ) );
            vs->data_variable< motor::math::mat2f_t >( "some_mat2" )->set( motor::math::mat2f_t() );
        }

        motor::graphics::wire_variable_input_bridge_t ib( motor::move( vs ) );

        motor::wire::named_outputs_t oss;

        {
            oss.add( "some_value", motor::shared( motor::wire::output_slot< float_t >( 10.0f ) ) );
        }

        {
            auto const res = ib.borrow_inputs().connect( "some_var", oss.get( "some_value" ) );
            assert( res && "connect float value" );

            auto * s = ib.borrow_inputs().borrow_by_cast< motor::wire::input_slot< float_t > >(
                "some_var" );
            assert( s->get_value() == 10.0f );
        }

        motor::log::global_t::status( "[test 02] : all ok" );
    }

    // test 03
    {
        auto vs = motor::shared( motor::graphics::variable_set_t() );
        {
            vs->data_variable< float_t >( "some_var" )->set( 1.0f );
            vs->data_variable< float_t >( "some_var2" )->set( 2.0f );
            vs->data_variable< motor::math::vec2f_t >( "some_vec2" )
                ->set( motor::math::vec2f_t( 1.0f ) );
            vs->data_variable< motor::math::mat2f_t >( "some_mat2" )->set( motor::math::mat2f_t() );
        }

        motor::graphics::wire_variable_output_bridge_t ob( motor::move( vs ) );

        assert( ob.borrow_outputs().borrow( "some_var" ) != nullptr );
        assert( ob.borrow_outputs().borrow( "some_var2" ) != nullptr );
        assert( ob.borrow_outputs().borrow( "some_vec2" ) != nullptr );
        assert( ob.borrow_outputs().borrow( "some_mat2" ) != nullptr );

        motor::log::global_t::status( "[test 03] : all ok" );
    }

    // test 04
    // We want the shader variables as output slots so we can connect to other slots
    // and take the values from the shader variables.
    {
        auto vs = motor::shared( motor::graphics::variable_set_t() );
        {
            vs->data_variable< float_t >( "some_var" )->set( 1.0f );
            vs->data_variable< float_t >( "some_var2" )->set( 2.0f );
            vs->data_variable< motor::math::vec2f_t >( "some_vec2" )
                ->set( motor::math::vec2f_t( 1.0f ) );
            vs->data_variable< motor::math::mat2f_t >( "some_mat2" )->set( motor::math::mat2f_t() );
        }

        motor::graphics::wire_variable_output_bridge_t ob( motor::share( vs ) );

        motor::wire::named_inputs_t iss;
        iss.add( "my_value", motor::shared( motor::wire::input_slot< float_t >( 101.0f ) ) );

        iss.connect( "my_value",
            motor::share( ob.borrow_outputs().borrow_by_cast< motor::wire::output_slot< float_t > >(
                "some_var" ) ) );

        assert(
            iss.borrow_by_cast< motor::wire::input_slot< float_t > >( "my_value" )->get_value() ==
            1.0f );

        {
            vs->data_variable< float_t >( "some_var" )->set( 2.0f );
            assert( iss.borrow_by_cast< motor::wire::input_slot< float_t > >( "my_value" )
                        ->get_value() == 1.0f );

            // pull data from shader variables
            ob.pull_data();

            assert( iss.borrow_by_cast< motor::wire::input_slot< float_t > >( "my_value" )
                        ->get_value() == 1.0f );

            // exchange data from output slots to connected input slots
            ob.exchange();

            assert( iss.borrow_by_cast< motor::wire::input_slot< float_t > >( "my_value" )
                        ->get_value() == 2.0f );
        }

        motor::release( motor::move( vs ) );

        motor::log::global_t::status( "[test 04] : all ok" );
    }

    motor::log::global_t::deinit();
    motor::memory::global_t::dump_to_std();

    return 0;
}