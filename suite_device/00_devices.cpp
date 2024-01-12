
#include <motor/device/system.h>
#include <motor/device/imodule.h>
#include <motor/device/layouts/three_mouse.hpp>
#include <motor/device/layouts/ascii_keyboard.hpp>

#include <motor/math/vector/vector2.hpp>
#include <motor/math/interpolation/interpolate.hpp>

#include <chrono>

using namespace motor::core::types ;

namespace this_file
{
    // emulates some ascii and mouse events.
    // pure test scenario and shows how to use the device system
    // module for user implemented modules.
    class dummy_module : public motor::device::imodule
    {
        motor_this_typedefs( dummy_module ) ;

    private:

        motor::device::three_device_mtr_t mouse_dev = nullptr ;
        motor::device::ascii_device_mtr_t ascii_dev = nullptr ;       

        using clock_t = std::chrono::high_resolution_clock ;
        clock_t::time_point _tp ;
        clock_t::time_point _tp_beg ;

    public:

        dummy_module( void_t ) noexcept
        {
            mouse_dev = motor::memory::create_ptr( motor::device::three_device_t(), "[dummy_module] : mouse" ) ;
            ascii_dev = motor::memory::create_ptr( motor::device::ascii_device_t(), "[dummy_module] : ascii" ) ;

            _tp_beg = clock_t::now() ;
        }
        
        dummy_module( this_cref_t rhv ) = delete ;
        dummy_module( this_rref_t rhv ) noexcept
        {
            mouse_dev = motor::move( rhv.mouse_dev ) ;
            ascii_dev = motor::move( rhv.ascii_dev ) ;
            _tp = std::move( rhv._tp ) ;
            _tp_beg = std::move( rhv._tp_beg ) ;
        }

        ~dummy_module() noexcept
        {
            this_t::release() ;
        }

    protected:

        // call funk for each supported/created device
        virtual void_t search( motor::device::imodule::search_funk_t funk ) noexcept 
        {
            funk( motor::share(mouse_dev) ) ;
            funk( motor::share(ascii_dev) ) ;
        }

        virtual void_t update( void_t ) noexcept 
        {
            // update all devices at the beginning 
            // of the update loop so the user will 
            // receive the actual event. Otherwise
            // the event might be consumed by some
            // device/component logic.
            {
                mouse_dev->update() ;
                ascii_dev->update() ;
            }

            auto tp = clock_t::now() ;
            auto dur = tp - _tp ;
            
            if( dur < std::chrono::milliseconds(20) ) return ;

            // since beginning
            auto dur2 = tp - _tp_beg ;
            
            using vec_t = motor::math::vec2f_t ;
            
            // 0 - 2 seconds do...
            if( dur2 > std::chrono::seconds( 0 ) && dur2 < std::chrono::seconds( 2 ) )
            {
                motor::device::layouts::three_mouse_t tm( mouse_dev ) ;

                {
                    if( auto * comp = tm.get_component( motor::device::layouts::three_mouse_t::button::right ) ; comp != nullptr )
                    {
                        *comp = motor::device::components::button_state::pressed ;
                    }
                }
            }

            // disable mouse state
            if( dur2 > std::chrono::seconds( 2 ) )
            {
                motor::device::layouts::three_mouse_t tm( mouse_dev ) ;
                if( auto * comp = tm.get_component( motor::device::layouts::three_mouse_t::button::right ) ; comp != nullptr )
                {
                    if( comp->state() == motor::device::components::button_state::pressed ||
                        comp->state() == motor::device::components::button_state::pressing )
                    {
                        *comp = motor::device::components::button_state::released ;
                    }
                }
            }

            // 2 - 4 seconds do...
            if( dur2 > std::chrono::seconds( 2 ) && dur2 < std::chrono::seconds( 4 ) ) 
            {
                motor::device::layouts::three_mouse_t tm( mouse_dev ) ;

                // change global coord
                {
                    float_t const t = float_t( double_t( std::chrono::duration_cast<std::chrono::milliseconds>
                        (dur2).count() ) / 1000.0 ) ;

                    auto coord = motor::math::interpolation<vec_t>::linear( vec_t(-5.0f, -5.0f), 
                        vec_t(5.0f, 5.0f), (t-2.0f) * 0.5f ) ;

                    *tm.get_global_component() = coord ;
                }

                // change local coord
                {
                    float_t const t = float_t( double_t( std::chrono::duration_cast<std::chrono::milliseconds>
                        (dur2).count() ) / 1000.0 ) ;

                    auto coord = motor::math::interpolation<vec_t>::linear( vec_t(-1.0f, -1.0f), 
                        vec_t(1.0f, 1.0f), (t-2.0f) * 0.5f ) ;

                    *tm.get_local_component() = coord ;
                }
            }

            // > 4 seconds do...
            // send mouse exit button press combo
            if( dur2 > std::chrono::seconds( 4 ) )
            {
                motor::device::layouts::three_mouse_t tm( mouse_dev ) ;

                {
                    if( auto * comp = tm.get_component( motor::device::layouts::three_mouse_t::button::left ) ; comp != nullptr )
                    {
                        *comp = motor::device::components::button_state::pressed ;
                    }

                    if( auto * comp = tm.get_component( motor::device::layouts::three_mouse_t::button::right ) ; comp != nullptr )
                    {
                        *comp = motor::device::components::button_state::pressed ;
                    }
                }
            }

            _tp = tp ;
        }

