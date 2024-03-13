#include <motor/platform/cpu_id.h>

#include <motor/log/global.h>
#include <motor/memory/global.h>

#include <future>

int main( int argc, char ** argv )
{
    motor::log::global_t::status( "vendor : " + motor::platform::cpu_id_t::vendor_string() ) ;
    motor::log::global_t::status( "instruction sets: " + motor::platform::cpu_id_t::instruction_sets_string() ) ;

    motor::log::global_t::status( "brand : " + motor::platform::cpu_id_t::brand_string() ) ;

    return 0 ;
}