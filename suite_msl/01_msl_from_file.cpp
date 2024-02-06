

#include <motor/msl/symbol.hpp>
#include <motor/msl/database.hpp>
#include <motor/msl/generator.h>
#include <motor/msl/dependency_resolver.hpp>

//#include <motor/format/global.h>
//#include <motor/format/nsl/nsl_module.h>
#include <motor/msl/parser.h>

#include <motor/concurrent/global.h>
#include <motor/io/database.h>
#include <motor/log/global.h>

#include <regex>
#include <iostream>

using namespace motor::core::types ;

//*******************************************************************************
void_t test_1( motor::io::database_mtr_t db )
{
    motor::msl::database_mtr_t ndb = motor::memory::create_ptr( motor::msl::database_t() ) ;
    motor::vector< motor::io::location_t > shader_locations = {
        motor::io::location_t( "test_01.lib_a.msl" ),
        motor::io::location_t( "test_01.lib_b.msl" ),
        motor::io::location_t( "test_01.effect.msl" )
    };

    motor::vector< motor::msl::symbol_t > config_symbols ;

    for( auto const& l : shader_locations )
    {
        motor::msl::post_parse::document_t doc ;

        auto res = db->load( l ).wait_for_operation( [&]( char_cptr_t data, size_t const sib, motor::io::result const )
        {
            motor::string_t file = motor::string_t( data, sib ) ;
            doc = motor::msl::parser_t( l.as_string() ).process( std::move( file ) ) ;
        } ) ;

        if( res ) ndb->insert( std::move( doc ), config_symbols ) ;
    }

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
    motor::memory::release_ptr( ndb ) ;
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

int main( int argc, char ** argv )
{
    motor::io::database_mtr_t db = motor::memory::create_ptr( 
        motor::io::database_t( motor::io::path_t( DATAPATH ), "./working", "data" ) ) ;

    
    test_1( db ) ;
    //test_2( db ) ;
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
