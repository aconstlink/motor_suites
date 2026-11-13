
#include <motor/platform/global.h>

#include <motor/gfx/primitive/primitive_render_3d.h>

#include <motor/math/utility/fn.hpp>
#include <motor/math/utility/angle.hpp>
#include <motor/math/animation/keyframe_sequence.hpp>

#include <motor/tool/imgui/imgui_property.h>

#include <motor/wire/kit/trigger_filter_node.hpp>
#include <motor/wire/slot/output_slot.h>

#include <motor/log/global.h>
#include <motor/memory/global.h>
#include <motor/concurrent/global.h>
#include <motor/format/global.h>

#include <future>
#include <semaphore>

// testing the new trigger_filter_node from wire.
namespace this_file
{
using namespace motor::core::types;

class my_app : public motor::application::app
{
    motor_this_typedefs( my_app );

    motor::gfx::primitive_render_3d_mtr_t _prim_render = nullptr;  

  private: // wire

    using os_float_t = motor::wire::output_slot< float_t >;
    using os_trafo_t = motor::wire::output_slot< motor::math::m3d::trafof_t >;

    // output slot for the imported scene animations.
    os_float_t * _time = nullptr;

    // allows to scale the whole scene. it is connected
    // to the root transformation in the imported sceen.
    os_trafo_t * _scale_os = nullptr;

    motor::wire::time_node_mtr_t _time_node = nullptr;
    motor::wire::inode_mtr_t _merger = nullptr;
    std::atomic< bool_t > _synced = false;

    // store all the wire nodes for easier destruction.
    motor::vector< motor::wire::inode_mtr_t > _node_dump;

    motor::wire::trigger_filter_node_mtr_t _animation_filters_node =
        motor::shared( motor::wire::trigger_filter_node_t() );

    bool_t _auto_trigger = true;
    float_t _auto_trigger_interval = 3.0f;

    size_t f0 = size_t( -1 );
    size_t f1 = size_t( -1 );
    size_t f2 = size_t( -1 );
    size_t f3 = size_t( -1 );
    size_t f4 = size_t( -1 );
    size_t f5 = size_t( -1 );
    size_t f6 = size_t( -1 );

  public:

    my_app( void_t ) noexcept {}
    my_app( this_cref_t ) = delete;
    my_app( this_rref_t rhv ) noexcept
        : _time( motor::move( rhv._time ) ), _scale_os( motor::move( rhv._scale_os ) ),
          _time_node( motor::move( rhv._time_node ) ), _merger( motor::move( rhv._merger ) ),
          _node_dump( std::move( rhv._node_dump ) ),
          _animation_filters_node( motor::move( rhv._animation_filters_node ) )
    {
    }

    virtual ~my_app( void_t ) noexcept
    {
        motor::release( motor::move( _animation_filters_node ) );
        motor::release( motor::move( _merger ) );
    }

    

