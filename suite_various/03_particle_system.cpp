

#include <motor/platform/global.h>
#include <motor/application/app.h>
#include <motor/tool/imgui/custom_widgets.h>

#include <motor/format/global.h>
#include <motor/format/future_items.hpp>
#include <motor/gfx/sprite/sprite_render_2d.h>
#include <motor/gfx/primitive/primitive_render_2d.h>
#include <motor/gfx/camera/generic_camera.h>

#include <motor/math/utility/fn.hpp>
#include <motor/math/utility/angle.hpp>

#include <motor/physics/particle_system.h>
#include <motor/physics/force_fields.hpp>

#include <random>
#include <thread>

namespace this_file
{
    using namespace motor::core::types ;

    struct test_particle_effect
    {
        motor_this_typedefs( test_particle_effect ) ;

        motor::physics::particle_system_t flakes ;
        motor::physics::emitter_mtr_t current_emitter ;
        motor::physics::radial_emitter_mtr_t emitter ;
        motor::physics::line_emitter_mtr_t lemitter ;
        motor::physics::acceleration_field_mtr_t g ;
        motor::physics::friction_force_field_mtr_t friction ;
        motor::physics::viscosity_force_field_mtr_t viscosity ;
        motor::physics::sin_velocity_field_mtr_t wind ;

        test_particle_effect( void_t ) noexcept
        {
            g = motor::shared( motor::physics::acceleration_field_t( motor::math::vec2f_t( 0.0f, -9.81f ) ) ) ;
            friction = motor::shared( motor::physics::friction_force_field_t() ) ;
            viscosity = motor::shared( motor::physics::viscosity_force_field_t( 0.01f ) ) ;
            wind = motor::shared( motor::physics::sin_velocity_field_t( 1.1f, 10.1f, 0.0f ) ) ;

            //flakes.attach_force_field( g ) ;
            //flakes.attach_force_field( viscosity ) ;
            flakes.attach_force_field( motor::share( wind ) ) ;


            {
                emitter = motor::shared( motor::physics::radial_emitter_t() ) ;
                emitter->set_mass( 1.0049f )  ;
                emitter->set_age( 2.5f ) ;
                emitter->set_amount( 10 ) ;
                emitter->set_rate( 2.0f ) ;
                emitter->set_angle( 0.3f ) ;
                emitter->set_velocity( 200.0f ) ;
                emitter->set_direction( motor::math::vec2f_t( 0.0f, 1.0f ) ) ;
            }

            {
                lemitter = motor::shared( motor::physics::line_emitter_t() ) ;
                lemitter->set_mass( 1.0049f ) ;
                lemitter->set_age( 2.0f ) ;
                lemitter->set_amount( 10 ) ;
                lemitter->set_rate( 2.0f ) ;
                lemitter->set_velocity( 200.0f ) ;
                lemitter->set_direction( motor::math::vec2f_t( 0.0f, 1.0f ) ) ;
            }

            current_emitter = motor::share( emitter ) ;
            flakes.attach_emitter( motor::share( current_emitter ) ) ;
        }

        test_particle_effect( this_cref_t ) = delete ;
        test_particle_effect( this_rref_t rhv ) noexcept
        {
            flakes = std::move( rhv.flakes ) ;
            emitter = motor::move( rhv.emitter ) ;
            lemitter = motor::move( rhv.lemitter ) ;
            wind = motor::move( rhv.wind ) ;
            g = motor::move( rhv.g ) ;
            current_emitter = motor::move( rhv.current_emitter ) ;
            friction = motor::move( rhv.friction ) ;
            viscosity = motor::move( rhv.viscosity ) ;
        }

        this_ref_t operator = ( this_rref_t rhv ) noexcept
        {
            flakes = std::move( rhv.flakes ) ;
            emitter = motor::move( rhv.emitter ) ;
            lemitter = motor::move( rhv.lemitter ) ;
            wind = motor::move( rhv.wind ) ;
            g = motor::move( rhv.g ) ;
            current_emitter = motor::move( rhv.current_emitter ) ;
            friction = motor::move( rhv.friction ) ;
            viscosity = motor::move( rhv.viscosity ) ;
            return *this ;
        }

        void_t update( float_t const dt ) noexcept
        {
            flakes.update( dt ) ;
        }

