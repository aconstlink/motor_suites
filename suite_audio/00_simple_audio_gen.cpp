
#include <motor/platform/global.h>

#include <motor/controls/layouts/ascii_keyboard.hpp>
#include <motor/controls/layouts/three_mouse.hpp>

#include <motor/tool/structs.h>
#include <motor/tool/imgui/player_controller.h>

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

        motor::tool::player_controller_t pc ;

        size_t cur_time = 0 ;

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
                //
                // prepare the audio buffer for playing
                //
                {
                    bo = motor::audio::buffer_object_t( "gen.sine" ) ;
                
                    motor::audio::channels channels = motor::audio::channels::stereo ;

                    size_t const sample_rate = 96000 ;
                    size_t const num_channels = motor::audio::to_number( channels ) ;
                    size_t const num_seconds = 4 ;

                    size_t const freq = 100 ;

                    {
                        motor::vector< float_t > floats( sample_rate * num_seconds * num_channels ) ;

                        for( size_t i = 0; i < floats.size()/num_channels; ++i )
                        {
                            size_t const idx = i * num_channels ;
                            double_t const a = 0.3f ;
                            double_t const f = motor::math::fn<double_t>::mod( 
                                double_t( i ) / double_t( sample_rate-num_channels  ), 1.0 ) ;
                        
                            double_t const value = a * std::sin( freq * f * 2.0 * motor::math::constants<double_t>::pi() ) ;
                        
                            floats[ idx + 0 ] = float_t( value ) ;
                            floats[ idx + 1 ] = float_t( value ) ;
                        }
                        bo.set_samples( channels, sample_rate, floats ) ;
                    }

                    eo = motor::audio::execution_options::stop ;
                }
            }
        }

        virtual void_t on_audio( motor::audio::frontend_ptr_t fptr, audio_data_in_t ) noexcept
        {
            if( !did_init )
            {
                did_init = true ;
                fptr->configure( &bo ) ;
            }

            {
                motor::audio::backend::execute_detail ed ;
                ed.to = eo ;
                fptr->execute( &bo, ed ) ;
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

        virtual bool_t on_tool( this_t::window_id_t const wid, motor::application::app::tool_data_ref_t ) noexcept 
        { 
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
                }
                ImGui::End() ;
            }
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