    virtual void_t on_init( void_t ) noexcept
    {

        {
            _time = motor::shared( os_float_t( 0.0f ) );
            _scale_os = motor::shared( os_trafo_t() );
        }

#if 0
        {
            motor::application::window_info_t wi;
            wi.x = 100;
            wi.y = 100;
            wi.w = 1920 >> 1;
            wi.h = 1080 >> 1;
            wi.gen = motor::application::graphics_generation::gen4_auto;

            this_t::send_window_message( this_t::create_window( wi ),
                [ & ]( motor::application::app::window_view & wnd )
            {
                wnd.send_message( motor::application::show_message( { true } ) );
                wnd.send_message( motor::application::cursor_message_t( { true } ) );
                wnd.send_message( motor::application::vsync_message_t( { true } ) );
            } );
        }
#endif
#if 1
        {
            motor::application::window_info_t wi;
            wi.x = 100 + ( 1920 >> 1 );
            wi.y = 100;
            wi.w = 1920 >> 1;
            wi.h = 1080 >> 1;
            wi.gen = motor::application::graphics_generation::gen4_gl4;

            this_t::send_window_message( this_t::create_window( wi ),
                [ & ]( motor::application::app::window_view & wnd )
            {
                wnd.send_message( motor::application::show_message( { true } ) );
                wnd.send_message( motor::application::cursor_message_t( { true } ) );
                wnd.send_message( motor::application::vsync_message_t( { true } ) );
            } );
        }
#endif

        {
            motor::gfx::primitive_render_3d_t prim_render;
            prim_render.init( "my_debug_render" );
            _prim_render = motor::shared( std::move( prim_render ) );
        }

        // would be linear with input signal
        {
            f0 = _animation_filters_node->add_filter(
                []( motor::wire::trigger_filter_node_t::filter_data const & d )
            { return d.sig_abs; }, {} );
            _animation_filters_node->connect_signal_is( f0, motor::share( _time ) );
        }

        // should always be 1
        {
            f1 = _animation_filters_node->add_filter(
                []( motor::wire::trigger_filter_node_t::filter_data const & ) { return 1.0f; },
                {} );
            _animation_filters_node->connect_signal_is( f1, motor::share( _time ) );
        }

        // should be a linear sawtooth in the interval [0;2]
        {
            f2 = _animation_filters_node->add_filter(
                []( motor::wire::trigger_filter_node_t::filter_data const & d )
            { return motor::math::fn< float_t >::mod( d.sig_abs, 2.0f ); }, {} );
            _animation_filters_node->connect_signal_is( f2, motor::share( _time ) );
        }

        // should be a constant with duration
        {
            f3 = _animation_filters_node->add_filter(
                []( motor::wire::trigger_filter_node_t::filter_data const & ) { return 5.0f; },
                {} );
            _animation_filters_node->connect_signal_is( f3, motor::share( _time ) );
            _animation_filters_node->set_duration( f3, 0.5f );
            _animation_filters_node->reset_on_detrigger( f3, true );
        }

        // should be a exponential falloff from y : [amplitutde to 0] in x :[0;duration]
        {
            f4 = _animation_filters_node->add_filter(
                []( motor::wire::trigger_filter_node_t::filter_data const & d )
            {
                float_t const u = ( d.time_rel ) / d.duration;
                float_t const k = 3.0f;
                float_t const e = std::exp( -k );
                return 20.0f * ( std::exp( -k * u ) - e ) / ( 1.0f - e );
            }, {} );
            _animation_filters_node->connect_signal_is( f4, motor::share( _time ) );
            _animation_filters_node->set_duration( f4, 2.0f );
        }

        // should be sine wave
        {
            f5 = _animation_filters_node->add_filter(
                []( motor::wire::trigger_filter_node_t::filter_data const & d )
            {
                return d.duration * std::sin( ( ( d.sig_abs - d.sig_start ) / d.duration ) * 2.0f *
                                              3.143652834f ) +
                       d.duration;
            }, { 5.0f, motor::wire::trigger_filter_node_t::trigger_behavoir::reset_on_trigger } );
            _animation_filters_node->connect_signal_is( f5, motor::share( _time ) );
            // is set via add_filter
            //_animation_filters_node->set_duration( f5, 5.0f );
        }

        // should be triangle
        {
            f6 = _animation_filters_node->add_filter(
                []( motor::wire::trigger_filter_node_t::filter_data const & d )
            {
                float_t const dh = d.duration * 0.5f;
                float_t const sig = d.sig_abs - d.sig_start;
                return 30.0f * ( ( dh - std::abs( sig - dh ) ) ) / dh;
            }, {} );
            _animation_filters_node->connect_signal_is( f6, motor::share( _time ) );
            _animation_filters_node->set_duration( f6, 1.0f );
        }

        {
            _animation_filters_node->connect_time_is( motor::share( _time ) );
            // need to trigger once so filters can be active
            _animation_filters_node->trigger();
        }

        {
            _merger = motor::shared( motor::wire::funk_node_t(
                "merger", [ & ]( motor::wire::funk_node_ptr_t ) { _synced.exchange( true ); } ) );
            _animation_filters_node->then( motor::share( _merger ) );
        }
    }

