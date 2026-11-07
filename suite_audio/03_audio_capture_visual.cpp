#include <motor/platform/global.h>

#include <motor/audio/util/frequency_bands.hpp>
#include <motor/audio/util/onset_detector.hpp>
#include <motor/audio/object/capture_object.hpp>
#include <motor/tool/structs.h>
#include <motor/tool/imgui/onset_meter.hpp>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>

#include <algorithm>
#include <cmath>

namespace motor::audio
{
using namespace motor::core::types;

} // namespace motor::audio

namespace audio_widgets
{
using namespace motor::core::types;

} // namespace audio_widgets

namespace this_file
{
using namespace motor::core::types;

class my_app : public motor::application::app
{
    motor_this_typedefs( my_app );

    motor::audio::capture_object_t co;
    motor::vector< float_t > captured_samples;
    motor::vector< float_t > captured_frequencies;

    motor::audio::envelope_t kix;
    motor::audio::envelope_t mids;
    motor::audio::envelope_t his;

    motor::audio::onset_detector kix_detector;
    motor::audio::onset_detector mid_detector;

    motor::tool::imgui_onset_meter low_onset_widget;
    motor::tool::imgui_onset_meter mid_onset_widget;

    int_t low_onset_counter = 0;

    virtual void_t on_init( void_t ) noexcept
    {
        motor::application::window_info_t wi;
        wi.x = 500;
        wi.y = 100;
        wi.w = 800;
        wi.h = 600;
        wi.gen = motor::application::graphics_generation::gen4_auto;

        this_t::send_window_message( this_t::create_window( wi ),
            []( motor::application::app::window_view & wnd )
        {
            wnd.send_message( motor::application::show_message( { true } ) );
            wnd.send_message( motor::application::cursor_message_t( { true } ) );
            wnd.send_message( motor::application::vsync_message_t( { true } ) );
        } );
    }

    virtual void_t on_audio( motor::audio::frontend_ptr_t fptr, audio_data_in_t ad ) noexcept
    {
        if( ad.first_frame )
        {
            fptr->configure( motor::audio::capture_type::what_u_hear, &co );
        }

        fptr->capture( &co );
        co.copy_samples_to( captured_samples );
        co.copy_frequencies_to( captured_frequencies );

        float_t const bin_width = co.get_bin_width_hz< float_t >();

        if( captured_frequencies.empty() ) return;

        motor::audio::frequency_bands fb( bin_width, captured_frequencies.size() );

        // low / kix ( 40 ; 250 Hz )
        {
            float_t sum = 0.0f;
            fb.for_each( 40.0f, 250.0f,
                [ & ]( size_t const, size_t const g ) { sum += captured_frequencies[ g ]; } );

            // Keep the raw meter and filtered signal in the same squared-amplitude units.
            kix.update( sum, ad.sec_dt );

            if( kix_detector.update( sum, ad.sec_dt ) )
            {
                // detected
            }
        }

        // mid ( 250 ; 2000 Hz )
        {
            float_t sum = 0.0f;
            fb.for_each( 250.0f, 2000.0f,
                [ & ]( size_t const, size_t const g ) { sum += captured_frequencies[ g ]; } );

            // Keep the raw meter and filtered signal in the same squared-amplitude units.
            mids.update( sum, ad.sec_dt );

            if( mid_detector.update( sum, ad.sec_dt ) )
            {
                // detected
            }
        }
    }

    virtual void_t on_event( window_id_t const,
        motor::application::window_message_listener::state_vector_cref_t sv ) noexcept
    {
        if( sv.create_changed )
        {
            motor::log::global_t::status( "[my_app] : window created" );
        }

        if( sv.close_changed )
        {
            motor::log::global_t::status( "[my_app] : window closed" );
            this->close();
        }
    }

    virtual bool_t on_tool(
        this_t::window_id_t const, motor::application::app::tool_data_ref_t ) noexcept
    {
        if( ImGui::Begin( "Detection" ) )
        {
            {
                ImGui::SameLine();
                low_onset_widget.draw( "40\n250\nHz", kix_detector );
            }
            {
                ImGui::SameLine();
                mid_onset_widget.draw( "250\n2000\nHz", mid_detector );
            }
            {
                float_t peak = 0.0000000001f;
                for( float_t const value : captured_frequencies )
                {
                    peak = std::max( peak, value );
                }
                ImGui::PlotHistogram( "Squared amplitude", captured_frequencies.data(),
                    int_t( captured_frequencies.size() ) >> 2, 0, nullptr, 0.0f, peak * 1.05f,
                    ImVec2( ImGui::GetContentRegionAvail().x, 50.0f ) );
            }
            {
                float_t peak = 0.00001f;
                for( float_t const sample : captured_samples )
                {
                    peak = std::max( peak, std::abs( sample ) );
                }
                float_t const scale = peak * 1.05f;
                ImGui::PlotLines( "Samples", captured_samples.data(),
                    int_t( captured_samples.size() ), 0, nullptr, -scale, scale,
                    ImVec2( ImGui::GetContentRegionAvail().x, 50.0f ) );
            }
        }
        ImGui::End();

        return true;
    }
};
} // namespace this_file

int main( motor::core::types::void_t )
{
    using namespace motor::core::types;

    motor::application::carrier_mtr_t carrier =
        motor::platform::global_t::create_carrier( motor::shared( this_file::my_app() ) );

    auto const ret = carrier->exec();

    motor::memory::release_ptr( carrier );

    motor::concurrent::global::deinit();
    motor::log::global::deinit();
    motor::memory::global::dump_to_std();

    return ret;
}
