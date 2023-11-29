
#include "main.h"

#include <natus/graphics/shader/nsl_bridge.hpp>

#include <natus/nsl/parser.h>
#include <natus/nsl/database.hpp>
#include <natus/nsl/dependency_resolver.hpp>
#include <natus/nsl/api/glsl/generator.h>

#include <natus/format/global.h>
#include <natus/io/database.h>
#include <natus/log/global.h>

using namespace natus::core::types ;

//
// This project tests plain 
// - loading of nsl files (io)
// - parsing and generating code (nsl)
//
int main( int argc, char ** argv )
{
    // create a database for the project
    natus::io::database_res_t db = natus::io::database_t( natus::io::path_t( DATAPATH ), "./working", "data" ) ;
    natus::nsl::database_res_t nsl_db = natus::nsl::database_t() ;

    // test lib
    {
        db->load( natus::io::location_t( "shader.mylib.nsl" ) ).wait_for_operation( [&] ( char_cptr_t d, size_t const sib )
        {
            natus::ntd::string_t file = natus::ntd::string_t( d, sib ) ;
            natus::nsl::post_parse::document_t doc = natus::nsl::parser_t("mylib.nsl").process( std::move( file ) ) ;

            nsl_db->insert( std::move( doc ) ) ;
        } ) ;
    }

    // test myshader
    {
        db->load( natus::io::location_t( "shader.myshader.nsl" ) ).wait_for_operation( [&] ( char_cptr_t d, size_t const sib )
        {
            natus::ntd::string_t file = natus::ntd::string_t( d, sib ) ;
            natus::nsl::post_parse::document_t doc = natus::nsl::parser_t( "myshader.nsl" ).process( std::move( file ) ) ;

            nsl_db->insert( std::move( doc ) ) ;
        } ) ;
    }

    natus::nsl::generatable_t res ;

    // resolve all dependencies
    {
        res = natus::nsl::dependency_resolver_t().resolve( nsl_db, natus::nsl::symbol("myconfig") ) ;
        if( res.missing.size() != 0 )
        {
            natus::log::global_t::warning("We have missing symbols.") ;
            for( auto const & s : res.missing )
            {
                natus::log::global_t::status( s.expand() ) ;
            }
        }
    }

    // generate code
    {
        natus::nsl::generator_t gen( std::move( res ) ) ;

        auto const gen_code = gen.generate() ;

        auto const sc = natus::graphics::nsl_bridge_t().create( gen_code ) ;
    }

    return 0 ;
}
