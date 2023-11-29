
#include <motor/graphics/shader/msl_bridge.hpp>

#include <motor/msl/parser.h>
#include <motor/msl/database.hpp>
#include <motor/msl/dependency_resolver.hpp>
#include <motor/msl/api/glsl/generator.h>

#include <motor/format/global.h>
#include <motor/io/database.h>
#include <motor/log/global.h>

using namespace motor::core::types ;

//
// This project tests plain 
// - loading of msl files (io)
// - parsing and generating code (msl)
//
int main( int argc, char ** argv )
{
    // create a database for the project
    motor::io::database_mtr_t db = motor::memory::create_ptr( motor::io::database_t( motor::io::path_t( DATAPATH ), "./working", "data" ) ) ;
    motor::msl::database_mtr_t msl_db = motor::memory::create_ptr(  motor::msl::database_t() ) ;

    // test lib
    {
        db->load( motor::io::location_t( "shader.mylib.msl" ) ).wait_for_operation( [&] ( char_cptr_t d, size_t const sib, motor::io::result const )
        {
            motor::string_t file = motor::string_t( d, sib ) ;
            motor::msl::post_parse::document_t doc = motor::msl::parser_t("mylib.msl").process( std::move( file ) ) ;

            msl_db->insert( std::move( doc ) ) ;
        } ) ;
    }

    // test myshader
    {
        db->load( motor::io::location_t( "shader.myshader.msl" ) ).wait_for_operation( [&] ( char_cptr_t d, size_t const sib, motor::io::result const )
        {
            motor::string_t file = motor::string_t( d, sib ) ;
            motor::msl::post_parse::document_t doc = motor::msl::parser_t( "myshader.msl" ).process( std::move( file ) ) ;

            msl_db->insert( std::move( doc ) ) ;
        } ) ;
    }

    motor::msl::generatable_t res ;

    // resolve all dependencies
    {
        res = motor::msl::dependency_resolver_t().resolve( motor::share( msl_db ), motor::msl::symbol("myconfig") ) ;
        if( res.missing.size() != 0 )
        {
            motor::log::global_t::warning("We have missing symbols.") ;
            for( auto const & s : res.missing )
            {
                motor::log::global_t::status( s.expand() ) ;
            }
        }
    }

    // generate code
    {
        motor::msl::generator_t gen( std::move( res ) ) ;

        auto const gen_code = gen.generate() ;

        auto const sc = motor::graphics::msl_bridge_t().create( gen_code ) ;
    }

    return 0 ;
}
