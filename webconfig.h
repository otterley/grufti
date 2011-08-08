/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * webconfig.h - Configurations for the web interface
 *
 *****************************************************************************/
#ifndef WEBCONFIG_H
#define WEBCONFIG_H

/*
 * Congratulations!  If you're tinkering around in this area then you've
 * decided to try your hand at the web interface.  It's worth it.
 *  
 * The main requirement is that your ISP must support not only CGI
 * executables, but user-compiled CGI executables.  Many ISP's allow you
 * to access CGI executables only from a predefined directory, giving you
 * few choices in what you can do with them.  CGI executables you compile
 * yourself cannot be used in this situation.
 * 
 * Most likely your ISP supports user-compiled CGI executables.  In that
 * case, your CGI executables will either reside in a specific CGI directory
 * included with your account, or in the standard public_html directory.
 * Many web servers require that the extension .cgi be used for executables
 * found in public_html.
 * 
 * CGI stands for Command Gateway Interface, and is used to create 2-way
 * communication between a program (the CGI executable) and a web page.
 *
 * You'll need to perform the following steps to get your web page up
 * and running properly:
 *
 * 1) Edit the DEFINES found below.  They dictate where the CGI is located,
 *    your page's URL, where to find the running bot, and HTML properties.
 *    This file is included by both the bot and the CGI program so changes
 *    only need to be made in one location.
 *
 * 2) Type "make" to build your bot and "make cgi" to build the CGI
 *    executable.  Any changes you make here will require that you rebuild
 *    the bot and the CGI.
 *
 * 3) Type "make install" to install the bot to its new location
 *    and "make install-cgi" to install the CGI executable.
 *
 * 4) After your bot starts up, you should be able to access the page from
 *    the address you specified as WEB_MAIN_URL.  Use the index.html in
 *    the webstuff directory to get started.
 *
 * It may be a little confusing why we must do all the configuration here
 * instead of in config.gb. The reason is that the cgi program is executed
 * every time the user clicks a link anywhere in the web area.  It does not
 * run continuously like the bot itself, so having the cgi load a configuration
 * file every time it runs adds a lot of overhead, and increases the response
 * time for each page displayed.  It makes more sense to directly compile in
 * the settings, which is why we use this include file.
 *
 */

/*
 * Name of your web page area
 */
#define WEB_TITLE "Grufti Land"

/*
 * The main URL where users will access the page. (CHANGE THIS)
 */
#define WEB_MAIN_URL "http://dummymachine.org/~stever/grufti"

/*
 * The URL which points to your CGI executable. (CHANGE THIS)
 */
#define WEB_CGI_URL "http://dummymachine.org/~stever/gb.cgi"

/*
 * IP address of the machine where the bot is running. (CHANGE THIS)
 * (Use hostname if you don't know, but IP addresses will respond faster)
 */
#define WEB_GRUFTI_HOST "123.45.67.89"

/*
 * Gateway port where the bot is listening (set in config.gb)
 */
#define WEB_GRUFTI_PORT 8888

/*
 * The URL for the welcome image displayed on the main page.  This image
 * is included in the webstuff directory.  You can leave blank if you don't
 * want to use one.
 */
#define WEB_WELCOME_IMAGE "http://area23.org/grufti/gruftiland2.jpg"
#define WEB_WELCOME_IMAGE_WIDTH 570
#define WEB_WELCOME_IMAGE_HEIGHT 118

/*
 * HTML body properties
 */
#define WEB_BGCOLOR "#000000"
#define WEB_TEXT_COLOR "#abaca4"
#define WEB_LINK_COLOR "#cc3333"
#define WEB_VLINK_COLOR "#993333"
#define WEB_ALINK_COLOR "#ff0000"
#define WEB_BACKGROUND_IMAGE ""

/*
 * Font colors (1-header bars, 2-headers, 3-bold text, 4-title text)
 */
#define WEB_COLOR1 "#002020"
#define WEB_COLOR2 "#cc3333"
#define WEB_COLOR3 "#ffffff"
#define WEB_COLOR4 "#cdcec6"

#endif /* WEBCONFIG_H */
