#ifndef __NEO_PIXEL_SERVER_DEFS_H__
#define __NEO_PIXEL_SERVER_DEFS_H__

// Change to force re-init
#define VERSION '8'

// Website types, need to be declared before including config
// NONE saves about 1.8kB of flash
// SHORT saves about 700b of flash
#define WEBSITE_NONE    0
#define WEBSITE_SHORT   1
#define WEBSITE_NORMAL  2

// Run-loop states
#define STATE_NONE          0
#define STATE_COLOR_CHANGE  1
#define STATE_COLOR_FADE    2
#define STATE_RAINBOW       3
#define STATE_RUN           4
#define STATE_CYLON         5
#define STATE_FIREWORKS     6
#define STATE_FLICKER       7
#define STATE_SPECTRUM      8

// Settings verification data
#define SETTINGS_MAGIC         "Nps"
#define SETTINGS_MAGIC_LENGTH  3

// Utility macros
#define NO_INIT_ON_RESET __attribute__ ((section (".noinit")))
#define NO_INLINE __attribute__ ((noinline))
//#define NO_INLINE

// Never include config directly
#include "NeoPixelServerConfig.h"

// Set up serial macros
#ifdef NO_SERIAL
    #pragma message ("No serial")
    #define SERIAL_PRINTLN(content)
    #define DEBUG_PRINTLN(content)   debugState.debugStage = content
#else
    #define SERIAL_PRINTLN(content)  Serial.println(content);
    #define DEBUG_PRINTLN(content)   { debugState.debugStage = content; Serial.println(content); }
#endif

#ifdef SERIAL_WAIT
    #pragma message ("Wait for serial before running")
#endif

// Set up effects macros
#ifdef NO_EFFECTS
    // Can also be defined separately
    #define NO_FADE
    #define NO_RAINBOW
    #define NO_FLICKER
    #define NO_FIREWORKS
    #define NO_RUN
    #define NO_CYLON
    #define NO_SPECTRUM
#endif

#ifdef NO_FADE
    #pragma message ("No fade")
#endif

#ifdef NO_RAINBOW
    #pragma message ("No rainbow")
#endif

#ifdef NO_FLICKER
    #pragma message ("No flicker")
#endif

#ifdef NO_FIREWORKS
    #pragma message ("No fireworks")
#endif

#ifdef NO_RUN
    #pragma message ("No run")
#endif

#ifdef NO_CYLON
    #pragma message ("No cylon")
    #define ERROR()  colorWipe()
#else
    #define ERROR()  cylon()
#endif

#ifdef NO_SPECTRUM
    #pragma message ("No spectrum")
    #define FIX_BRIGHTNESS()
#else
    #define FIX_BRIGHTNESS()  setBrightness(settings.getBrightness())
#endif

// Ensure a complete config
#if !defined(CC3000_PIN_IRQ) || !defined(CC3000_PIN_VBAT) || !defined(CC3000_PIN_CS)
    #error Incomplete CC3000 hardware config
#endif

#if !defined(WLAN_SECURITY) || !defined(WLAN_SSID) || !defined(WLAN_PASS)
    #error Incomplete Wifi setup
#endif

#if !defined(NEOPIXEL_PIN) || !defined(NEOPIXEL_OFFSET) || !defined(NEOPIXEL_COUNT)
    #error Incomplete NeoPixel hardware setup
#endif

#ifndef NEOPIXEL_BRIGHTNESS
    #define NEOPIXEL_BRIGHTNESS      128
#endif

#ifndef NEOPIXEL_MAX_BRIGHTNESS
    #define NEOPIXEL_MAX_BRIGHTNESS  255
#endif

#ifndef NEOPIXEL_USB_BRIGHTNESS
    #define NEOPIXEL_USB_BRIGHTNESS  8
#endif

#ifndef NEOPIXEL_COLOR_DELAY
    #define NEOPIXEL_COLOR_DELAY     40
#endif

#ifndef NEOPIXEL_RAINBOW_DELAY
    #define NEOPIXEL_RAINBOW_DELAY   20
#endif

#ifndef NEOPIXEL_FLICKER_DELAY
    #define NEOPIXEL_FLICKER_DELAY   100
#endif

#if !defined(SPECTRUM_IN_PIN) && !defined(NO_SPECTRUM)
    #error No spectrum display input pin defined
#endif

#ifndef WEBSERVER_PORT
    #define WEBSERVER_PORT  80
#endif

#ifndef WEBSITE_TYPE
    #define WEBSITE_TYPE  WEBSITE_NORMAL
#endif

#if WEBSITE_TYPE == WEBSITE_NONE
    #pragma message ("No website")
    #define PRINT_CONTENT(content)
#else
    #if WEBSITE_TYPE == WEBSITE_SHORT
        #pragma message ("Shortened website")
    #endif

    #ifdef WEBSITE_MINIMAL_CSS
        #pragma message ("Minimal CSS website")
    #endif

    #define PRINT_CONTENT(content)  _sopt_clientPrintString(content)
#endif

#ifdef START_ENABLED
    #pragma message ("Starting enabled")
#endif

#endif
