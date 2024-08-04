

#include <motor/profiling/probe_guard.hpp>

#include <motor/platform/global.h>

#include <motor/wire/node/node.h>
#include <motor/wire/node/node_disconnector.hpp>

#include <motor/tool/imgui/imnodes_wire.h>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>

#include <future>

namespace this_file
{
    using namespace motor::core::types ;

    class my_app : public motor::application::app
    {
        motor_this_typedefs( my_app ) ;

        motor::tool::imnodes_wire_t _imn_wire ;
        motor::wire::inode_mtr_t _start ;

        virtual void_t on_init( void_t ) noexcept
        {
            MOTOR_PROBE( "application", "on_init" ) ;

            {
                motor::application::window_info_t wi ;
                wi.x = 100 ;
                wi.y = 100 ;
                wi.w = 800 ;
                wi.h = 600 ;
                wi.gen = motor::application::graphics_generation::gen4_auto ;
                
                this_t::send_window_message( this_t::create_window( wi ), [&]( motor::application::app::window_view & wnd )
                {
                    wnd.send_message( motor::application::show_message( { true } ) ) ;
                    wnd.send_message( motor::application::cursor_message_t( {true} ) ) ;
                    wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                } ) ;
            }

            // init node graph
            {
                auto start = motor::shared( motor::wire::node( "start", [=] ( motor::wire::node_ptr_t )
                {
                } ), "start node" ) ;

                auto a = motor::shared( motor::wire::node( "a", [=] ( motor::wire::node_ptr_t )
                {
                } ), "node" ) ;

                auto b = motor::shared( motor::wire::node( "b",  [=] ( motor::wire::node_ptr_t )
                {
                    //std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) ) ;
                    
                } ), "node" ) ;

                auto c = motor::shared( motor::wire::node( "c", [=] ( motor::wire::node_ptr_t )
                {
                } ), "node" ) ;

                auto d = motor::shared( motor::wire::node( "d", [=] ( motor::wire::node_ptr_t )
                {
                } ), "node" ) ;

                auto e = motor::shared( motor::wire::node( "e", [=] ( motor::wire::node_ptr_t )
                {
                } ), "node" ) ;

                auto f = motor::shared( motor::wire::node( "f", [&] ( motor::wire::node_ptr_t )
                {
                } ), "node" ) ;

                // #graph
                //         .-(a)-.     .-(d)-.
                // (start)-|     |-(c)-'-(e)-|
                //         '-(b)-'-----------'-(f)

                {
                    start->then( motor::move( a ) )->then( motor::share( c ) )->then( motor::move( d ) )->then( motor::share( f ) ) ;
                    start->then( motor::share( b ) )->then( motor::move( c ) )->then( motor::move( e ) )->then( motor::share( f ) ) ;
                    
                    b->then( motor::move( f ) ) ;
                    
                    motor::release( b ) ;
                }
                {
                    _imn_wire.build( start ) ;
                }

                _start = motor::move( start ) ;
            }
        }

        virtual void_t on_event( window_id_t const wid, 
                motor::application::window_message_listener::state_vector_cref_t sv ) noexcept
        {
            MOTOR_PROBE( "application", "on_event" ) ;

            if( sv.create_changed )
            {
                motor::log::global_t::status("[my_app] : window created") ;
            }
            if( sv.close_changed )
            {
                motor::log::global_t::status("[my_app] : window closed") ;
                this->close() ;
            }
        }

        virtual bool_t on_tool( this_t::window_id_t const wid, motor::application::app::tool_data_ref_t ) noexcept 
        { 
            MOTOR_PROBE( "application", "on_tool" ) ;

            // #1 : nodes graph window
            if ( ImGui::Begin( "node graph visualization" ) )
            {
                _imn_wire.begin() ;
                _imn_wire.visualize( 0 ) ;
                _imn_wire.end( true ) ;
                
                ImGui::End();
            }

            return true ; 
        }

        virtual void_t on_shutdown( void_t ) noexcept 
        {
            motor::wire::node_disconnector_t::disconnect_everyting( motor::move( _start ) ) ;
        }
    };
}

int main( int argc, char ** argv )
{
    return motor::platform::global_t::create_and_exec< this_file::my_app >() ;
}