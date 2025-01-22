

#include <motor/msl/symbol.hpp>
#include <motor/msl/database.hpp>
#include <motor/msl/dependency_resolver.hpp>
#include <motor/msl/generators/generator.h>
#include <motor/msl/generators/hlsl5_generator.h>
#include <motor/msl/generators/glsl4_generator.h>
#include <motor/msl/generators/essl3_generator.h>

#include <motor/msl/parser.h>

#include <motor/concurrent/global.h>
#include <motor/log/global.h>

using namespace motor::core::types ;

//*******************************************************************************
void_t test_1( void_t ) noexcept
{
    motor::msl::database_t ndb = motor::msl::database_t() ;

    #if 1
    motor::string_t code = R"(
    vec2_t offset = { vec2_t(-0.5,0.0), vec2_t(0.5,0.0) } ;
    )" ;
    #else
    motor::string_t code = R"(
    config test_texture_array
    {
        vertex_shader
        {
            void main()
            {
                vec2_t offset = { vec2_t(-0.5,0.0), vec2_t(0.5,0.0) } ;
            }
        }
        
    })" ;
    #endif

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


int main( int argc, char ** argv )
{    
    test_1() ;

    motor::concurrent::global_t::deinit() ;
    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;

    return 0 ;
}
