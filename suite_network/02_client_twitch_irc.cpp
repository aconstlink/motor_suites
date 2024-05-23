


#include <motor/network/typedefs.h>
#include <motor/platform/network/network_module_creator.hpp>
#include <motor/social/twitch/twitch_irc_client.h>
#include <motor/io/database.h>

using namespace motor::core::types ;

bool_t done = false ;

int main( int argc, char ** argv )
{
    auto * mod = motor::platform::network_module_creator::create() ;

    motor::io::database_mtr_t db = motor::shared( 
        motor::io::database( motor::io::path_t( DATAPATH ), "./working", "data" ) ) ;

    // platform not supported
    if ( mod == nullptr ) return 1 ;

    auto mtr = motor::shared( motor::social::twitch::twitch_irc_bot( motor::move( db ) ) ) ;

    mod->create_tcp_client( motor::network::create_tcp_client_info { 
        "my_twitch_client", 
        motor::network::ipv4::binding_point_host { "6667", "irc.chat.twitch.tv"},
        //motor::network::ipv4::binding_point_host { "80", "irc-ws.chat.twitch.tv" },
        motor::share( mtr ) } ) ;
    
    motor::social::twitch::twitch_irc_bot::commands_t comms ;

    while ( !done )
    {
        std::this_thread::sleep_for( std::chrono::seconds( 1 ) ) ;
        if( mtr->swap_commands( comms ) )
        {
            for( auto const & c : comms )
            {
                motor::log::global_t::status(
                    " Got request from " + c.user + "; commands: " + c.name ) ;

                if( c.name == "commands" )
                {
                    mtr->send_response("!echo !discord") ;
                }
                else if ( c.name == "discord" )
                {
                    mtr->send_response( "https://discord.gg/FnxJYWsN" ) ;
                }
                else if( c.name == "test" )
                {
                    mtr->send_announcement( "Mega Announcement!" ) ;
                }
                else
                {
                    mtr->send_response( "NotLikeThis unrecognized command. Try !commands" ) ;
                }
            }
        }
    }

    motor::release( mtr ) ;

    return 0 ;
}