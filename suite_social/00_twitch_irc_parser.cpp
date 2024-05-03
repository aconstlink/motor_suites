

#include <motor/social/twitch/irc_parser.hpp>

using namespace motor::core::types ;


int main( int argc, char ** argv )
{
    motor::social::twitch::irc_parser parser ;

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

    // contains capabilities
    motor::string_t const twitch_irc_user_msg =
        "@badge-info=;badges=;client-nonce=88b76e4b7f8b7e1b4020662d4cb39ad7;color=;"
        "display-name=excyl;emotes=;first-msg=0;flags=;id=2e96817d-2cd2-4044-998b-fd64973cd57f;"
        "mod=0;returning-chatter=0;room-id=100043230;subscriber=0;tmi-sent-ts=1714732875386;"
        "turbo=0;user-id=39633589;user-type= :excyl!excyl@excyl.tmi.twitch.tv"
        "PRIVMSG #aconstlink :Oh nice. Ja berhmte Worte von Entwicklern. Eigenlich solllte... ist auch so einer" ;

    

    return 0 ;
}