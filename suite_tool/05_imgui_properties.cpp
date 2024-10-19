

#include <motor/tool/imgui/imgui_property.h>

#include <motor/profiling/probe_guard.hpp>

#include <motor/platform/global.h>

#include <motor/controls/types/ascii_keyboard.hpp>
#include <motor/controls/types/three_mouse.hpp>

#include <motor/property/generic_property.hpp>
#include <motor/property/property_sheet.hpp>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>

#include <future>

namespace this_file
{
    using namespace motor::core::types ;

    enum class test_enum
    {
        invalid,
        enum_1,
        enum_2,
        enum_3,
        max_entries
    };
    using my_enum_property_tmp = motor::property::generic_property< test_enum, true > ;
    motor_typedefs( my_enum_property_tmp, my_enum_property ) ;
    

    namespace detail
    {
        static char_cptrc_t  __test_enum_strings__[] = {"invalid", "enum_1", "enum_2", "enum_3"} ;

        static size_t num_entries( void_t ) noexcept
        {
            return sizeof( detail::__test_enum_strings__ ) / sizeof( detail::__test_enum_strings__[ 0 ] ) ;
        }
    }

    static char_cptr_t to_string( test_enum const e ) noexcept 
    {
        return detail::__test_enum_strings__[ size_t(e) >= size_t(test_enum::max_entries) ? 0 : size_t(e) ]  ;
    }

    static my_enum_property_t::strings_funk_t my_enum_strings_funk_t = [] ( void_t )
    {
        return std::make_pair( this_file::detail::__test_enum_strings__, this_file::detail::num_entries() ) ;
    } ;
}

namespace this_file
{
    using namespace motor::core::types ;

    class my_app : public motor::application::app
    {
        motor_this_typedefs( my_app ) ;

        motor::property::property_sheet_t _props1 ;
        motor::property::property_sheet_t _props2 ;

        //******************************************************************************************************
        virtual void_t on_init( void_t ) noexcept
        {
            // #1 : init window
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

            // if the enum should be viewable in any gui, this must be done.
            my_enum_property_t::set_strings_funk( this_file::my_enum_strings_funk_t ) ;

            {
                _props1.add_property( "some_int_value", motor::property::int_property_t( 7, { -10, 100 } ) ) ;
                _props1.add_property( "some_float_value", motor::property::float_property_t( -4, { -16, 50 } ) ) ;
                _props1.add_property( "my_enum", this_file::my_enum_property_t( this_file::test_enum::enum_1 ) ) ;
                _props1.add_property( "my_string", motor::property::string_property_t( "Hello World" ) ) ;
            }

            {
                _props2.add_property( "some_int_value", motor::property::int_property_t( 5, { -5, 5 } ) ) ;
                _props2.add_property( "some_float_value", motor::property::float_property_t( 5, { -10, 50 } ) ) ;
                _props2.add_property( "my_enum", this_file::my_enum_property_t( this_file::test_enum::enum_3 ) ) ;
                _props2.add_property( "my_string", motor::property::string_property_t( "Hello World again" ) ) ;
            }
        }

        //******************************************************************************************************
        virtual void_t on_event( window_id_t const wid, 
                motor::application::window_message_listener::state_vector_cref_t sv ) noexcept
        {
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

        //******************************************************************************************************
        virtual bool_t on_tool( this_t::window_id_t const wid, motor::application::app::tool_data_ref_t ) noexcept 
        { 
            MOTOR_PROBE( "application", "on_tool" ) ;

            {
                if( ImGui::Begin("property window") )
                {
                    motor::tool::imgui_property::handle( "my property sheet 1", _props1 ) ;
                    motor::tool::imgui_property::handle( "my property sheet 2", _props2 ) ;
                }
                ImGui::End() ;
            }
            
            {
                bool_t show = true ;
                ImGui::ShowDemoWindow( &show ) ;
            }

            return true ; 
        }

        //******************************************************************************************************
        virtual void_t on_shutdown( void_t ) noexcept {}
    };
}

int main( int argc, char ** argv )
{
    return motor::platform::global_t::create_and_exec< this_file::my_app >() ;
}