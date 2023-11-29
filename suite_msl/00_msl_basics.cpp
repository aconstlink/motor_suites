

#include <motor/msl/symbol.hpp>
#include <motor/msl/database.hpp>
#include <motor/msl/generator.h>
#include <motor/msl/dependency_resolver.hpp>

//#include <motor/format/global.h>
//#include <motor/format/nsl/nsl_module.h>
#include <motor/msl/parser.h>

#include <motor/log/global.h>

#include <regex>
#include <iostream>

using namespace motor::core::types ;

namespace this_file
{
    // this function is used in the nsl parser
    motor::vector< motor::string_t > tokenize( motor::string_cref_t s ) noexcept
    {
        motor::vector< motor::string_t > tokens ;

        size_t off = 0 ;
        size_t pos = s.find_first_of( ' ' ) ;
        while( pos != std::string::npos )
        {
            tokens.emplace_back( s.substr( off, pos - off ) ) ;

            off = pos + 1 ;
            pos = s.find_first_of( ' ', off ) ;
        }
        tokens.emplace_back( s.substr( off ) ) ;

        return std::move( tokens ) ;
    }

    //*******************************************************************************
    void_t test_1( void_t )
    {
        {
            motor::string_t s = "in ." ;
            std::regex input( "in *\\." ) ;
            s = std::regex_replace( s, input, "input." ) ; 
            int pb = 0 ;
        }

        {
            motor::string_t s = "in[0]." ;
            std::regex input( "in *(\\[ *[0-9]*[a-z]* *\\])? *\\." ) ;
            s = std::regex_replace( s, input, "input$1." ) ; 
            int pb = 0 ;
        }
        int pb = 0 ;
    }

    //*******************************************************************************
    void_t test_2( void_t ) 
    {
        motor::string_t s = "a = mul ( mul ( mul ( a , b ) , bv ) , vec4_t ( in.pos , 1.0 ) )";

        motor::string_t const what( "mul" ) ;
        auto repl = [&] ( motor::vector< motor::string_t > const& args ) -> motor::string_t
        {
            if( args.size() != 2 ) return "mul ( INVALID_ARGS ) " ;

            return args[ 0 ] + " * " + args[ 1 ] ;
        } ;

        size_t p0 = s.find( what ) ;
        while( p0 != std::string::npos )
        { 
            motor::vector< motor::string_t > args ;

            size_t level = 0 ;
            size_t beg = p0 + what.size() + 3 ; 
            for( size_t i=beg; i<s.size(); ++i )
            {
                if( level == 0 && s[i] == ',' ||
                    level == 0 && s[i] == ')' )
                {
                    args.emplace_back( s.substr( beg, (i-1) - beg ) ) ;
                    beg = i + 2 ;
                }

                if( s[ i ] == '(' ) ++level ;
                else if( s[ i ] == ')' ) --level ;
                if( level == size_t( -1 ) ) break ;
            }
            p0 = s.replace( p0, (--beg) - p0, repl( args ) ).find( what ) ;
        }
        int bp = 0  ;
    }

    //*******************************************************************************
    // extract used functions with arg list in shader code
    void_t test_3_extract_by_tokenize( void_t ) 
    {
        // 1. precondition is that white spaces are inserted before function extraction.
        motor::string_t s = "a = mul ( mul ( mul ( a , b ) , bv ) , vec4_t ( in.pos , 1.0 ) )";

        auto const tokens = this_file::tokenize( s ) ;

        // 2. find the first ( so we can check if it is a grouping (..) or
        // a possible function call.
        auto iter = std::find( tokens.begin(), tokens.end(), "(" ) ;
        if( std::distance( tokens.begin(), iter ) == 0 ) ++iter ;

        struct func_sym
        {
            motor::string_t name ;
            motor::vector< motor::string_t > args ;
        };
        motor::vector< func_sym > extracted ;

        // 3. extract function names and args
        for( iter; iter != tokens.end(); ++iter )
        {
            if( *iter != "(" ) continue ;
            if( *iter == "(" && ((iter-1)->size() == 1)  ) continue ;

            motor::string_t func_name = *(iter-1) ;
            motor::vector< motor::string_t > args ;
            motor::string_t arg ;

            // extract args
            {
                size_t level = 1 ;
                auto iter2 = ++iter ;
                while( level != 0 )
                {
                    // level should reach 0 again, otherwise its an error
                    if( iter2 == tokens.end() ) break ;
                
                    if( *iter2 == "(" ) ++level ;
                    else if( *iter2 == ")" ) --level ;

                    // within first level parenthesis
                    if( *iter2 == "," && level == 1 ) 
                    {
                        args.emplace_back( arg.substr( 0, arg.size()-1 ) ) ;
                        arg = "" ;
                    }
                    // outside of parenthesis, with closing )
                    else if( *iter2 == ")" && level == 0 )
                    {
                        args.emplace_back( arg.substr( 0, arg.size()-1 ) ) ;
                        break ;
                    }
                    else arg += *iter2 + " " ;

                    ++iter2 ;
                }

                if( level != 0 ) 
                {
                    // print error.
                }
            }

            extracted.emplace_back( func_sym { func_name, args } ) ;
        }

        int bp = 0 ;
    
    }