        void_t render( motor::gfx::primitive_render_2d_mtr_t pr )
        {
            flakes.on_particles( [&] ( motor::vector< motor::physics::particle_t > const & particles )
            {
                size_t i = 0 ;

                motor::concurrent::parallel_for<size_t>( motor::concurrent::range_1d<size_t>( 0, particles.size() ),
                    [&] ( motor::concurrent::range_1d<size_t> const & r )
                {
                    for ( size_t i = r.begin(); i < r.end(); ++i )
                    {
                        auto & p = particles[ i ] ;

                        #if 0
                        pr->draw_circle( 4, 10, p.pos, p.mass,
                            motor::math::vec4f_t( 1.0f ),
                            motor::math::vec4f_t( 1.0f, 0.0f, 0.0f, 1.0f ) ) ;
                        #elif 1
                        motor::math::vec2f_t const pos = p.pos ;
                        float_t const s = p.mass * 1.0f;

                        motor::math::vec2f_t points[] =
                        {
                            pos + motor::math::vec2f_t( -s, -s ),
                            pos + motor::math::vec2f_t( -s, +s ),
                            pos + motor::math::vec2f_t( +s, +s ),
                            pos + motor::math::vec2f_t( +s, -s )
                        } ;
                        float_t const life = p.age / emitter->get_age() ;
                        float_t const alpha = motor::math::fn<float_t>::smooth_pulse( life, 0.1f, 0.7f ) ;
                        pr->draw_rect( i % 50,
                            points[ 0 ], points[ 1 ], points[ 2 ], points[ 3 ],
                            motor::math::vec4f_t( 0.0f, 0.0f, 0.5f, alpha ),
                            motor::math::vec4f_t( 1.0f, 0.0f, 0.0f, alpha ) ) ;
                        #endif
                    }
                } ) ;


            } ) ;
        }
    } ;
    motor_typedef( test_particle_effect ) ;

    using namespace motor::core::types ;

    class my_app : public motor::application::app
    {
        motor_this_typedefs( my_app ) ;

        motor::gfx::primitive_render_2d_t pr ;
        this_file::test_particle_effect_t _spe ;

        motor::gfx::generic_camera_t camera ;

        virtual void_t on_init( void_t ) noexcept
        {
            {
                motor::application::window_info_t wi ;
                wi.x = 100 ;
                wi.y = 100 ;
                wi.w = 800 ;
                wi.h = 600 ;
                wi.gen = motor::application::graphics_generation::gen4_auto ;

                this_t::send_window_message( this_t::create_window( wi ), [&] ( motor::application::app::window_view & wnd )
                {
                    wnd.send_message( motor::application::show_message( { true } ) ) ;
                    wnd.send_message( motor::application::cursor_message_t( { true } ) ) ;
                    wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                } ) ;
            }

            #if 0
            {
                motor::application::window_info_t wi ;
                wi.x = 400 ;
                wi.y = 100 ;
                wi.w = 800 ;
                wi.h = 600 ;
                wi.gen = motor::application::graphics_generation::gen4_gl4 ;

                this_t::send_window_message( this_t::create_window( wi ), [&] ( motor::application::app::window_view & wnd )
                {
                    wnd.send_message( motor::application::show_message( { true } ) ) ;
                    wnd.send_message( motor::application::cursor_message_t( { false } ) ) ;
                    wnd.send_message( motor::application::vsync_message_t( { true } ) ) ;
                } ) ;
            }
            #endif

            pr.init( "my_prim_render" ) ;

            {
                camera.set_dims( 1.0f, 1.0f, 1.0f, 3000.0f ) ;
                camera.perspective_fov( motor::math::angle<float_t>::degree_to_radian( 45.0f ) ) ;
                camera.look_at( motor::math::vec3f_t( 0.0f, 100.0f, -1000.0f ),
                    motor::math::vec3f_t( 0.0f, 1.0f, 0.0f ), motor::math::vec3f_t( 0.0f, 0.0f, 0.0f ) ) ;
            }
        }

        virtual void_t on_event( window_id_t const wid,
            motor::application::window_message_listener::state_vector_cref_t sv ) noexcept
        {
            if ( sv.create_changed )
            {
                motor::log::global_t::status( "[my_app] : window created" ) ;
            }
            if ( sv.close_changed )
            {
                motor::log::global_t::status( "[my_app] : window closed" ) ;
                this->close() ;
            }
            if ( sv.resize_changed )
            {
                float_t const w = float_t( sv.resize_msg.w ) ;
                float_t const h = float_t( sv.resize_msg.h ) ;
                camera.set_sensor_dims( w, h ) ;
                camera.perspective_fov() ;
            }
        }

        virtual void_t on_physics( physics_data_in_t pd ) noexcept 
        {
            
        }

        virtual void_t on_graphics( motor::application::app::graphics_data_in_t gd ) noexcept
        {
            // do update all particle effects
            {
                _spe.update( gd.sec_dt ) ;
            }

            // draw particle effects
            {
                _spe.render( &pr ) ;

                #if 0
                pr.draw_circle( 0, 20, _spe.emitter->get_position(), _spe.emitter->get_radius(),
                    motor::math::vec4f_t( 0.0f, 0.5f, 0.0f, 0.1f ), motor::math::vec4f_t( 0.0f, 0.5f, 0.0f, 0.1f ) ) ;

                {
                    auto const points = _spe.flakes.get_extend_rect() ;
                    pr.draw_rect( 0, points[ 0 ], points[ 1 ], points[ 2 ], points[ 3 ],
                        motor::math::vec4f_t( 0.0f ),
                        motor::math::vec4f_t( 0.5f ) ) ;
                }
                #endif

                
            }
            pr.set_view_proj( camera.mat_view(), camera.mat_proj() ) ;
            pr.prepare_for_rendering() ;
        }

