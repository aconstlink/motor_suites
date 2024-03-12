

#include <motor/msl/symbol.hpp>
#include <motor/msl/database.hpp>
#include <motor/msl/dependency_resolver.hpp>
#include <motor/msl/generators/generator.h>
#include <motor/msl/generators/hlsl5_generator.h>
#include <motor/msl/generators/glsl4_generator.h>
#include <motor/msl/generators/essl3_generator.h>

#include <motor/msl/parser.h>

#include <motor/concurrent/global.h>
#include <motor/io/database.h>
#include <motor/log/global.h>

using namespace motor::core::types ;

//*******************************************************************************
void_t test_1( motor::io::database_mtr_t db ) noexcept
{
    motor::msl::database_t ndb = motor::msl::database_t() ;

    motor::string_t code = R"(
    config some_config
    {
        vertex_shader
        {
            float_t some_other_function( float_t y )
            {
                return noise_1d( y ) ;
            }

            void main()
            {
                float_t a = fract( 1.9 ) ;
                float_t r2 = noise_1d( 1.0 ) * some_other_function( a ) ;
            }
        }
    
        pixel_shader
        {
            float_t some_other_function( float_t y )
            {
                return noise_1d( y ) ;
            }

            void main()
            {
                float_t a = fract( 1.9 ) ;
                float_t some_noise = noise_1d( vec3_t(1.0, 2.0, 4.0) ) * some_other_function( a ) ;
            }
        }

    })" ;

    motor::msl::post_parse::document_t doc = 
        motor::msl::parser_t( "my_parser" ).process( std::move( code ) ) ;

    motor::vector< motor::msl::symbol_t > config_symbols ;

    ndb.insert( std::move( doc ), config_symbols ) ;    

    for( auto const & c : config_symbols )
    {
        motor::msl::generatable_t res = motor::msl::dependency_resolver_t().resolve( &ndb, c ) ;
        if( res.missing.size() != 0 )
        {
            motor::log::global_t::warning( "We have missing symbols." ) ;
            for( auto const& s : res.missing )
            {
                motor::log::global_t::status( s.expand() ) ;
            }
        }

        motor::msl::generator_t gen( std::move( res ) ) ;

        {
            auto gcode = gen.generate<motor::msl::glsl::glsl4_generator_t>() ;
            int const bp = 0 ;
        }

        {
            auto gcode = gen.generate<motor::msl::hlsl::hlsl5_generator_t>() ;
            int const bp = 0 ;
        }

        {
            auto gcode = gen.generate<motor::msl::essl::essl3_generator_t>() ;
            int const bp = 0 ;
        }
    }
}

//*******************************************************************************
void_t test_2( motor::io::database_mtr_t db ) noexcept
{
    motor::msl::database_t ndb = motor::msl::database_t() ;

    motor::string_t code = R"(
    config some_config
    {
        vertex_shader
        {
            float_t some_other_function( float_t y )
            {
                return noise_1d( y ) ;
            }

            void main()
            {
                float_t a = fract( 1.9 ) ;
                float_t r2 = noise_1d( 1.0 ) * some_other_function( a ) ;
            }
        }
    
        pixel_shader
        {
            float_t some_other_function( float_t y )
            {
                return noise_1d( y ) ;
            }

            void main()
            {
                float_t a = fract( 1.9 ) ;
                float_t some_noise = perlin_1d( vec3_t(1.0, 2.0, 4.0) ) * some_other_function( a ) ;
            }
        }

    })" ;

    motor::msl::post_parse::document_t doc = 
        motor::msl::parser_t( "my_parser" ).process( std::move( code ) ) ;

    motor::vector< motor::msl::symbol_t > config_symbols ;

    ndb.insert( std::move( doc ), config_symbols ) ;    

    for( auto const & c : config_symbols )
    {
        motor::msl::generatable_t res = motor::msl::dependency_resolver_t().resolve( &ndb, c ) ;
        if( res.missing.size() != 0 )
        {
            motor::log::global_t::warning( "We have missing symbols." ) ;
            for( auto const& s : res.missing )
            {
                motor::log::global_t::status( s.expand() ) ;
            }
        }

        motor::msl::generator_t gen( std::move( res ) ) ;

        {
            auto gcode = gen.generate<motor::msl::glsl::glsl4_generator_t>() ;
            int const bp = 0 ;
        }

        {
            auto gcode = gen.generate<motor::msl::hlsl::hlsl5_generator_t>() ;
            int const bp = 0 ;
        }

        {
            auto gcode = gen.generate<motor::msl::essl::essl3_generator_t>() ;
            int const bp = 0 ;
        }
    }
}

