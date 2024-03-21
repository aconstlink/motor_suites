

#include <motor/noise/permutation_table.h>

#include <motor/log/global.h>

using namespace motor::core::types ;


int main( int argc, char ** argv )
{
    {
        uint_t const seed = 127436 ;
        uint_t const bit = 8 ;
        uint_t const mixes = 3 ;

        motor::noise::permutation_table_t pt( seed, bit, mixes ) ;

        motor::log::global_t::status("permutation table: ") ;
        motor::log::global_t::status("seed : " + motor::to_string( seed ) ) ;
        motor::log::global_t::status("bit : " + motor::to_string( bit ) ) ;
        motor::log::global_t::status("mixes : " + motor::to_string( mixes ) ) ;

        motor::string_t entries ;
        for( size_t i=0;i<pt.get_num_entries(); ++i )
        {
            if( i % 10 == 0  ) entries += "\n" ;
            entries += motor::to_string( pt.permute_at( uint_t(i) ) ) + " ";
        }

        motor::log::global_t::status( entries ) ;
    }
   
    motor::log::global_t::deinit() ;
    motor::memory::global_t::dump_to_std() ;

    return 0 ;
}