        virtual void_t on_render( this_t::window_id_t const wid, motor::graphics::gen4::frontend_ptr_t fe,
            motor::application::app::render_data_in_t rd ) noexcept
        {
            if ( rd.first_frame )
            {
                pr.configure( fe ) ;
            }

            // render text layer 0 to screen
            {
                pr.prepare_for_rendering( fe ) ;
                for( size_t i=0; i<100; ++i )
                {
                    pr.render( fe, 0 ) ;
                    pr.render( fe, 1 ) ;
                }
            }
        }


        virtual bool_t on_tool( this_t::window_id_t const, motor::application::app::tool_data_ref_t ) noexcept
        {
            ImGui::Begin( "Control Particle System" ) ;

            static int item_current = 0 ;
            bool_t item_changed = false ;
            {
                const char * items[] = { "Radial", "Line" };
                if ( item_changed = ImGui::Combo( "Emitter", &item_current, items, IM_ARRAYSIZE( items ) ) )
                {
                    _spe.flakes.detach_emitter( motor::share( _spe.current_emitter ) ) ;
                }
            }

            if ( item_current == 0 )
            {
                if ( item_changed )
                {
                    _spe.current_emitter = _spe.emitter ;
                    _spe.flakes.attach_emitter( motor::share( _spe.current_emitter ) ) ;
                }

                {
                    float_t v = _spe.emitter->get_radius() ;
                    if ( ImGui::SliderFloat( "Radius", &v, 0.0f, 100.0f ) )
                    {
                        _spe.emitter->set_radius( v ) ;
                    }
                }
                {
                    float_t v = _spe.emitter->get_angle() ;
                    if ( ImGui::SliderFloat( "Angle", &v, 0.0f, 3.14f ) )
                    {
                        _spe.emitter->set_angle( v ) ;
                    }
                }

                {
                    const char * items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.emitter->get_radius_variation_type() == motor::physics::variation_type::fixed ? 0 : 1 ;
                    if ( item_changed = ImGui::Combo( "Radius Variation", &rand_item, items, IM_ARRAYSIZE( items ) ) )
                    {
                        if ( rand_item == 0 ) _spe.emitter->set_radius_variation_type( motor::physics::variation_type::fixed ) ;
                        else if ( rand_item == 1 ) _spe.emitter->set_radius_variation_type( motor::physics::variation_type::random ) ;
                    }
                }
                {
                    const char * items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.emitter->get_angle_variation_type() == motor::physics::variation_type::fixed ? 0 : 1 ;
                    if ( item_changed = ImGui::Combo( "Angle Variation", &rand_item, items, IM_ARRAYSIZE( items ) ) )
                    {
                        if ( rand_item == 0 ) _spe.emitter->set_angle_variation_type( motor::physics::variation_type::fixed ) ;
                        else if ( rand_item == 1 ) _spe.emitter->set_angle_variation_type( motor::physics::variation_type::random ) ;
                    }
                }
            }
            else if ( item_current == 1 )
            {
                if ( item_changed )
                {
                    _spe.current_emitter = _spe.lemitter ;
                    _spe.flakes.attach_emitter( motor::share( _spe.current_emitter ) ) ;
                }

                {
                    float_t v = _spe.lemitter->get_ortho_distance() ;
                    if ( ImGui::SliderFloat( "Ortho Dist", &v, -400.0f, 400.0f ) )
                    {
                        _spe.lemitter->set_ortho_distance( v ) ;
                    }
                }
                {
                    float_t v = _spe.lemitter->get_parallel_distance() ;
                    if ( ImGui::SliderFloat( "Parallel Dist", &v, 1.0f, 400.0f ) )
                    {
                        _spe.lemitter->set_parallel_distance( v ) ;
                    }
                }
                {
                    const char * items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.lemitter->get_ortho_variation_type() == motor::physics::variation_type::fixed ? 0 : 1 ;
                    if ( item_changed = ImGui::Combo( "Ortho Variation", &rand_item, items, IM_ARRAYSIZE( items ) ) )
                    {
                        if ( rand_item == 0 ) _spe.lemitter->set_ortho_variation_type( motor::physics::variation_type::fixed ) ;
                        else if ( rand_item == 1 ) _spe.lemitter->set_ortho_variation_type( motor::physics::variation_type::random ) ;
                    }
                }
                {
                    const char * items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.lemitter->get_parallel_variation_type() == motor::physics::variation_type::fixed ? 0 : 1 ;
                    if ( item_changed = ImGui::Combo( "Parallel Variation", &rand_item, items, IM_ARRAYSIZE( items ) ) )
                    {
                        if ( rand_item == 0 ) _spe.lemitter->set_parallel_variation_type( motor::physics::variation_type::fixed ) ;
                        else if ( rand_item == 1 ) _spe.lemitter->set_parallel_variation_type( motor::physics::variation_type::random ) ;
                    }
                }
            }

            {
                {
                    auto dir = _spe.current_emitter->get_direction() ;
                    if ( motor::tool::custom_imgui_widgets::direction( "dir", dir ) )
                    {
                        _spe.current_emitter->set_direction( dir ) ;
                    }
                }
                ImGui::SameLine() ;
                {
                    float_t v = _spe.current_emitter->get_rate() ;
                    if ( ImGui::VSliderFloat( "Rate", ImVec2( 50, 100 ), &v, 1.0f, 100.0f ) )
                    {
                        _spe.current_emitter->set_rate( v ) ;
                        _spe.flakes.clear() ;
                    }
                }
                ImGui::SameLine() ;
                {
                    int_t v = int_t( _spe.current_emitter->get_amount() ) ;
                    if ( ImGui::VSliderInt( "Amount", ImVec2( 50, 100 ), &v, 1, 100 ) )
                    {
                        _spe.current_emitter->set_amount( v ) ;
                        _spe.flakes.clear() ;
                    }
                }
                ImGui::SameLine() ;
                {
                    float_t v = _spe.current_emitter->get_age() ;
                    if ( ImGui::VSliderFloat( "Age", ImVec2( 50, 100 ), &v, 1.0f, 10.0f ) )
                    {
                        _spe.current_emitter->set_age( v ) ;
                    }
                }
                ImGui::SameLine() ;


                ImGui::SameLine() ;
                {
                    float_t v = _spe.current_emitter->get_velocity() ;
                    if ( ImGui::VSliderFloat( "Velocity", ImVec2( 50, 100 ), &v, 0.0f, 1000.0f ) )
                    {
                        _spe.current_emitter->set_velocity( v ) ;
                    }
                }

                {
                    const char * items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.current_emitter->get_mass_variation_type() == motor::physics::variation_type::fixed ? 0 : 1 ;
                    if ( item_changed = ImGui::Combo( "Mass Variation", &rand_item, items, IM_ARRAYSIZE( items ) ) )
                    {
                        if ( rand_item == 0 ) _spe.current_emitter->set_mass_variation_type( motor::physics::variation_type::fixed ) ;
                        else if ( rand_item == 1 ) _spe.current_emitter->set_mass_variation_type( motor::physics::variation_type::random ) ;
                    }
                }
                {
                    const char * items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.current_emitter->get_age_variation_type() == motor::physics::variation_type::fixed ? 0 : 1 ;
                    if ( item_changed = ImGui::Combo( "Age Variation", &rand_item, items, IM_ARRAYSIZE( items ) ) )
                    {
                        if ( rand_item == 0 ) _spe.current_emitter->set_age_variation_type( motor::physics::variation_type::fixed ) ;
                        else if ( rand_item == 1 ) _spe.current_emitter->set_age_variation_type( motor::physics::variation_type::random ) ;
                    }
                }
                {
                    const char * items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.current_emitter->get_acceleration_variation_type() == motor::physics::variation_type::fixed ? 0 : 1 ;
                    if ( item_changed = ImGui::Combo( "Acceleration Variation", &rand_item, items, IM_ARRAYSIZE( items ) ) )
                    {
                        if ( rand_item == 0 ) _spe.current_emitter->set_acceleration_variation_type( motor::physics::variation_type::fixed ) ;
                        else if ( rand_item == 1 ) _spe.current_emitter->set_acceleration_variation_type( motor::physics::variation_type::random ) ;
                    }
                }
                {
                    const char * items[] = { "Fixed", "Random" } ;
                    static int rand_item = _spe.current_emitter->get_velocity_variation_type() == motor::physics::variation_type::fixed ? 0 : 1 ;
                    if ( item_changed = ImGui::Combo( "Velocity Variation", &rand_item, items, IM_ARRAYSIZE( items ) ) )
                    {
                        if ( rand_item == 0 ) _spe.current_emitter->set_velocity_variation_type( motor::physics::variation_type::fixed ) ;
                        else if ( rand_item == 1 ) _spe.current_emitter->set_velocity_variation_type( motor::physics::variation_type::random ) ;
                    }
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

    motor::io::global::deinit() ;
    motor::concurrent::global::deinit() ;
    motor::log::global::deinit() ;
    motor::memory::global::dump_to_std() ;


    return ret ;
}
