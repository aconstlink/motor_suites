#include <motor/platform/global.h>
#include <motor/application/window/window_message_listener.h>
#include <motor/tool/imgui/custom_widgets.h>
#include <motor/tool/imgui/timeline.h>
#include <motor/tool/imgui/player_controller.h>

#include <motor/log/global.h>
#include <motor/memory/global.h>

#include <motor/device/layouts/ascii_keyboard.hpp>
#include <motor/device/layouts/three_mouse.hpp>

#include <motor/tool/imgui/imgui.h>

#include <motor/concurrent/parallel_for.hpp>

#include <future>

namespace this_file
{
    enum class wave_type
    {
        sine, square, sawtooth
    };

    namespace internal
    {
        static char const * __wave_types_names [] = { "sine", "square", "sawtooth" } ;
        static int const __num_wave_types_names = sizeof( __wave_types_names ) / sizeof( __wave_types_names[0] ) ;
    }

    static char const * const to_string( wave_type const wt ) noexcept
    {
        return internal::__wave_types_names[size_t(wt)] ;
    }
}

int main( int argc, char ** argv )
{
    using namespace motor::core::types ;

    motor::application::carrier_mtr_t carrier = motor::platform::global_t::create_carrier() ;

    auto fut_update_loop = std::async( std::launch::async, [&]( void_t )
    {
        motor::application::window_message_listener_mtr_t msgl_out = motor::memory::create_ptr<
            motor::application::window_message_listener>( "[out] : message listener" ) ;

        motor::device::ascii_device_mtr_t ascii_dev = nullptr ;
        motor::device::three_device_mtr_t mouse_dev = nullptr ;

        // looking for device. It is managed, so pointer must be copied.
        carrier->device_system()->search( [&] ( motor::device::idevice_borrow_t::mtr_t dev_in )
        {
            if( auto * ptr1 = dynamic_cast<motor::device::three_device_mtr_t>(dev_in); ptr1 != nullptr )
            {
                mouse_dev = motor::share( ptr1 ) ;
            }
            else if( auto * ptr2 = dynamic_cast<motor::device::ascii_device_mtr_t>(dev_in); ptr2 != nullptr )
            {
                ascii_dev = motor::share( ptr2 ) ;
            }
        } ) ;

        {
            motor::application::window_info_t wi ;
            wi.x = 100 ;
            wi.y = 100 ;
            wi.w = 800 ;
            wi.h = 600 ;
            wi.gen = motor::application::graphics_generation::gen4_auto;

            auto wnd = carrier->create_window( wi ) ;
            wnd->register_out( motor::share( msgl_out ) ) ;
            wnd->send_message( motor::application::show_message( { true } ) ) ;
            //wnd->send_message( motor::application::vsync_message_t( { true } ) ) ;

            {
                motor::tool::imgui_t imgui ;
                motor::tool::player_controller_t pc ;

                motor::audio::buffer_object_t bo ;
                motor::audio::execution_options eo = motor::audio::execution_options::undefined ;
                motor::audio::result_t play_res = motor::audio::result::initial ;

                float_t amp = 0.3f ;
                float_t freq = 100.0f ;
                bool_t update_buffer = true ;
                this_file::wave_type wt = this_file::wave_type::sine ;

                size_t cur_time = 0 ;

                // window creation loop
                {
                    bool_t created = false ;

                    // init loop
                    while( !created ) 
                    {
                        motor::application::window_message_listener_t::state_vector_t sv ;
                        if( msgl_out->swap_and_reset( sv ) )
                        {
                            if( sv.create_changed )
                            {
                                created = true ;
                            }
                        }
                    }
                }

                // prepare audio object
                {
                    //
                    // prepare the audio buffer for playing
                    //
                    {
                        bo = motor::audio::buffer_object_t( "gen.sine" ) ;
                        eo = motor::audio::execution_options::stop ;
                    }
                }

                // configure audio object
                {
                    bool_t created = false ;

                    while( !created )
                    {
                        created = carrier->audio_system()->on_audio( [&]( motor::audio::frontend_ptr_t fptr )
                        {
                            fptr->configure( &bo ) ; ;
                        } )  ;
                    }
                }

                while( true ) 
                {
                    std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) ) ;

                    motor::application::window_message_listener_t::state_vector_t sv ;
                    if( msgl_out->swap_and_reset( sv ) )
                    {
                        if( sv.close_changed )
                        {
                            break ;
                        }

                        if( sv.resize_changed )
                        {
                            imgui.update( {(int_t)sv.resize_msg.w,(int_t)sv.resize_msg.h} ) ;
                        }
                    }
                    
                    // must be done along with the user thread due
                    // to synchronization issues with any device 
                    carrier->update_device_system() ;

                    {
                        imgui.update( motor::share( ascii_dev ) ) ;
                        imgui.update( motor::share( mouse_dev ) ) ;
                    }
                    
                    wnd->render_frame< motor::graphics::gen4::frontend_t >( [&]( motor::graphics::gen4::frontend_ptr_t fe )
                    {
                        imgui.execute( [&]( void_t )
                        {
                            if( ImGui::Begin("motor custom widgets") )
                            {
                                motor::tool::time_info_t ti {1000000, cur_time};

                                // the player_controller stores some state, 
                                // so it is defined further above
                                {
                                    auto const s = pc.do_tool( "Player" ) ;
                                    if( s == motor::tool::player_controller_t::player_state::stop )
                                    {
                                        eo = motor::audio::execution_options::stop ;
                                        cur_time = 0  ;
                                    }
                                    else if( s == motor::tool::player_controller_t::player_state::play )
                                    {
                                        eo = motor::audio::execution_options::play ;
                                        if( ti.cur_milli >= ti.max_milli ) 
                                        {
                                            cur_time = 0 ;
                                        }
                                    }
                                    else if( s == motor::tool::player_controller_t::player_state::pause )
                                    {
                                        eo = motor::audio::execution_options::pause ;
                                    }

                                    if( pc.get_state() == motor::tool::player_controller_t::player_state::play )
                                    {
                                        cur_time += 100 ;
                                    }

                                    if( cur_time > ti.max_milli )
                                    {
                                        cur_time = ti.max_milli ;
                                        pc.set_stop() ;
                                    }
                                }

                                {
                                    int value = (int)freq ;
                                    update_buffer = ImGui::SliderInt( "frequency", &value, 100, 1000 ) || update_buffer ;
                                    freq = (float_t)value ;
                                }

                                {
                                    float_t value = amp ;
                                    update_buffer = ImGui::SliderFloat( "amplitude", &value, 0.0f, 1.0f ) || update_buffer ;
                                    amp = (float_t)value ;
                                }

                                {
                                    char const ** items = this_file::internal::__wave_types_names ;
                                    int const ni = this_file::internal::__num_wave_types_names ;
                                    static int item_current = int(wt);
                                    update_buffer = ImGui::ListBox("listbox", &item_current, items, ni ) || update_buffer ;
                                    wt = this_file::wave_type(item_current) ;
                                }
                            }
                            ImGui::End() ;

                            //bool_t show = true ;
                            //ImGui::ShowDemoWindow( &show ) ;
                        } ) ;
                        imgui.render( fe ) ;
                    } ) ;

                    carrier->audio_system()->on_audio( [&]( motor::audio::frontend_ptr_t fptr )
                    {
                        motor::audio::backend::execute_detail ed ;

                        // tone generation taken from OpenAL demo
                        if( update_buffer )
                        {
                            // data: computed data
                            // a_ : amplitude
                            // sr_ : sample rate
                            // freq_ : frequency
                            auto base_funk = [&]( motor::vector< float_t > & data, float_t const a_, float_t const sr_, float_t const freq_ )
                            {
                                motor::concurrent::parallel_for<size_t>( motor::concurrent::range_1d<size_t>( 0, data.size() ), 
                                    [&]( motor::concurrent::range_1d<size_t> const & r )
                                {
                                    for( size_t i = r.begin(); i < r.end(); ++i )
                                    {
                                        double_t const b = motor::math::fn<double_t>::mod( 
                                        double_t( i ) / double_t( sr_  ), 1.0 ) ;
                        
                                        double_t const value = a_ * std::sin( freq_ * b * 2.0 * motor::math::constants<double_t>::pi() ) ;
                        
                                        data[ i ] += float_t( value ) ;
                                    }
                                } ) ;
                            } ;

                            motor::audio::channels channels = motor::audio::channels::mono ;

                            size_t const sample_rate = 44100 ;
                            size_t const num_seconds = 1 ;
                            double_t const pi = motor::math::constants<double_t>::pi();

                            motor::vector< float_t > floats( sample_rate * num_seconds ) ;
                            switch( wt ) 
                            {
                            case this_file::wave_type::sine: 
                                base_funk(floats, amp, sample_rate, freq );
                                break ;
                            case this_file::wave_type::sawtooth:
                                for(size_t i = 1;freq*i < sample_rate/2;i++)
                                    base_funk(floats, amp*float_t( 2.0/pi * ((i&1)*2 - 1.0) / i), sample_rate, freq*i);
                                break ;
                            case this_file::wave_type::square :
                                for(size_t i = 1;freq*i < sample_rate/2;i+=2)
                                    base_funk(floats, amp*float_t( 4.0/pi * 1.0/i ), sample_rate, freq*i);
                                break ;
                            }
                            bo.set_samples( channels, sample_rate, std::move( floats ) ) ;
                            ed.to = motor::audio::execution_options::stop ;
                            fptr->execute( &bo, ed ) ;
                            fptr->update( &bo ) ;
                            update_buffer = false ;
                        }

                        ed.to = eo ;
                        fptr->execute( &bo, ed ) ;
                    } )  ;
                }
            }

            motor::memory::release_ptr( wnd ) ;
        }
        
        motor::memory::release_ptr( ascii_dev ) ;
        motor::memory::release_ptr( mouse_dev ) ;

        motor::memory::release_ptr( msgl_out ) ;
    }) ;

    // end the program by closing the carrier
    auto fut_end = std::async( std::launch::async, [&]( void_t )
    {
        fut_update_loop.wait() ;
        carrier->close() ;
    } ) ;

    auto const ret = carrier->exec() ;
    
    motor::memory::release_ptr( carrier ) ;

    motor::concurrent::global_t::deinit() ;
    motor::log::global::deinit() ;
    motor::memory::global::dump_to_std() ;


    return ret ;
}