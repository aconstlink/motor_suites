

#include <motor/wire/node/node.h>
#include <motor/wire/node/node_disconnector.hpp>
#include <motor/log/global.h>
#include <motor/profiling/global.h>

#include <motor/concurrent/task/loose_thread_scheduler.hpp>
#include <motor/concurrent/global.h>

using namespace motor::core::types ;

namespace this_file
{
    // one option would be to derive from inode
    // and implement all the logic of the node 
    // directly in the execute() function.
    class my_node : public motor::wire::inode
    {
        using base_t = motor::wire::inode ;
        motor_this_typedefs( my_node ) ;

    private:

        motor::wire::input_slot< float_t > * _input_slot1 = nullptr ;
        motor::wire::input_slot< float_t > * _input_slot2 = nullptr ;

        motor::wire::output_slot< bool_t > * _output_slot1 = nullptr ;
        motor::wire::output_slot< int_t > * _output_slot2 = nullptr ;
        motor::wire::output_slot< float_t > * _output_slot3 = nullptr ;

    public:

        my_node( void_t ) noexcept : base_t( "my_node" )
        {
            this_t::init_slots() ;
        }

        my_node( motor::string_cref_t name ) noexcept : base_t( name )
        {
            this_t::init_slots() ;
        }

        my_node( this_rref_t rhv ) noexcept : base_t( std::move( rhv) ),
            _input_slot1( motor::move( rhv._input_slot1 ) ), _input_slot2( motor::move( rhv._input_slot2 ) ),
            _output_slot1( motor::move( rhv._output_slot1 ) ), _output_slot2( motor::move( rhv._output_slot2 ) ),
            _output_slot3( motor::move( rhv._output_slot3 ) )
        {
        }

        my_node( this_cref_t ) = delete ;

        virtual ~my_node( void_t ) noexcept
        {
            // slots do not need to be released. That is done
            // in the base class.
        }

        // the execute usually does some logic based on the input slots
        // and outputs the result to the output slots.
        virtual void_t execute( void_t ) noexcept
        {
            _output_slot1->set_value( _input_slot1->get_value() > 0.5f ) ;
            _output_slot2->set_value( int_t( _input_slot2->get_value() ) ) ;
            _output_slot3->set_value( _input_slot1->get_value() ) ;
        }

    private:

        void_t init_slots( void_t ) noexcept
        {
            // make input slots
            // #1 define slots
            // #2 add to sheet
            {
                _input_slot1 = motor::shared( motor::wire::input_slot< float_t >( 0.0f ) ) ;
                _input_slot2 = motor::shared( motor::wire::input_slot< float_t >( 1.0f ) ) ;

                this_t::inputs().add( "i1", motor::share_unsafe( _input_slot1 ) ) ;
                this_t::inputs().add( "i2", motor::share_unsafe( _input_slot2 ) ) ;
            }

            // make output slots
            // #1 add to sheet
            // #2 assign to slots
            {
                this_t::outputs().add( "o1", motor::shared( motor::wire::output_slot< bool_t >( false ) ) ) ;
                this_t::outputs().add( "o2", motor::shared( motor::wire::output_slot< int_t >( 100 ) ) ) ;
                this_t::outputs().add( "o3", motor::shared( motor::wire::output_slot< float_t >( 0.0f ) ) ) ;

                _output_slot1 = this_t::outputs().borrow_by_cast<decltype( _output_slot1 )>( "o1" ) ;
                _output_slot2 = this_t::outputs().borrow_by_cast<decltype( _output_slot2 )>( "o2" ) ;
                _output_slot3 = this_t::outputs().borrow_by_cast<decltype( _output_slot3 )>( "o3" ) ;
            }
        }
    };
    motor_typedef( my_node ) ;
}

