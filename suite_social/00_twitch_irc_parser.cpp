

#include <motor/social/twitch/irc_parser.hpp>

using namespace motor::core::types ;


int main( int argc, char ** argv )
{
    motor::social::twitch::irc_parser parser ;

    {
        motor::string_t const irc_message =
            "PING :tmi.twitch.tv\r\n";

        parser.parse( irc_message ) ;
        int bp = 0 ;
    }

    parser.clear() ;

    {
        // multi message - multi irc messages should be 
        // separated by \r\n
        motor::string_t const some_messages =
            ":00_ava!00_ava@00_ava.tmi.twitch.tv JOIN #aconstlink\r\n"
            ":markzynk!markzynk@markzynk.tmi.twitch.tv JOIN #aconstlink\r\n"
            ":sukoxi!sukoxi@sukoxi.tmi.twitch.tv JOIN #aconstlink\r\n"
            ":excyl!excyl@excyl.tmi.twitch.tv JOIN #aconstlink\r\n"
            ":00_aaliyah!00_aaliyah@00_aaliyah.tmi.twitch.tv JOIN #aconstlink\r\n"
            ":d0nk7!d0nk7@d0nk7.tmi.twitch.tv JOIN #aconstlink\r\n"
            ":drapsnatt!drapsnatt@drapsnatt.tmi.twitch.tv JOIN #aconstlink\r\n"
            ":8roe!8roe@8roe.tmi.twitch.tv JOIN #aconstlink\r\n"
            ":asmr_miyu!asmr_miyu@asmr_miyu.tmi.twitch.tv JOIN #aconstlink\r\n"
            ;

        parser.parse( some_messages ) ;

        int bp = 0 ;

    }

    parser.clear() ;

    {
        // contains capabilities
        motor::string_t const twitch_irc_user_msg =
            "@badge-info=;badges=;client-nonce=12b23e567f8b7e1b1234562d4cb39ad7;color=;"
            "display-name=user;emotes=;first-msg=0;flags=;id=12345678-1v34-12b4-123n-123h56fcd57f;"
            "mod=0;returning-chatter=0;room-id=100043230;subscriber=0;tmi-sent-ts=1714732875386;"
            "turbo=0;user-id=12345678;user-type= :user!user@user.tmi.twitch.tv"
            "PRIVMSG #aconstlink :Oh nice. Hellllllllllllllo World." ;

        parser.parse( twitch_irc_user_msg ) ;
    }

    parser.clear() ;

    {
        motor::string_t const initial_response =
            ":aconstlink!aconstlink@aconstlink.tmi.twitch.tv JOIN #aconstlink\r\n"
            ":aconstlink.tmi.twitch.tv 353 aconstlink = #aconstlink :aconstlink\r\n"
            ":aconstlink.tmi.twitch.tv 366 aconstlink #aconstlink :End of /NAMES list\r\n"
            ":tmi.twitch.tv CAP * ACK :twitch.tv/membership twitch.tv/tags twitch.tv/commands\r\n"

            "@badge-info=;badges=broadcaster/1;color=#0000FF;display-name=aconstlink;emote-sets=0,300374282;"
            "id=af710d46-f556-4c92-ae9c-75c4f384ce0f;mod=0;subscriber=0;"
            "user-type= :tmi.twitch.tv USERSTATE #aconstlink\r\n"

            "@msg-id=unrecognized_cmd :tmi.twitch.tv NOTICE #aconstlink :Unrecognized command: /color\r\n"

            "@badge-info=;badges=broadcaster/1;color=#0000FF;display-name=aconstlink;"
            "emote-sets=0,300374282;id=4ebc42c9-3e6f-4ccc-9f9e-0b25c823ddba;mod=0;subscriber=0;"
            "user-type= :tmi.twitch.tv USERSTATE #aconstlink\r\n" ;
    }

    

    return 0 ;
}