#if 0
//*******************************************************************************
void_t test_1( motor::io::database_mtr_t db ) noexcept
{
    motor::msl::database_t ndb = motor::msl::database_t() ;

    motor::string_t code = R"(
    config some_config
    {
        vertex_shader
        {
            in vec4_t pos : position ;
            in vec4_t color : color ;

            out vec4_t pos : position ;
            out vec4_t color : color ;

            uint_t vid : vertex_id ;

            // will contain the transform feedback data
            data_buffer_t u_data ;

            void main()
            {
                int_t idx = vid / 4 ;
                vec4_t pos = fetch_data( u_data, (idx << 2) + 0 ) ; 
                //vec4_t vel = fetch_data( u_data, (idx << 2) + 1 ) ;

                pos = vec4_t( pos.xyz ' vec3_t(0.001, 0.001, 0.001), 1.0 ) ;
                out.color = in.color ;
                out.pos = ((in.pos'vec4_t(0.01,0.01,0.01,1.0))+ vec4_t(pos.xyz,0.0)) ;
            }
        }

        pixel_shader
        {
            in vec4_t color : color ;
            out vec4_t color : color ;

            void main()
            {
                out.color = in.color ;
            }
        }
    })" ;

    motor::msl::post_parse::document_t doc = 
        motor::msl::parser_t( code ).process( std::move( file ) ) ;

    motor::vector< motor::msl::symbol_t > config_symbols ;

    ndb.insert( std::move( doc ), config_symbols ) ;    

    for( auto const & c : config_symbols )
    {
        motor::msl::generatable_t res = motor::msl::dependency_resolver_t().resolve( ndb, c ) ;
        if( res.missing.size() != 0 )
        {
            motor::log::global_t::warning( "We have missing symbols." ) ;
            for( auto const& s : res.missing )
            {
                motor::log::global_t::status( s.expand() ) ;
            }
        }

        motor::msl::generator_t gen( std::move( res ) ) ;

        {
            auto code = gen.generate<motor::msl::glsl::generator_t>() ;
            int const bp = 0 ;
        }

        {
            auto code = gen.generate<motor::msl::hlsl::generator_t>() ;
            int const bp = 0 ;
        }
    }
}


//*******************************************************************************
void_t test_2( motor::io::database_mtr_t db )
{
    motor::msl::database_mtr_t ndb = motor::memory::create_ptr( motor::msl::database_t() ) ;
    motor::vector< motor::io::location_t > shader_locations = {
        motor::io::location_t( "shaders.test_if.msl" )
    };

    motor::vector< motor::msl::symbol_t > config_symbols ;

    #if 0
    for( auto const& l : shader_locations )
    {
        motor::format::module_registry_res_t mod_reg = motor::format::global_t::registry() ;
        auto fitem2 = mod_reg->import_from( l, db ) ;

        motor::format::nsl_item_res_t ii = fitem2.get() ;
        if( ii.is_valid() ) ndb->insert( std::move( std::move( ii->doc ) ), config_symbols ) ;
    }
    #endif
    for( auto const & c : config_symbols )
    {
        motor::msl::generatable_t res = motor::msl::dependency_resolver_t().resolve( motor::share( ndb ), c ) ;
        if( res.missing.size() != 0 )
        {
            motor::log::global_t::warning( "We have missing symbols." ) ;
            for( auto const& s : res.missing )
            {
                motor::log::global_t::status( s.expand() ) ;
            }
        }

        {
            auto code = motor::msl::generator_t( std::move( res ) ).generate<motor::msl::glsl::generator_t>() ;
            int const bp = 0 ;
        }

        {
            auto code = motor::msl::generator_t( std::move( res ) ).generate<motor::msl::hlsl::generator_t>() ;
            int const bp = 0 ;
        }
    }

    motor::memory::release_ptr( ndb ) ;
}

void_t test_3( motor::io::database_mtr_t db )
{
    motor::msl::database_mtr_t ndb = motor::memory::create_ptr( motor::msl::database_t() ) ;
    motor::vector< motor::io::location_t > shader_locations = {
        motor::io::location_t( "shaders.build_ins.msl" )
    };

    motor::vector< motor::msl::symbol_t > config_symbols ;

    #if 0
    for( auto const& l : shader_locations )
    {
        motor::format::module_registry_res_t mod_reg = motor::format::global_t::registry() ;
        auto fitem2 = mod_reg->import_from( l, db ) ;

        motor::format::nsl_item_res_t ii = fitem2.get() ;
        if( ii.is_valid() ) ndb->insert( std::move( std::move( ii->doc ) ), config_symbols ) ;
    }
    #endif
    for( auto const & c : config_symbols )
    {
        motor::msl::generatable_t res = motor::msl::dependency_resolver_t().resolve( motor::share( ndb ), c ) ;
        if( res.missing.size() != 0 )
        {
            motor::log::global_t::warning( "We have missing symbols." ) ;
            for( auto const& s : res.missing )
            {
                motor::log::global_t::status( s.expand() ) ;
            }
        }

        {
            auto code = motor::msl::generator_t( std::move( res ) ).generate<motor::msl::glsl::generator_t>() ;
            int const bp = 0 ;
        }

        {
            auto code = motor::msl::generator_t( std::move( res ) ).generate<motor::msl::hlsl::generator_t>() ;
            int const bp = 0 ;
        }
    }
}

void_t test_4( motor::io::database_mtr_t db )
{
    motor::msl::database_mtr_t ndb = motor::memory::create_ptr( motor::msl::database_t() ) ;
    motor::vector< motor::io::location_t > shader_locations = {
        motor::io::location_t( "shaders.main_early_return.msl" )
    };

    motor::vector< motor::msl::symbol_t > config_symbols ;

    #if 0
    for( auto const& l : shader_locations )
    {
        motor::format::module_registry_res_t mod_reg = motor::format::global_t::registry() ;
        auto fitem2 = mod_reg->import_from( l, db ) ;

        motor::format::nsl_item_res_t ii = fitem2.get() ;
        if( ii.is_valid() ) ndb->insert( std::move( std::move( ii->doc ) ), config_symbols ) ;
    }
    #endif
    for( auto const & c : config_symbols )
    {
        motor::msl::generatable_t res = motor::msl::dependency_resolver_t().resolve( motor::share( ndb ), c ) ;
        if( res.missing.size() != 0 )
        {
            motor::log::global_t::warning( "We have missing symbols." ) ;
            for( auto const& s : res.missing )
            {
                motor::log::global_t::status( s.expand() ) ;
            }
        }

        {
            auto code = motor::msl::generator_t( std::move( res ) ).generate<motor::msl::glsl::generator_t>() ;
            int const bp = 0 ;
        }

        {
            auto code = motor::msl::generator_t( std::move( res ) ).generate<motor::msl::hlsl::generator_t>() ;
            int const bp = 0 ;
        }
    }
}

void_t test_5( motor::io::database_mtr_t db )
{
    motor::msl::database_mtr_t ndb = motor::memory::create_ptr( motor::msl::database_t() ) ;
    motor::vector< motor::io::location_t > shader_locations = {
        motor::io::location_t( "shaders.inout.msl" )
    };

    motor::vector< motor::msl::symbol_t > config_symbols ;

    #if 0
    for( auto const& l : shader_locations )
    {
        motor::format::module_registry_res_t mod_reg = motor::format::global_t::registry() ;
        auto fitem2 = mod_reg->import_from( l, db ) ;

        motor::format::nsl_item_res_t ii = fitem2.get() ;
        if( ii.is_valid() ) ndb->insert( std::move( std::move( ii->doc ) ), config_symbols ) ;
    }
    #endif

    for( auto const & c : config_symbols )
    {
        motor::msl::generatable_t res = motor::msl::dependency_resolver_t().resolve( motor::share( ndb ), c ) ;
        if( res.missing.size() != 0 )
        {
            motor::log::global_t::warning( "We have missing symbols." ) ;
            for( auto const& s : res.missing )
            {
                motor::log::global_t::status( s.expand() ) ;
            }
        }

        {
            auto code = motor::msl::generator_t( std::move( res ) ).generate<motor::msl::glsl::generator_t>() ;
            int const bp = 0 ;
        }

        {
            auto code = motor::msl::generator_t( std::move( res ) ).generate<motor::msl::hlsl::generator_t>() ;
            int const bp = 0 ;
        }
    }
}

#endif

int main( int argc, char ** argv )
{
    motor::io::database_mtr_t db = motor::memory::create_ptr( 
        motor::io::database_t( motor::io::path_t( DATAPATH ), "./working", "data" ) ) ;

    
    test_1( db ) ;
    test_2( db ) ;
    //test_3( db ) ;

    //test_4( db ) ;
    //test_5( db ) ;
    //test_6( db ) ;
    

    motor::memory::release_ptr( db ) ;


    motor::concurrent::global_t::deinit() ;
    motor::io::global_t::deinit() ;
    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;

    return 0 ;
}