int main( int argc, char ** argv )
{
    {
        motor::concurrent::loose_thread_scheduler lts ;
        lts.init() ;

        bool_t run_loop = true ;

        motor::wire::inode_mtr_t start = nullptr ;
        motor::wire::inode_mtr_t a = nullptr ;
        motor::wire::inode_mtr_t b = nullptr ;
        motor::wire::inode_mtr_t c = nullptr ;
        motor::wire::inode_mtr_t d = nullptr ;
        
        // option #1: Search for slots 
        // Search for the slots by name within the lambda function.
        {
            start = motor::shared( motor::wire::node( [=] ( motor::wire::node_ptr_t n )
            {
                if( auto output_slot = dynamic_cast<motor::wire::output_slot<float_t>*>( n->outputs().borrow("out") ); output_slot != nullptr ) 
                {
                    output_slot->set_value( 10.0f ) ;
                }
            } ) ) ;
            start->outputs().add( "out", motor::shared( motor::wire::output_slot<float_t>( 0.0f ) ) ) ;
        }
        
        // option #2 : Capture slots
        // Define slots somewhere before and just capture and use the slots.
        // This is very efficient, because the slots are "just" there and do
        // not need to be searched for as in option #1.
        // This options holds a reference of the slots within the sheets in the node.
        // So the lambda can capture ("borrow") the resource safely.
        {
            // create slots
            auto output_slot = motor::shared( motor::wire::output_slot<float_t>( 0.0f ) ) ;
            auto input_slot = motor::shared( motor::wire::input_slot<float_t>( 0.0f ) ) ;

            // capture slots
            a = motor::shared( motor::wire::node( [=] ( motor::wire::node_ptr_t n )
            {
                float_t const value = input_slot->get_value() * 2.0f ;
                output_slot->set_value( value ) ;
            } ) ) ;

            // safe slot for ref counting
            a->outputs().add( "out", motor::move( output_slot ) ) ;
            a->inputs().add( "in", motor::move( input_slot ) ) ;
        }

        // option #3 : Use Custom Node
        {
            b = motor::shared( this_file::my_node( "my_custom_node") ) ;
        }

        // as option #2
        {
            auto input_slot1 = motor::shared( motor::wire::input_slot<float_t>( 0.0f ) ) ;
            auto input_slot2 = motor::shared( motor::wire::input_slot<float_t>( 0.0f ) ) ;

            c = motor::shared( motor::wire::node( [=] ( motor::wire::node_ptr_t )
            {
                auto const v1 = input_slot1->get_value() ;
                auto const v2 = input_slot2->get_value() ;

                motor::log::global::status( "In node c:" ) ;
                motor::log::global_t::status( "slot 1: " + motor::to_string( v1 ) ) ;
                motor::log::global_t::status( "slot 2: " + motor::to_string( v2 ) ) ;
            } ) ) ;
            c->inputs().add( "in1", motor::move( input_slot1 ) ) ;
            c->inputs().add( "in2", motor::move( input_slot2 ) ) ;
        }

        // using a lambda node to shutdown the program
        {
            d = motor::shared( motor::wire::node( [&] ( motor::wire::node_ptr_t )
            {
                run_loop = false ;
                motor::log::global::status( "update ending" ) ;
            } ) ) ;
        }

        // #slots
        // connect all the slots
        {
            start->outputs().borrow( "out" )->connect( a->inputs().get("in" ) ) ;
            start->outputs().borrow( "out" )->connect( b->inputs().get("in1" ) ) ;
            start->outputs().borrow( "out" )->connect( c->inputs().get("in1" ) ) ;
            
            a->outputs().borrow( "out" )->connect( b->inputs().get( "in2" ) ) ;
            a->outputs().borrow( "out" )->connect( c->inputs().get( "in2" ) ) ;
        }

        // #graph
        //         .-(a)-.
        // (start)-|     |-(c)
        //         '-(b)-'
        {
            start->then( motor::move( a ) )->then( motor::share( c ) ) ;
            start->then( motor::move( b ) )->then( motor::move( c ) )->then( motor::move( d ) ) ;
        }

        // #execute round 1
        {
            lts.schedule( start->get_task() ) ;

            while ( run_loop )
            {
                lts.update() ;
            }
        }

        {
            motor::wire::node_disconnector_t::disconnect_everyting( motor::move( start ) ) ;
        }

        lts.deinit() ;
    }

    motor::profiling::global::deinit() ;
    motor::concurrent::global::deinit() ;
    motor::log::global::deinit() ;
    motor::memory::global_t::dump_to_std() ;
    return 0 ;
}