
#include <motor/memory/global.h>
#include <motor/log/global.h>

#include <motor/std/string>
#include <motor/std/vector>

int main( int argc, char ** argv )
{
    {
        motor::string_t my_string( "hello observer" ) ;
        motor::vector< size_t > some_values ;
        for ( size_t i = 0; i < 10000; ++i ) some_values.push_back( i ) ;
    }

    motor::memory::observer_ptr_t obs = motor::memory::global_t::get_observer() ;
    
    {
        auto const data = obs->swap_and_clear() ;
        // make something with data
    }

    {
        motor::string_t some_string ;
        some_string += "Hello" ;
        some_string += "World" ;
        some_string += motor::string_t("Observer ") + motor::string_t("too") ;
    }
    
    {
        auto const data = obs->swap_and_clear() ;
        // make something with data
    }

    {
        motor::string_t some_string = "Hello " ;
        some_string += "again" ;
    }

    {
        auto const data = obs->swap_and_clear() ;
        // make something with data
    }

    return 0 ;
}
