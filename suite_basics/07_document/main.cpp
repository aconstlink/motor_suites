
#include <motor/core/document.hpp>

#include <motor/memory/global.h>
#include <motor/log/global.h>

int main( int argc, char ** argv )
{
    {
        motor::core::document rd( "This is a line\n\t{\n\t   this   also\n\t\t{\n  \t\tand this \n\t}\n}" ) ;

        // line by line
        rd.for_each_line( [&]( motor::core::document::line_view const & lv )
        {
            int bp = 0 ;
        } ) ;

        // token by token
        rd.for_each_token( [&] ( size_t const ln, size_t const tn, std::string_view const & token )
        {
            int bp = 0 ;
        } ) ;

        // token by line
        rd.for_each_line( [&] ( motor::core::document::line_view const & lv )
        {
            for( size_t t = 0 ; t < lv.get_num_tokens(); ++t )
            {
                auto const token = lv.get_token( t ) ;
                int bp = 0 ;
            }
        } ) ;

        int bp =0 ;
    }

    {
        motor::core::document rd ;

        rd.println( "hello line   " ) ;
        rd.println( "" ) ;
        rd.println( "in vec3_t pos : position ;" ) ;
        rd.println( " " ) ;
        rd.println( "{" ) ;
        {
            rd.section_open() ;
            rd.println( "hello another line " ) ;
            rd.section_close() ;
        }
        rd.println( "}" ) ;

        // token by token
        rd.for_each_token( [&] ( size_t const ln, size_t const tn, std::string_view const & token )
        {
            std::cout << token << " " ;
        } ) ;

        motor::log::global::status( rd.to_string() );
    }

    motor::log::global::deinit();

    return motor::memory::global::dump_to_std() != 0 ? 1 : 0 ;    
}