    //*******************************************************************************
    // test operator extraction
    void_t test_4( void_t ) 
    {
        // 1. precondition is that white spaces are inserted before function extraction.
        motor::string_t s0 = "sign ( in.pos.xy ) * __float(0.5) + __float(0.5)";
        motor::string_t s1 = "vec2i_t ( __int(1) , __int(1) ) * - __int(1)";
        motor::string_t s2 = "++ i ; i ++ ; + i ; - i ";
        motor::string_t s3 = "__float(0.5) + __float(0.5)";

        struct repl
        {
            motor::string_t what ;
            motor::string_t with ;

            // a + b -> false
            // a /= b -> false
            // ++a -> true
            // ^b -> true
            bool_t single = false ;
        };

        typedef std::function< bool_t ( char const t, size_t const l ) > is_stop_t ;
        auto do_replacement = [] ( motor::string_ref_t line, repl const& r, is_stop_t is_stop )
        {
            size_t off = 0 ;
            while( true )
            {
                size_t const p0 = line.find( r.what, off ) ;
                if( p0 == std::string::npos ) break ;
            
                motor::string_t arg0, arg1 ;

                size_t beg = 0 ;
                size_t end = 0 ;

                // arg 0, left of what
                if( p0 > 0 )
                {
                    size_t level = 0 ;
                    size_t const cut = p0 - 1 ;
                    size_t p1 = p0 ;
                    while( --p1 != size_t( -1 ) )
                    {
                        if( line[ p1 ] == ')' ) ++level ;
                        else if( line[ p1 ] == '(' ) --level ;

                        if( is_stop( line[ p1 ], level ) ) break ;
                    }
                    // if the beginning is hit, there is one position missing.
                    if( p1 == size_t( -1 ) ) --p1 ;

                    // p3 is the (at stop char) + the char + a white space
                    size_t const p3 = p1 + 2 ;

                    // the condition may happen if the algo above has no arg0 for operator found
                    if( p3 > cut ) arg0 = "" ;
                    else arg0 = line.substr( p1 + 2, cut - ( p1 + 2 ) ) ;

                    beg = p3 ;
                }

                // arg1, right of what
                {
                    size_t level = 0 ;

                    size_t p1 = p0 + r.what.size() - 1 ;
                    while( ++p1 != line.size() )
                    {
                        if( line[ p1 ] == '(' ) ++level ;
                        else if( line[ p1 ] == ')' ) --level ;

                        if( is_stop( line[ p1 ], level ) ) break ;
                    }

                    // +1 : jump over the whitespace
                    size_t const cut = p0 + r.what.size() + 1 ;
                    if( p1 == cut ) arg1 = "" ;
                    else arg1 = line.substr( cut, ( p1 - 1 ) - cut ) ;

                    end = p1 ;
                }

                if( r.single )
                {
                    // prefix
                    if( arg0.empty() )
                    {
                        line = line.replace( beg, end - beg,
                            r.with + " ( " + arg1 + " ) " ) ;
                    }
                    // postfix
                    else if( arg1.empty() )
                    {
                        line = line.replace( beg, end - beg,
                            r.with.substr(0, r.with.size()-1) + "_post: ( " + arg0 + " ) " ) ;
                    }
                    // else ambiguous operator 
                    else {}
                }
                else
                {
                    if( arg0.empty() )
                    {
                        line = line.replace( beg, end - beg,
                            r.with + " ( " + arg1 + " ) " ) ;
                    }
                    else if( arg1.empty() )
                    {
                        line = line.replace( beg, end - beg,
                            r.with + " ( " + arg0 + " ) " ) ;
                    }
                    else
                    {
                        line = line.replace( beg, end - beg,
                            r.with + " ( " + arg0 + " , " + arg1 + " ) " ) ;
                    }
                }

                // find another
                off = p0 + 1 ;
            }
        } ;

        // replacing operators #2
        {
            motor::vector< repl > repls =
            {
                { "++", ":inc:", true },
                { "--", ":dec:", true },
                { "+", ":add:", true },
                { "-", ":sub:", true },
                { "*", ":mmul:" },    // math multiplication
                { "'", ":cmul:" },    // component-wise multiplication
                { "/", ":div:" },
                { "+", ":add:" },
                { "-", ":sub:" },
                { ">>", ":rs:" },
                { "<<", ":ls:" },
                { "<", ":lt:" },
                { ">", ":gt:" },
                { "||", ":lor:" },
                { "&&", ":land:" }
            } ;

            auto is_stop = [&] ( char const t, size_t const l )
            {
                if( l == size_t( -1 ) )  return true ;
                if( l != 0 ) return false ;
                if( t == '*' || t == '/' || t == '+' || t == '-' ||
                    t == '=' || t == ';' || t == ',' || t == '>' || t == '<' ) return true ;
                return false ;
            } ;

            for( auto const & r : repls )
            {
                do_replacement( s0, r, is_stop ) ;
                do_replacement( s1, r, is_stop ) ;
                do_replacement( s2, r, is_stop ) ;
                do_replacement( s3, r, is_stop ) ;
            }
        }

        return ; 
    }
}

int main( int argc, char ** argv )
{
    this_file::test_1() ;
    
    this_file::test_2() ;
    this_file::test_3_extract_by_tokenize() ;
    this_file::test_4() ;
   
    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;

    return 0 ;
}