        virtual void_t release( void_t ) noexcept 
        {
            motor::memory::release_ptr( motor::move( mouse_dev ) ) ;
            motor::memory::release_ptr( motor::move( ascii_dev ) ) ;
        }
    };
}

int main( int argc, char ** argv )
{
    {
        motor::device::system_t sys ;
        motor::device::three_device_mtr_t mouse_dev = nullptr ;
        motor::device::ascii_device_mtr_t ascii_dev = nullptr ;

        sys.add_module( motor::memory::create_ptr( this_file::dummy_module(), "my device module" ) ) ;
    }

    #if 0
    {
        // looking for device. It is managed, so pointer must be copied.
        motor::device::global_t::system()->search( [&] ( motor::device::idevice_mtr_t dev_in )
        {
            if( auto * ptr1 = dynamic_cast<motor::device::three_device_mtr_t>(dev_in); ptr1 != nullptr )
            {
                mouse_dev = ptr1 ;
            }
            else if( auto * ptr2 = dynamic_cast<motor::device::ascii_device_mtr_t>(dev_in); ptr2 != nullptr )
            {
                ascii_dev = ptr2 ;
            }
        } ) ;

        if( mouse_dev == nullptr )
        {
            motor::log::global_t::status( "no three mouse found" ) ;
        }

        if( ascii_dev == nullptr )
        {
            motor::log::global_t::status( "no ascii keyboard found" ) ;
        }
    }

    // mouse
    {
        bool_t exit = false ;

        // test buttons
        if( mouse_dev != nullptr )
        {
            motor::device::layouts::three_mouse_t mouse( mouse_dev ) ;

            auto button_funk = [&] ( motor::device::layouts::three_mouse_t::button const button )
            {
                if( mouse.is_pressed( button ) )
                {
                    motor::log::global_t::status( "button pressed: " + motor::device::layouts::three_mouse_t::to_string( button ) ) ;
                    return true ;
                }
                else if( mouse.is_pressing( button ) )
                {
                    motor::log::global_t::status( "button pressing: " + motor::device::layouts::three_mouse_t::to_string( button ) ) ;
                    return true ;
                }
                else if( mouse.is_released( button ) )
                {
                    motor::log::global_t::status( "button released: " + motor::device::layouts::three_mouse_t::to_string( button ) ) ;
                }
                return false ;
            } ;

            while( !exit )
            {
                motor::device::global_t::system()->update() ;

                auto const l = button_funk( motor::device::layouts::three_mouse_t::button::left ) ;
                auto const r = button_funk( motor::device::layouts::three_mouse_t::button::right ) ;
                button_funk( motor::device::layouts::three_mouse_t::button::middle ) ;

                // exit if left AND right are pressed
                exit = l && r ;

                // test coords
                {
                    static bool_t show_coords = false ;
                    if( mouse.is_released( motor::device::layouts::three_mouse_t::button::right ) )
                    {
                        show_coords = !show_coords ;
                    }
                    if( show_coords )
                    {
                        auto const locals = mouse.get_local() ;
                        auto const globals = mouse.get_global() ;

                        motor::log::global_t::status(
                            "local : [" + motor::from_std( std::to_string( locals.x() ) )+ ", " +
                            motor::from_std( std::to_string( locals.y() ) ) + "]"
                        ) ;

                        motor::log::global_t::status(
                            "global : [" + motor::from_std( std::to_string( globals.x() ) ) + ", " +
                            motor::from_std( std::to_string( globals.y() ) ) + "]"
                        ) ;
                    }
                }

                // test scroll
                {
                    float_t s ;
                    if( mouse.get_scroll( s ) )
                    {
                        motor::log::global_t::status( "scroll : " + motor::from_std( std::to_string( s ) ) ) ;
                    }
                }
            }
        }

        
    }

    // keyboard
    {
        if( ascii_dev != nullptr )
        {
            motor::device::layouts::ascii_keyboard_t keyboard( ascii_dev ) ;
            
            using layout_t = motor::device::layouts::ascii_keyboard_t ;
            using key_t = layout_t::ascii_key ;
            
            for( size_t i=0; i<size_t(key_t::num_keys); ++i )
            {
                auto const ks = keyboard.get_state( key_t( i ) ) ;
                if( ks != motor::device::components::key_state::none ) 
                {
                    motor::log::global_t::status( layout_t::to_string( key_t(i) ) + " : " + 
                        motor::device::components::to_string(ks) ) ;
                }
            }
        }        
    }

    #endif
    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;
}