    virtual void_t on_event( window_id_t const wid,
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
        if( sv.resize_changed )
        {
            float_t const w = float_t( sv.resize_msg.w );
            float_t const h = float_t( sv.resize_msg.h );
        }
    }

    //******************************************************************************************************
    virtual void_t on_update( motor::application::app::update_data_in_t ud ) noexcept
    {
        // 4 seconds animation is in sync with the blender export
        {

            _time->set_and_exchange( _time->get_value() + ud.sec_dt );
        }

        if( _auto_trigger )
        {

            if( motor::math::fn< float_t >::mod( _time->get_value(), _auto_trigger_interval ) <=
                0.01f )
                _animation_filters_node->trigger();
        }

        {
            motor::concurrent::global_t::schedule(
                _animation_filters_node->get_task(), motor::concurrent::schedule_type::pool );
        }

        while( !_synced );
        _synced = false;
    }

    //******************************************************************************************************
    virtual void_t on_graphics( motor::application::app::graphics_data_in_t gd ) noexcept
    {

        // use the debug_graphics_visitor_t and render the scene
        {

            //_prim_render->set_view_proj( cam->get_view_matrix(), cam->get_proj_matrix() );
            _prim_render->prepare_for_rendering();
        }
    }

    virtual void_t on_first_frame( this_t::window_id_t const wid,
        motor::graphics::gen4::frontend_ptr_t fe,
        motor::application::app::render_data_in_t ) noexcept
    {
        _prim_render->configure( fe );
    }

    virtual void_t on_last_frame( this_t::window_id_t const wid,
        motor::graphics::gen4::frontend_ptr_t fe,
        motor::application::app::render_data_in_t ) noexcept
    {
        _prim_render->release( fe );
    }

    virtual void_t on_render( this_t::window_id_t const wid,
        motor::graphics::gen4::frontend_ptr_t fe,
        motor::application::app::render_data_in_t ) noexcept
    {
        _prim_render->prepare_for_rendering( fe );
        _prim_render->render( fe );
    }

    //************************************************************************************
    virtual void_t on_frame_done( void_t ) noexcept {}

