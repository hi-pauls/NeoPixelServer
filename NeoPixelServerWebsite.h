#ifndef __NEO_PIXEL_SERVER_WEBSITE_H__
#define __NEO_PIXEL_SERVER_WEBSITE_H__

#include <avr/pgmspace.h>

// Send the length attribute (not necessary)
//#define WEBSITE_HEADER_SEND_LENGTH
#if defined(WEBSITE_HEADER_SEND_LENGTH) && ((WEBSITE_TYPE == WEBSITE_NONE) || (WEBSITE_TYPE == WEBSITE_SHORT))
    // Never send length in short or none mode
    #undef WEBSITE_HEADER_SEND_LENGTH
#endif

// Website definitions
const char PROGMEM website_http_data[] =
#if defined(WEBSITE_HEADER_SEND_LENGTH)
"HTTP/1.0 200 OK\n\
Server: arduino\n\
Cache-Control: no-store, no-cache, must-revalidate\n\
Pragma: no-cache\n\
Connection: close\n\
Access-Control-Allow-Origin: *\n\
Content-Type: text/html\n\
Content-Length: 856\n";
#else
"HTTP/1.0 200 OK\n\
Server: arduino\n\
Cache-Control: no-store, no-cache, must-revalidate\n\
Pragma: no-cache\n\
Connection: close\n\
Access-Control-Allow-Origin: *\n\
Content-Type: text/html\n\n";
#endif

const char PROGMEM website_doctype_data[] =
#if WEBSITE_TYPE == WEBSITE_NONE || WEBSITE_TYPE == WEBSITE_SHORT
"";
#define WEBSITE_DOCTYPE_LENGTH   (0)
#else
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\">";
#define WEBSITE_DOCTYPE_LENGTH   (50)
#endif

const char PROGMEM website_header_data[] =
#if WEBSITE_TYPE == WEBSITE_NONE
"<html/>";
#define WEBSITE_HEADER_LENGTH (7)
#elif WEBSITE_TYPE == WEBSITE_SHORT
"<html>\
<head>\
<style>\
a,p,h3{display:block}\
</style>\
</head>\
<body>";
#define WEBSITE_HEADER_LENGTH    (6 + 6 + 7 + 21 + 8 + 7 + 6)
#else
#ifdef WEBSITE_MINIMAL_CSS
"<html>\
<head>\
<title>Lights</title>\
<meta name='viewport' content='width=320,maximum-scale=1.0,minimum-scale=1.0,user-scalable=false'/>\
<style>\
a,p,h3{display:block}\
</style>\
</head>\
<body>";
#define WEBSITE_HEADER_LENGTH    (6 + 6 + 21 + 99 + 7 + 21 + 8 + 7 + 6)
#else
"<html>\
<head>\
<title>Lights</title>\
<meta name='viewport' content='width=320,maximum-scale=1.0,minimum-scale=1.0,user-scalable=false'/>\
<style>\
div{width:19em;margin:auto;}\
a,p,h3{text-align:center;font-size:2em;display:block}\
a:focus,a:hover,a:visited,a:link{font-style:normal;font-weight:bold;text-decoration:none;margin:5px;padding:5px;background:#666;color:#FFF;}\
</style>\
</head>\
<body>";
#define WEBSITE_HEADER_LENGTH    (6 + 6 + 21 + 99 + 7 + 28 + 53 + 140 + 8 + 7 + 6)
#endif
#endif

