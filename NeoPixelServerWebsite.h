#ifndef __NEO_PIXEL_SERVER_WEBSITE_H__
#define __NEO_PIXEL_SERVER_WEBSITE_H__

#include <avr/pgmspace.h>

// Save about 1.8kB of flash
//#define WEBSITE_NONE

// Save about 700b of flash
//#define WEBSITE_SHORT

// Send the length attribute (not necessary)
//#define WEBSITE_HEADER_SEND_LENGTH

// Sanitize the config a bit
#if defined(WEBSITE_NONE) && defined(WEBSITE_SHORT)
    // Make sure only one content configuration is active. None over short.
    #undef WEBSITE_SHORT
#endif
#if (defined(WEBSITE_NONE) || defined(WEBSITE_SHORT)) && defined(WEBSITE_HEADER_SEND_LENGTH)
    // Never send length in short mode
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
Content-Type: text/html\n\
Content-Length: 936\n";
#else
"HTTP/1.0 200 OK\n\
Server: arduino\n\
Cache-Control: no-store, no-cache, must-revalidate\n\
Pragma: no-cache\n\
Connection: close\n\
Content-Type: text/html\n\n";
#endif

const char PROGMEM website_doctype_data[] =
#ifdef WEBSITE_SHORT
"";
#define WEBSITE_DOCTYPE_LENGTH   0
#else
"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\">";
#define WEBSITE_DOCTYPE_LENGTH   50
#endif

const char PROGMEM website_header_data[] =
#if defined(WEBSITE_NONE)
"<html/>";
#elif defined(WEBSITE_SHORT)
"<html>\
<head/>\
<body>";
#define WEBSITE_HEADER_LENGTH    19
#else
"<html>\
<head>\
<title>Lights</title>\
<meta name='viewport' content='width=320, maximum-scale=1.0, minimum-scale=1.0, user-scalable=false'/>\
</head>\
<body>";
#define WEBSITE_HEADER_LENGTH    148
#endif

const char PROGMEM website_options_data[] =
#ifdef WEBSITE_SHORT
"<p>\
Brightness: ";
#define WEBSITE_OPTIONS_LENGTH   16
#else
"<h3>Light:</h3>\
<p>\
<a href='c0'>Off</a><br/>\
<a href='c16777050'>Light</a><br/>\
<a href='c16777215'>White</a><br/>\
<a href='c16711680'>Red</a><br/>\
<a href='c65280'>Green</a><br/>\
<a href='c255'>Blue</a><br/>\
<a href='fd'>Fade</a><br/>\
<a href='rb'>Rainbow</a><br/>\
<a href='fw'>Fireworks</a><br/>\
<a href='rn'>Run</a><br/>\
<a href='cy'>Cylon</a><br/>\
<a href='cn'>Candle</a><br/>\
<a href='sp'>Spectrum</a><br/>\
<br/>\
<a href='st'>Stop</a>\
</p>\
<hr/>\
<h3>Brightness:</h3>\
<p>\
<a href='b32'>32</a><br/>\
<a href='b64'>64</a><br/>\
<a href='b96'>96</a><br/>\
<a href='b128'>128</a><br/>\
<a href='b160'>160</a><br/>\
<a href='b192'>192</a><br/>\
<a href='b224'>224</a><br/>\
<a href='b255'>255</a>\
</p>\
<hr/>\
<p>\
Brightness: ";
#define WEBSITE_OPTIONS_LENGTH   675
#endif

const char PROGMEM website_color_data[] =
  "<br/>Color: #";
#define WEBSITE_COLOR_LENGTH   13

const char PROGMEM website_command_data[] =
  "<br/>Command: ";
#define WEBSITE_COMMAND_LENGTH   14

const char PROGMEM website_footer_data[] =
"</p>\
</body>\
</html>";
#define WEBSITE_FOOTER_LENGTH    18
#define WEBSITE_BASE_LENGTH      (WEBSITE_DOCTYPE_LENGTH + \
                                 WEBSITE_HEADER_LENGTH +   \
                                 WEBSITE_OPTIONS_LENGTH +  \
                                 WEBSITE_COLOR_LENGTH +    \
                                 WEBSITE_COMMAND_LENGTH +  \
                                 WEBSITE_FOOTER_LENGTH)
#define WEBSITE_VARIABLE_LENGTH  18
#endif
