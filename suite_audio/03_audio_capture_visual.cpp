

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
#include <array>

#define USE_HISTORY_BUFFER 1

namespace this_file
{
    using namespace motor::core::types ;

    class my_app : public motor::application::app
    {
        motor_this_typedefs( my_app ) ;

        motor::audio::capture_object_t co ;

        motor::vector< float_t > captured_samples ;
        motor::vector< float_t > captured_frequencies ;

        motor::vector< float_t > captured_frequencies_avg ;
        motor::vector< float_t > captured_frequencies_var ;

        struct analysis
        {
        public: //

            int_t kick_start = 50 ;
            int_t kick_band = 70 ;

        public: // computation

            size_t kick_history_idx = size_t( -1 ) ;
            size_t midl_history_idx = size_t( -1 ) ;

            std::array< float_t, 43 > kick_history ;
            std::array< float_t, 43 > midl_history ;

            float_t kick_history_single = 0.0f ;
            float_t midl_history_single = 0.0f ;

            float_t kick_average = 0.0f ;
            float_t midl_average = 0.0f ;

            void_t write_kick( float_t const v ) noexcept
            {
                kick_history[ ++kick_history_idx % kick_history.size() ] = v ;
                //if ( kick_history_idx >= kick_history.size() ) kick_history_idx = 0 ;
            }

            void_t write_midl( float_t const v ) noexcept
            {
                midl_history[ ++midl_history_idx % midl_history.size() ] = v ;
                //if ( midl_history_idx >= midl_history.size() ) midl_history_idx = 0 ;
            }

            void_t recompute_average( void_t ) noexcept
            {
                // kick
                {
                    float_t accum = kick_history[ 0 ] ;
                    for ( size_t i = 1; i < kick_history.size(); ++i )
                    {
                        accum += kick_history[ i ] ;
                    }
                    accum /= float_t( kick_history.size() ) ;
                    kick_average = accum ;
                }

                // midl
                {
                    float_t accum = midl_history[ 0 ] ;
                    for ( size_t i = 1; i < midl_history.size(); ++i )
                    {
                        accum += midl_history[ i ] ;
                    }
                    accum /= float_t( midl_history.size() ) ;
                    midl_average = accum ;
                }
            }

            float_t compute_kick_variance( void_t ) const noexcept
            {
                float_t accum = 0.0f ;
                for ( size_t i = 0; i < kick_history.size(); ++i )
                {
                    float_t const dif = kick_history[ i ] - kick_average ;
                    float_t const sq = dif * dif ;
                    accum += sq ;
                }
                return accum / float_t( kick_history.size() ) ;
            }

            float_t compute_midl_variance( void_t ) const noexcept
            {
                float_t accum = 0.0f ;
                for ( size_t i = 0; i < midl_history.size(); ++i )
                {
                    float_t const dif = midl_history[ i ] - midl_average ;
                    float_t const sq = dif * dif ;
                    accum += sq ;
                }
                return accum / float_t( midl_history.size() ) ;
            }

        public: // for visualization 

            bool_t is_kick = false ;
            bool_t is_lowm = false ;

            float_t kick = 0.0f ;
            float_t midl = 0.0f ;
        };

        analysis asys ;