const char PROGMEM website_options_data[] =
#if WEBSITE_TYPE == WEBSITE_NONE
"";
#define WEBSITE_OPTIONS_LENGTH   (0)
#elif WEBSITE_TYPE == WEBSITE_SHORT
"<div>\
<p>\
Brightness: ";
#define WEBSITE_OPTIONS_LENGTH   (5 + 3 + 12)
#else
"<div>\
<h3>Mode</h3>\
<a href='c0'>Off</a>\
<a href='cFFCC66'>On</a>\
<a href='cFFFFFF'>White</a>\
<a href='cFF0000'>Red</a>\
<a href='cFF00'>Green</a>\
<a href='cFF'>Blue</a>\
<a href='fd'>Fade</a>\
<a href='rb'>Rainbow</a>\
<a href='cn'>Candle</a>\
<a href='fw'>Fireworks</a>\
<a href='rn'>Run</a>\
<a href='cy'>Cylon</a>\
<a href='sr'>Strobe</a>\
<a href='sp'>Spectrum</a>\
<a href='st'>Stop</a>\
<h3>Brightness</h3>\
<a href='b8'>8</a>\
<a href='b32'>32</a>\
<a href='b128'>128</a>\
<a href='b160'>160</a>\
<a href='b192'>192</a>\
<a href='b255'>255</a>\
<p>\
Brightness: ";
#define WEBSITE_OPTIONS_LENGTH   (5 + 13 + 20 + 24 + 27 + 25 + 25 + 22 + 21 + 24 + 26 + 20 + 22 + 23 + 25 + 21 + 19 + 18 + 20 + 24 + 24 + 24 + 24 + 3 + 12)
#endif

const char PROGMEM website_color_data[] =
#if WEBSITE_TYPE == WEBSITE_NONE
"";
#define WEBSITE_COLOR_LENGTH   0
#else
"<br/>Color: #";
#define WEBSITE_COLOR_LENGTH   13
#endif

const char PROGMEM website_version_data[] =
#if WEBSITE_TYPE == WEBSITE_NONE
"";
#define WEBSITE_VERSION_LENGTH   0
#else
"<br/>V/R/D/Prev: ";
#define WEBSITE_VERSION_LENGTH   17
#endif

const char PROGMEM website_reboot_data[] =
#if WEBSITE_TYPE == WEBSITE_NONE
"";
#define WEBSITE_REBOOT_LENGTH   0
#else
", ";
#define WEBSITE_REBOOT_LENGTH   2
#endif

const char PROGMEM website_disconnect_data[] =
#if WEBSITE_TYPE == WEBSITE_NONE
"";
#define WEBSITE_DISCONNECT_LENGTH   0
#else
", ";
#define WEBSITE_DISCONNECT_LENGTH   2
#endif

const char PROGMEM website_prevState_data[] =
#if WEBSITE_TYPE == WEBSITE_NONE
"";
#define WEBSITE_PREVSTATE_LENGTH   0
#else
", ";
#define WEBSITE_PREVSTATE_LENGTH   2
#endif

const char PROGMEM website_command_data[] =
#if WEBSITE_TYPE == WEBSITE_NONE
"";
#define WEBSITE_COMMAND_LENGTH   0
#else
"<br/>Command: ";
#define WEBSITE_COMMAND_LENGTH   15
#endif

const char PROGMEM website_footer_data[] =
#if WEBSITE_TYPE == WEBSITE_NONE
"";
#define WEBSITE_FOOTER_LENGTH    0
#else
"</p>\
</div>\
</body>\
</html>";
#define WEBSITE_FOOTER_LENGTH    (4 + 6 + 7 + 7)
#endif
#define WEBSITE_BASE_LENGTH      (WEBSITE_DOCTYPE_LENGTH +   \
                                 WEBSITE_HEADER_LENGTH +     \
                                 WEBSITE_OPTIONS_LENGTH +    \
                                 WEBSITE_COLOR_LENGTH +      \
                                 WEBSITE_VERSION_LENGTH +    \
                                 WEBSITE_REBOOT_LENGTH +     \
                                 WEBSITE_PREVSTATE_LENGTH +  \
                                 WEBSITE_COMMAND_LENGTH +    \
                                 WEBSITE_FOOTER_LENGTH)
#define WEBSITE_VARIABLE_LENGTH  26
#endif
