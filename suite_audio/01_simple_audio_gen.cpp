

#include <motor/platform/global.h>

#include <motor/controls/layouts/ascii_keyboard.hpp>
#include <motor/controls/layouts/three_mouse.hpp>

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

    class my_app : public motor::application::app
    {
        motor_this_typedefs( my_app ) ;

        motor::tool::player_controller_t pc ;

        size_t cur_time = 0 ;

        float_t amp = 0.3f ;
        float_t freq = 100.0f ;
        bool_t update_buffer = true ;
        this_file::wave_type wt = this_file::wave_type::sine ;

        bool_t did_init = false ;
        motor::audio::buffer_object_t bo ;
        motor::audio::execution_options eo = motor::audio::execution_options::undefined ;
        motor::audio::result_t play_res = motor::audio::result::initial ;

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

            // prepare audio object
            {
                bo = motor::audio::buffer_object_t( "gen.sine" ) ;
                eo = motor::audio::execution_options::stop ;
            }
        }

        virtual void_t on_audio( motor::audio::frontend_ptr_t fptr, audio_data_in_t ) noexcept
        {
            if( !did_init )
            {
                did_init = true ;
                fptr->configure( &bo ) ;
            }

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