        virtual void_t on_init( void_t ) noexcept
        {
            {
                motor::application::window_info_t wi ;
                wi.x = 500 ;
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
                wi.x = 500 ;
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

            if ( captured_frequencies_avg.size() != captured_frequencies.size() )
            {
                {
                    captured_frequencies_avg.resize( captured_frequencies.size() )  ;
                    for ( size_t i = 0; i < captured_frequencies.size(); ++i )
                        captured_frequencies_avg[ i ] = 0.0f ;
                }
                {
                    captured_frequencies_var.resize( captured_frequencies.size() )  ;
                    for ( size_t i = 0; i < captured_frequencies.size(); ++i )
                        captured_frequencies_var[ i ] = 0.0f ;
                }
            }

            for ( size_t i = 0; i < captured_frequencies.size(); ++i )
            {
                {
                    captured_frequencies_avg[ i ] += captured_frequencies[ i ] ;
                    captured_frequencies_avg[ i ] /= 2.0f ;
                }

                {
                    float_t const var = captured_frequencies[ i ] - captured_frequencies_avg[ i ] ;
                    captured_frequencies_var[ i ] += var * var ;
                    captured_frequencies_var[ i ] /= 2.0f ;
                }
            }

            // [0,1] : kick frequencies
            // [2,3] : mid low frequencies
            motor::vector< size_t > const bands = 
            { 
                size_t( asys.kick_start ), 
                size_t( asys.kick_start + asys.kick_band ),
                300, 750 
            } ;
            size_t const num_bands = bands.size() >> 1 ;
            size_t const band_width = co.get_band_width() ;

            if ( band_width == 0 ) return ;

            // kick average
            {
                size_t const kick_idx_0 = bands[ 0 ] / band_width ;
                size_t const kick_idx_1 = bands[ 1 ] / band_width ;
                size_t const kick_idx_range = ( kick_idx_1 - kick_idx_0 ) + 1 ;

                float_t accum_frequencies = 0.0f ;
                for ( size_t i = kick_idx_0; i <= kick_idx_1; ++i )
                {
                    accum_frequencies += captured_frequencies[ i ] ;
                }
                accum_frequencies /= float_t( kick_idx_range ) ;

                #if USE_HISTORY_BUFFER
                asys.kick_history_single = accum_frequencies ;
                asys.write_kick( accum_frequencies ) ;
                #else
                asys.kick_history_single = accum_frequencies ;
                asys.kick_average += accum_frequencies ;
                asys.kick_average /= 2.0f ;
                #endif
            }

            // mid low average
            {
                size_t const midl_idx_0 = bands[ 2 ] / band_width ;
                size_t const midl_idx_1 = bands[ 3 ] / band_width ;
                size_t const midl_idx_range = ( midl_idx_1 - midl_idx_0 ) + 1 ;

                float_t accum_frequencies = 0.0f ;
                for ( size_t i = midl_idx_0; i <= midl_idx_1; ++i )
                {
                    accum_frequencies += captured_frequencies[ i ] ;
                }
                accum_frequencies /= float_t( midl_idx_range ) ;

                #if USE_HISTORY_BUFFER
                asys.midl_history_single = accum_frequencies ;
                asys.write_midl( accum_frequencies ) ;
                #else
                asys.midl_history_single = accum_frequencies ;
                asys.midl_average += accum_frequencies ;
                asys.midl_average /= 2.0f ;
                #endif
            }

            {
                #if USE_HISTORY_BUFFER
                asys.recompute_average() ;
                #endif
            }



            // do threshold
            {
                #if USE_HISTORY_BUFFER
                float_t const kick_var = asys.compute_kick_variance() ;
                float_t const midl_var = asys.compute_midl_variance() ;
                #else
                float_t const kick_dif = asys.kick_history_single - asys.kick_average ;
                float_t const midl_dif = asys.midl_history_single - asys.midl_average ;

                float_t const kick_var = kick_dif * kick_dif ;
                float_t const midl_var = midl_dif * midl_dif ;
                #endif
                auto beat_threshold = [&] ( float_t const v )
                {
                    return -15.0f * v + 1.55f ;
                } ;

                asys.is_kick = ( asys.kick_history_single - 0.05f ) > beat_threshold( kick_var ) * asys.kick_average ;
                asys.is_lowm = ( asys.midl_history_single - 0.005f ) > beat_threshold( midl_var ) * asys.midl_average ;
            }

            // do value degradation
            {
                if ( asys.is_kick ) asys.kick = 1.0f ;
                if ( asys.is_lowm ) asys.midl = 1.0f ;

                float_t const deg = ad.sec_dt * 0.5f ;

                asys.kick -= deg ;
                asys.midl -= deg ;

                asys.kick = std::max( asys.kick, 0.0f ) ;
                asys.midl = std::max( asys.midl, 0.0f ) ;
            }
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

        virtual bool_t on_tool( this_t::window_id_t const wid, motor::application::app::tool_data_ref_t td ) noexcept 
        { 
            if ( ImGui::Begin( "Capture Control" ) )
            {
                {
                    ImGui::SliderInt( "Kick Start", &asys.kick_start, 0, 150 ) ;
                    ImGui::SliderInt( "Kick Band", &asys.kick_band, 10, 150 ) ;
                }
            }
            ImGui::End() ;

            if( ImGui::Begin("Capture Audio") )
            {
                
                // print wave form
                {
                    auto const mm = co.minmax() ;
                    ImGui::PlotLines( "Samples", captured_samples.data(), ( int ) captured_samples.size(), 0, 0, mm.x(), mm.y(), ImVec2( ImGui::GetWindowWidth(), 100.0f ) );
                }

                // print frequencies captured
                {
                    ImGui::PlotHistogram( "Frequencies Captured", captured_frequencies.data(),
                        (int) captured_frequencies.size(), 0, 0, 0.0f, 1.0f, ImVec2( ImGui::GetWindowWidth(), 100.0f ) ) ;
                }

                // print frequencies captured 4th - quater size
                {
                    ImGui::PlotHistogram( "Frequencies Captured 4", captured_frequencies.data(),
                        (int) (captured_frequencies.size() >> 2), 0, 0, 0.0f, 1.0f, ImVec2( ImGui::GetWindowWidth(), 100.0f ) ) ;
                }

                // print frequencies average
                {
                    ImGui::PlotHistogram( "Frequencies Average", captured_frequencies_avg.data(),
                        ( int ) captured_frequencies_avg.size() , 0, 0, 0.0f, 1.0f, ImVec2( ImGui::GetWindowWidth(), 100.0f ) ) ;                    
                }

                // print frequencies variance
                {
                    ImGui::PlotHistogram( "Frequencies Variance", captured_frequencies_var.data(),
                        (int) captured_frequencies_var.size(), 0, 0, 0.0f, 1.0f, ImVec2( ImGui::GetWindowWidth(), 100.0f ) ) ;
                }

                {
                    ImGui::Checkbox( "Kick", &asys.is_kick ) ; ImGui::SameLine() ;
                    ImGui::Checkbox( "Mid", &asys.is_lowm ) ; ImGui::SameLine() ;
                }

                {
                    ImGui::VSliderFloat( "Kick Value", ImVec2( 18, 160 ),  &(asys.kick), 0.0f, 1.0f ) ; ImGui::SameLine() ;
                    ImGui::VSliderFloat( "Midl Value", ImVec2( 18, 160 ), &( asys.midl ), 0.0f, 1.0f ) ; 
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