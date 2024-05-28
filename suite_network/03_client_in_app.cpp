

#include <motor/profiling/probe_guard.hpp>

#include <motor/platform/global.h>

#include <motor/controls/types/ascii_keyboard.hpp>
#include <motor/controls/types/three_mouse.hpp>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>

#include <future>

namespace this_file
{
    using namespace motor::core::types ;

    class my_client : public motor::network::iclient_handler
    {
        motor::string_t data ;

        virtual motor::network::user_decision on_connect( motor::network::connect_result const res, size_t const tries ) noexcept
        {
            motor::log::global::status( "client connection status: " + motor::network::to_string( res ) ) ;
            if( res == motor::network::connect_result::closed )
            {
                motor::log::global::status("Client will shutdown.") ;
                return motor::network::user_decision::shutdown ;
            }
            return motor::network::user_decision::keep_going ;
        }

        virtual motor::network::user_decision on_sync( void_t ) noexcept
        {
            return motor::network::user_decision::keep_going ;
        }

        virtual motor::network::user_decision on_update( void_t ) noexcept 
        {
            return motor::network::user_decision::keep_going ;
        }

        virtual void_t on_receive( byte_cptr_t buffer, size_t const sib ) noexcept 
        {
            data += motor::string_t( char_cptr_t(buffer), sib ) ;
        }

        virtual void_t on_received( void_t ) noexcept 
        {
            motor::log::global::status( data ) ;
            data.clear() ;
        }

        virtual void_t on_send( byte_cptr_t & buffer, size_t & num_sib ) noexcept 
        {
        }

        virtual void_t on_sent( motor::network::transmit_result const ) noexcept 
        {
        }
    };

    class my_app : public motor::application::app
    {
        motor_this_typedefs( my_app ) ;


        virtual void_t on_init( void_t ) noexcept
        {
            MOTOR_PROBE( "application", "on_init" ) ;

            {
                motor::application::window_info_t wi ;
                wi.x = 600 ;
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
            
            this->create_tcp_client( "my_client", 
                motor::network::ipv4::binding_point_host { "3456", "localhost" }, 
                motor::shared( this_file::my_client() ) ) ;
            
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

        virtual void_t on_network( network_data_in_t ) noexcept 
        { 
        }

        virtual bool_t on_tool( this_t::window_id_t const wid, motor::application::app::tool_data_ref_t ) noexcept 
        { 
            MOTOR_PROBE( "application", "on_tool" ) ;

            #if 0
            {
                if( ImGui::Begin("test window") ){}
                ImGui::End() ;
            }
            
            {
                bool_t show = true ;
                ImGui::ShowDemoWindow( &show ) ;
                ImPlot::ShowDemoWindow( &show ) ;
            }
            #endif
            return true ; 
        }

        virtual void_t on_shutdown( void_t ) noexcept {}
    };
}

int main( int argc, char ** argv )
{
    using namespace motor::core::types ;

    motor::application::carrier_mtr_t carrier = motor::platform::global_t::create_carrier(
        motor::shared( this_file::my_app() ) ) ;
    
    auto const ret = carrier->exec() ;
    
    motor::memory::release_ptr( carrier ) ;

    motor::concurrent::global::deinit() ;
    motor::log::global::deinit() ;
    motor::profiling::global::deinit() ;
    motor::memory::global::dump_to_std() ;
    

    return ret ;
}