//
// Created by matthieu on 28/01/17.
//
#pragma once

#define __MESSAGE_DECORATOR "\x9D\x31\x7B\x2C\x54\xFA\x85\x0E\xAD\x65\x1B"

class MenuMessages
{
  public:
    #define MENUMESSAGE_CONTROLLER_CONF_HELP_MSG "Configure an associated controller. Your controller has to be associated / plugged before."
    #define MENUMESSAGE_CONTROLLER_BT_HELP_MSG "Pair a bluetooth controller with your recalbox. Your controller must be in pairing mode."
    #define MENUMESSAGE_CONTROLLER_FORGET_HELP_MSG "Forget all paired bluetooth controllers. You will have to pair your controllers again, but this option can help if you have issues to reconnect a controller, which is already paired."
    #define MENUMESSAGE_CONTROLLER_DRIVER_HELP_MSG "Change the driver if your pad is not working at all or not working properly in-game."
    #define MENUMESSAGE_CONTROLLER_PADOSD_HELP_MSG "Always display Pad OSD whether you are in pad menus or not."
    #define MENUMESSAGE_CONTROLLER_PADOSDTYPE_HELP_MSG "Change the icon used to display pad OSD."
    #define MENUMESSAGE_CONTROLLER_AUTOPAIRONBOOT_HELP_MSG "Activates Bluetooth pairing automatically each time you boot."
    #define MENUMESSAGE_CONTROLLER_RUMBLE_HELP_MSG "Enable rumble with compatible controllers."

    #define MENUMESSAGE_TATE_VIRTUALSYSTEM_MSG "Shows the Tate virtual system in the systems list."
    #define MENUMESSAGE_TATE_GAMELISTS_MSG "Shows only games playable in tate mode in gamelists."
    #define MENUMESSAGE_TATE_GAMESROTATION_MSG "Proceed to a screen rotation in games tate compatible."
    #define MENUMESSAGE_TATE_SCREEN_ROTATION "Proceed to a complete screen rotation, for frontend and games."

    #define MENUMESSAGE_SCRAPER_FINAL_POPUP "Scraping complete! {PROCESSED} games processed.\n\n{SUCCESS} game(s) scraped or updated\n{NOTFOUND} game(s) not found...\n{ERRORS} request/download errors\n\n{TEXTINFO} Text information updated\n{IMAGES} images and {VIDEOS} videos downloaded\n{MEDIASIZE} of media saved"
    #define MENUMESSAGE_SCRAPER_FINAL_QUOTA "You reached your daily quota of scraping request.\nAll your today's scrapes have been saved anyway.\n\nStart scraping again tomorrow.\nDont forget to select 'update' and not 'scrape all'"
    #define MENUMESSAGE_SCRAPER_FINAL_FATAL "A fatal error occured while scraping your game! It may be related to server issues or bad login/password.\n\nTry again in a few moment or fix your credentials if required."
    #define MENUMESSAGE_SCRAPER_FINAL_DISKFULL "Your share partition is almost full.\nThe scraper stopped automatically.\n\nRemove unused games, media, files to make room before running the scraper again!"

    static const char* LICENCE_MSG;
    static const int   LICENCE_MSG_SIZE;

    #define LICENSE_ORIGINAL_TEXT "Redistribution and use of the RECALBOX code or any derivative works are permitted provided that the following conditions are met:\n" \
                                  "\n" \
                                  "- Redistributions may not be sold without authorization, nor may they be used in a commercial product or activity.\n" \
                                  "- Redistributions that are modified from the original source must include the complete source code, including the source code for all components used by a binary built from the modified sources.\n" \
                                  "- Redistributions must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.\n" \
                                  "\n" \
                                  "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS \"AS IS\" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
};