    //******************************************************************************************************
    bool_t on_tool(
        this_t::window_id_t const wid, motor::application::app::tool_data_ref_t td ) noexcept
    {
#if 0
        {
            bool_t show = true;
            ImGui::ShowDemoWindow( &show );
            ImPlot::ShowDemoWindow( &show );
        }
#endif
        if( ImGui::Begin( "Some stats" ) )
        {
            if( ImGui::Button( "trigger" ) )
            {
                _animation_filters_node->trigger();
            }
            ImGui::SameLine();
            ImGui::Checkbox( "Auto Trigger", &_auto_trigger );
            ImGui::SameLine();
            ImGui::SliderFloat( "Auto Trigger Interval", &_auto_trigger_interval, -1.0f, 5.0f );

            if( ImPlot::BeginPlot( "Line Plots" ) )
            {
                static const size_t max_values = 1000;
                static size_t cur_pos = size_t( -1 );
                cur_pos = ++cur_pos % max_values;
                if( cur_pos == 0 )
                {
                    _time->set_and_exchange( 0.0f );
                }

                ImPlot::SetupAxes( "x", "y" );
                ImPlot::SetupAxesLimits( 0.0, 20.0, -1.0, 20.0, ImPlotCond_Once );

                {
                    static std::array< float_t, max_values > xs1;
                    static std::array< float_t, max_values > ys1;

                    xs1[ cur_pos ] = _time->get_value();
                    ys1[ cur_pos ] = _animation_filters_node->get_filter_output_signal( f0 );

                    ImPlot::PlotLine( "f(x)", xs1.data(), ys1.data(), int_t( cur_pos ) );
                }

                {
                    static std::array< float_t, max_values > xs1;
                    static std::array< float_t, max_values > ys1;

                    xs1[ cur_pos ] = _time->get_value();
                    ys1[ cur_pos ] = _animation_filters_node->get_filter_output_signal( f1 );

                    ImPlot::PlotLine( "f1(x)", xs1.data(), ys1.data(), int_t( cur_pos ) );
                }

                {
                    static std::array< float_t, max_values > xs1;
                    static std::array< float_t, max_values > ys1;

                    xs1[ cur_pos ] = _time->get_value();
                    ys1[ cur_pos ] = _animation_filters_node->get_filter_output_signal( f2 );

                    ImPlot::PlotLine( "f2(x)", xs1.data(), ys1.data(), int_t( cur_pos ) );
                }

                {
                    static std::array< float_t, max_values > xs1;
                    static std::array< float_t, max_values > ys1;

                    xs1[ cur_pos ] = _time->get_value();
                    ys1[ cur_pos ] = _animation_filters_node->get_filter_output_signal( f3 );

                    ImPlot::PlotLine( "f3(x)", xs1.data(), ys1.data(), int_t( cur_pos ) );
                }

                {
                    static std::array< float_t, max_values > xs1;
                    static std::array< float_t, max_values > ys1;

                    xs1[ cur_pos ] = _time->get_value();
                    ys1[ cur_pos ] = _animation_filters_node->get_filter_output_signal( f4 );

                    ImPlot::PlotLine( "f4(x)", xs1.data(), ys1.data(), int_t( cur_pos ) );
                }

                {
                    static std::array< float_t, max_values > xs1;
                    static std::array< float_t, max_values > ys1;

                    xs1[ cur_pos ] = _time->get_value();
                    ys1[ cur_pos ] = _animation_filters_node->get_filter_output_signal( f5 );

                    ImPlot::PlotLine( "f5(x)", xs1.data(), ys1.data(), int_t( cur_pos ) );
                }

                {
                    static std::array< float_t, max_values > xs1;
                    static std::array< float_t, max_values > ys1;

                    xs1[ cur_pos ] = _time->get_value();
                    ys1[ cur_pos ] = _animation_filters_node->get_filter_output_signal( f6 );

                    ImPlot::PlotLine( "f6(x)", xs1.data(), ys1.data(), int_t( cur_pos ) );
                }

                ImPlot::EndPlot();
            }
        }
        ImGui::End();

        
        return true;
    }

    //******************************************************************************************************
    void_t on_shutdown( void_t ) noexcept
    {
        release_all_objects();
    }

  private:

    void_t release_node_dump( void_t ) noexcept
    {
        for( auto * ptr : _node_dump )
        {
            ptr->disconnect();
            motor::release( motor::move( ptr ) );
        }
        _node_dump.clear();
    }

    void_t release_all_objects( void_t ) noexcept
    {
        motor::wire::release( motor::move( _time ) );
        motor::wire::release( motor::move( _scale_os ) );
        motor::wire::release( motor::move( _time_node ) );
        motor::wire::release( motor::move( _merger ) );

        this_t::release_node_dump();

        motor::release( motor::move( _prim_render ) );
    }
};
} // namespace this_file

int main( int argc, char ** argv )
{
    using namespace motor::core::types;

    motor::application::carrier_mtr_t carrier =
        motor::platform::global_t::create_carrier( motor::shared( this_file::my_app() ) );

    auto const ret = carrier->exec();

    motor::memory::release_ptr( carrier );

    motor::io::global::deinit();
    motor::concurrent::global::deinit();
    motor::log::global::deinit();
    motor::memory::global::dump_to_std();

    return ret;
}
