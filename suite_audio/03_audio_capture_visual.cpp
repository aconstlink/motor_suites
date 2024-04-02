

#include <motor/platform/global.h>

#include <motor/audio/object/capture_object.hpp>

#include <motor/controls/types/ascii_keyboard.hpp>
#include <motor/controls/types/three_mouse.hpp>

#include <motor/tool/structs.h>
#include <motor/tool/imgui/player_controller.h>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>
#include <motor/concurrent/parallel_for.hpp>

#include <future>

namespace this_file
{
    using namespace motor::core::types ;

    class my_app : public motor::application::app
    {
        motor_this_typedefs( my_app ) ;

        motor::audio::capture_object_t co ;

        motor::vector< float_t > captured_samples ;
        motor::vector< float_t > captured_frequencies ;

        virtual void_t on_init( void_t ) noexcept
        {
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
            {
                motor::application::window_info_t wi ;
                wi.x = 100 ;
                wi.y = 100 ;
                wi.w = 800 ;
                wi.h = 600 ;
                wi.gen = motor::application::graphics_generation::gen4_gl4 ;
                
                this_t::send_window_message( this_t::create_window( wi ), [&]( motor::application::app::window_view & wnd )
                {
                    wnd.send_message( motor::application::show_message( { true } ) ) ;
                    wnd.send_message( motor::application::cursor_message_t( {true} ) ) ;
                    wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                } ) ;
            }
        }

        virtual void_t on_audio( motor::audio::frontend_ptr_t fptr, audio_data_in_t ad ) noexcept
        {
            if( ad.first_frame )
            {
                fptr->configure( motor::audio::capture_type::what_u_hear, &co ) ;
            }

            fptr->capture( &co ) ;
            co.copy_samples_to( captured_samples ) ;
            co.copy_frequencies_to( captured_frequencies ) ;
        }

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

        virtual bool_t on_tool( this_t::window_id_t const wid, motor::application::app::tool_data_ref_t ) noexcept 
        { 
            if( ImGui::Begin("Capture Audio") )
            {
                
                // print wave form
                {
                    auto const mm = co.minmax() ;
                    ImGui::PlotLines( "Samples", captured_samples.data(), ( int ) captured_samples.size(), 0, 0, mm.x(), mm.y(), ImVec2( ImGui::GetWindowWidth(), 100.0f ) );
                }

                // print frequencies
                {
                    float_t max_value = std::numeric_limits<float_t>::min() ;
                    for( size_t i = 0 ; i < captured_frequencies.size(); ++i )
                        max_value = std::max( captured_frequencies[ i ], max_value ) ;

                    static float_t smax_value = 0.0f ;
                    float_t const mm = ( max_value + smax_value ) * 0.5f;
                    ImGui::PlotHistogram( "Frequencies", captured_frequencies.data(), ( int ) captured_frequencies.size() / 4, 0, 0, 0.0f, mm, ImVec2( ImGui::GetWindowWidth(), 100.0f ) );
                    smax_value = max_value ;
                }
            }
            ImGui::End() ;

            return true ; 
        }
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
    motor::memory::global::dump_to_std() ;


    return ret ;
}