#include <SPI.h>
#include "Adafruit_NeoPixel.h"
#include "NeoPixelServerConfig.h"
#include "NeoPixelServerWebsite.h"
#include "Adafruit_CC3000.h"
#include "Adafruit_CC3000_Server.h"

// Save about 600kB of flash
#define NO_SERIAL
#ifdef NO_SERIAL
    #define SERIAL_PRINTLN(content)
#else
    #define SERIAL_PRINTLN(content)  Serial.println(content);
#endif

// Save about 5.2kB of flash
//#define NO_EFFECTS
#ifdef NO_EFFECTS
    #define NO_CANDLE
    #define NO_SPECTRUM

    #define ERROR()  colorWipe()
#else
    #define ERROR()  cylon()
#endif

// Save about 100B of flash
//#define NO_CANDLE

// Save about 2.7kB of flash
#define NO_SPECTRUM
#ifdef NO_SPECTRUM
    #define FIX_BRIGHTNESS()
#else
    #include "FHT.h"

    #define MIC_PIN A5
    #define FIX_BRIGHTNESS()  setBrightness(brightness)
#endif

// NeoPixel Strip setup
#define NEOPIXEL_PIN            6
#define NEOPIXEL_COLOR_DELAY    40
#define NEOPIXEL_RAINBOW_DELAY  20
#define NEOPIXEL_FLICKER_DELAY  100
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Wifi adapter setup
#define CC3000_IRQ   3
#define CC3000_VBAT  5
#define CC3000_CS    10
Adafruit_CC3000 cc3000 = Adafruit_CC3000(CC3000_CS, CC3000_IRQ, CC3000_VBAT, SPI_CLOCK_DIV4);

// Web server setup
#define WEBSERVER_PORT  80
Adafruit_CC3000_Server webServer(WEBSERVER_PORT);

#ifdef WEBSITE_NONE
    #define PRINT_CONTENT(content)
#else
    #define PRINT_CONTENT(content)  client.fastrprint(content)
#endif

#define STATE_NONE          0
#define STATE_COLOR_CHANGE  1
#define STATE_COLOR_FADE    2
#define STATE_RAINBOW       3
#define STATE_RUN           4
#define STATE_CYLON         5
#define STATE_FIREWORKS     6
#define STATE_FLICKER       7
#define STATE_SPECTRUM      8

// Settings
uint8_t brightness;
int32_t color = 0;
int32_t variance = 0;
int8_t direction = 1;

#ifndef NO_SPECTRUM
    uint8_t cutoff = 100;
    uint8_t sensitivity = 10;
    uint8_t frequencyRolloff = 4;
#endif

// State
uint8_t state;
int16_t offset = 0;
uint32_t lastChange = 0;

void setupError()
{
    color = 0xFF0000;
    offset = 0;
    while (true)
    {
        ERROR();
        delay(20);
    }
}

#ifndef NO_SPECTRUM
void setupMicrophone()
{
    // Get the analog input pin for the microphone
    uint8_t pin = MIC_PIN;
#ifdef __AVR_ATmega32u4__
    if (pin >= 18)
    {
        // This is a pin, convert it to an analog channel
        pin -= 18;
    }

    // ATmega32u4 boards have a mapped pin out
    pin = analogPinToChannel(pin);
#else
    if (pin >= 14)
    {
        pin -= 14;
    }
#endif

    // Configure the analog input for free running mode
    ADCSRB = (ADCSRB & ~(1 << MUX5)) | (((pin >> 3) & 0x01) << MUX5);
    ADCSRB = (1 << ADTS2) | (1 << ADTS0);
    ADMUX = (DEFAULT << 6) | (pin & 0x07);
    DIDR0 = 1 << pin;
    ADCSRA |= (1 << ADATE) | (1 << ADEN) | (1 << ADSC) | 0x07;

    // Set the timing for ADC sampling to a clock divider of 127
    ADCSRA |= 0x07;

}
#endif

void setup()
{
    // Initialize the strip.
    strip.begin();
    strip.show();
    setBrightness(NEOPIXEL_BRIGHTNESS);

#ifndef NO_SPECTRUM
    // Initialize the microphone
    setupMicrophone();
#endif

#ifndef NO_SERIAL
    Serial.begin(115200);
    //while (! Serial);
#endif

    if (! cc3000.begin())
    {
        SERIAL_PRINTLN(F("!50")); // Init
        setupError();
    }

    if (! cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY))
    {
        SERIAL_PRINTLN(F("!51")); // Associate
        setupError();
    }

    // Block until DHCP address data is available, or forever, if it isn't.
    color = 0xFF;
    offset = 0;
    while (! cc3000.checkDHCP())
    {
        ERROR();
        delay(20);
    }

    // Display the IP address DNS, Gateway, etc.
    while (! displayConnectionDetails())
    {
        ERROR();
        delay(20);
    }

    // Start listening for connections
    webServer.begin();

    // Initialize the strip.
    offset = 0;
    color = 0xFF00;
    direction = 1;
    state = STATE_RUN;
}

bool displayConnectionDetails(void)
{
    uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;

    if(! cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
    {
        return false;
    }

#ifndef NO_SERIAL
    cc3000.printIPdotsRev(ipAddress);
    Serial.println();
#endif

    return true;
}

void setBrightness(uint8_t bright)
{
    if (bright > NEOPIXEL_MAX_BRIGHTNESS)
    {
        bright = NEOPIXEL_MAX_BRIGHTNESS;
    }

    strip.setBrightness(bright);
    brightness = bright;

    if (state == STATE_NONE)
    {
        state = STATE_COLOR_CHANGE;
        offset = 0;
    }
}

bool checkTime(uint32_t timeout)
{
    if ((millis() - lastChange) < timeout)
    {
        // Wait another receive cycle.
        return false;
    }

    lastChange = millis();
    return true;
}

// Fill the dots one after the other with a color
void colorWipe()
{
    if (! checkTime(NEOPIXEL_COLOR_DELAY))
    {
        return;
    }

    strip.setPixelColor(offset, color);
    strip.show();

    offset += direction;
    if (offset > NEOPIXEL_COUNT)
    {
        // Done!
        state = STATE_NONE;
    }
}

void rainbow()
{
    if (! checkTime(NEOPIXEL_RAINBOW_DELAY))
    {
        return;
    }

    for(uint8_t i = 0; i < NEOPIXEL_COUNT; i++)
    {
        strip.setPixelColor(i, Wheel((i + offset) & 255));
    }
    strip.show();

    offset = (offset + direction) % 256;
}

// Slightly different, this makes the rainbow equally distributed throughout
void fadeColors()
{
    if (! checkTime(NEOPIXEL_RAINBOW_DELAY))
    {
        return;
    }

    for(uint8_t i = 0; i < NEOPIXEL_COUNT; i++)
    {
        strip.setPixelColor(i, Wheel(((i * 256 / NEOPIXEL_COUNT) + offset) & 255));
    }
    strip.show();

    offset = (offset + direction) % 256;
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos)
{
    if(WheelPos < 85)
    {
        return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    }
    else if(WheelPos < 170)
    {
        WheelPos -= 85;
        return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    else
    {
        WheelPos -= 170;
        return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
}

void run()
{
    if (! checkTime(NEOPIXEL_RAINBOW_DELAY))
    {
        return;
    }

    strip.setPixelColor(offset, color);
    strip.setPixelColor(offset - (1 * direction), (color >> 1) & 0x7F7F7F);
    strip.setPixelColor(offset - (2 * direction), (color >> 2) & 0x3F3F3F);
    strip.setPixelColor(offset - (3 * direction), (color >> 3) & 0x1F1F1F);
    strip.setPixelColor(offset - (4 * direction), 0);
    strip.show();

    offset += direction;
    if ((offset < -4) || (offset > NEOPIXEL_COUNT + 4))
    {
        state = STATE_NONE;
    }
}

void cylon()
{
    run();

    if (offset < 0 || offset > NEOPIXEL_COUNT)
    {
        direction *= -1;
        offset += direction;
    }
}

void fireworks()
{
    if (color < 1 && random(0, 300) < 1)
    {
        color = strip.Color(random(0, 255), random(0, 255), random(0, 255));
    }

    if (color > 0)
    {
        run();
    }

    if (state == STATE_NONE)
    {
        state = STATE_FIREWORKS;
        offset = 0;
        color = 0;
    }
}

void flicker()
{
    if (! checkTime(NEOPIXEL_FLICKER_DELAY))
    {
        return;
    }

    uint8_t varr = (variance >> 16) & 0xFF;
    uint8_t varg = (variance >> 8) & 0xFF;
    uint8_t varb = variance & 0xFF;

    for (uint8_t i = 0; i < NEOPIXEL_COUNT; i++)
    {
        uint8_t r = ((color >> 16) & 0xFF) + random(-varr, varr);
        uint8_t g = ((color >> 8) & 0xFF) + random(-varg, varg);
        uint8_t b = (color & 0xFF) + random(-varb, varb);
        strip.setPixelColor(i, strip.Color(r, g, b));
    }

    strip.show();
}

#ifndef NO_SPECTRUM
void spectrum()
{
    // Get the required number of continuous samples
    int16_t sample;
    cli();
    for (int i = 0 ; i < FHT_N ; i++)
    {
        // ADSC is cleared when the conversion finishes
        while (bit_is_set(ADCSRA, ADSC));
        ADCSRA |= (1 << ADSC);
        sample = ADCL;
        sample |= (ADCH << 8);

        // apply sensitivity
        sample = sample * sensitivity / 10;

        // convert ADC data to a 16bit signed int
        sample -= 0x0200;
        sample <<= 6;

        fht_input[i] = sample;
    }

    sei();

    fht_window();
    fht_reorder();
    fht_run();
    fht_mag_log();

    uint8_t cut;
    for (uint8_t i = 0; i < (FHT_N >> 1); i++)
    {
        cut = cutoff + (32 >> i) - ((i * frequencyRolloff) >> 1);
        if (fht_log_out[i] < cut)
        {
            fht_log_out[i] = 0;
        }
        else
        {
            fht_log_out[i] -= cut;
        }
    }

    // result in fht_log_out[], length FHT_N/2, 16bit
    uint32_t r = 0;
    uint32_t g = 0;
    uint32_t b = 0;
    for (uint8_t i = 0; i < (FHT_N >> 1); i++)
    {
        if (i < (FHT_N >> 6))
        {
            b += fht_log_out[i];
        }
        else if (i < ((FHT_N >> 2) + (FHT_N >> 3) - (FHT_N >> 4)))
        {
            g += fht_log_out[i];
        }
        else
        {
            r += fht_log_out[i];
        }
    }

    b /= FHT_N >> 6;
    g /= (FHT_N >> 2) + (FHT_N >> 3) - (FHT_N >> 4);
    r /= (FHT_N >> 3);

    b <<= 3;
    r <<= 3;
    g <<= 3;


    if (b > 255)
    {
        b = 255;
    }

    if (r > 255)
    {
        r = 255;
    }

    if (g > 255)
    {
        g = 255;
    }

    offset++;
    if (b > 80)
    {
        offset = 0;
    }

    bool trigger = false;
    if (offset < 4)
    {
        trigger = true;
    }
    else if (offset > 7)
    {
        offset = 4;
        trigger = true;
    }


    if (trigger)
    {
        for (uint8_t i = NEOPIXEL_COUNT - 1; i > 0; i--)
        {
            strip.setPixelColor(i, strip.getPixelColor(i - 1));
        }
    }

    strip.setPixelColor(0, strip.Color(r, g, b));
    strip.show();
}
#endif

void readCommand(Adafruit_CC3000_ClientRef client, char* buffer)
{
    uint8_t offset = 0;
    char current = '\0';

    while (true)
    {
        if (client.available())
        {
            current = client.read();
            if (offset < 4)
            {
                // skip the: 'GET ' part
            }
            else if (current == '/')
            {
                // skip the: '/', if any
            }
            else if (current == '\n' || current == ' ' || offset > 29)
            {
                buffer[offset - 5] = '\0';
                break;
            }
            else
            {
                buffer[offset - 5] = current;
            }

            offset++;
        }
        else
        {
            // wait for more data from that specific client.
            delay(1);
        }
    }
}

void processCommand(Adafruit_CC3000_ClientRef client, char* buffer)
{
#ifndef NO_EFFECTS
    SERIAL_PRINTLN((const char*) buffer);
    if (strcmp(buffer, "rb") == 0)
    {
        PRINT_CONTENT(F("Rainbow"));
        FIX_BRIGHTNESS();
        state = STATE_RAINBOW;
        offset = 0;
        direction = 1;
    }
    else if (strcmp(buffer, "fd") == 0)
    {
        PRINT_CONTENT(F("Fade"));
        FIX_BRIGHTNESS();
        state = STATE_COLOR_FADE;
        offset = 0;
        direction = 1;
    }
    else if (strcmp(buffer, "st") == 0)
    {
        PRINT_CONTENT(F("Stop"));
        FIX_BRIGHTNESS();
        state = STATE_NONE;
    }
    else if (strcmp(buffer, "rn") == 0)
    {
        PRINT_CONTENT(F("Run"));
        FIX_BRIGHTNESS();
        state = STATE_RUN;
        direction = 1;
        offset = 0;
    }
    else if (strcmp(buffer, "cy") == 0)
    {
        PRINT_CONTENT(F("Cylon"));
        FIX_BRIGHTNESS();
        state = STATE_CYLON;
        direction = 1;
        offset = 0;
    }
    else if (strcmp(buffer, "fw") == 0)
    {
        PRINT_CONTENT(F("Fireworks"));
        FIX_BRIGHTNESS();
        state = STATE_FIREWORKS;
        direction = 1;
        offset = 0;
    }
#ifndef NO_CANDLE
    else if (strcmp(buffer, "cn") == 0)
    {
        PRINT_CONTENT(F("Candle"));
        FIX_BRIGHTNESS();
        color = 0xBF3F00;
        variance = 0x402800;
        state = STATE_FLICKER;
    }
#endif
#ifndef NO_SPECTRUM
    else if (strcmp(buffer, "sp") == 0)
    {
        PRINT_CONTENT(F("Spectrum"));
        state = STATE_SPECTRUM;
        strip.setBrightness(255);
    }
#endif
    else
#endif
    if (strlen(buffer) > 1)
    {
        uint32_t value = atol(buffer + 1);
        if (buffer[0] == 'c')
        {
            PRINT_CONTENT(F("Color: "));
            PRINT_CONTENT((const char*)(buffer + 1));
            state = STATE_COLOR_CHANGE;
            color = value;
            offset = 0;
            direction = 1;
        }
#ifndef NO_EFFECTS
        else if (buffer[0] == 'f')
        {
            PRINT_CONTENT(F("Flicker: "));
            PRINT_CONTENT((const char*)(buffer + 1));
            FIX_BRIGHTNESS();
            state = STATE_FLICKER;
            variance = value;
        }
#ifndef NO_SPECTRUM
        else if (buffer[0] == 's')
        {
            PRINT_CONTENT(F("Sensitivity: "));
            PRINT_CONTENT((const char*)(buffer + 1));
            variance = value;
        }
        else if (buffer[0] == 'n')
        {
            PRINT_CONTENT(F("Noise cutoff: "));
            PRINT_CONTENT((const char*)(buffer + 1));
            variance = value;
        }
        else if (buffer[0] == 'r')
        {
            PRINT_CONTENT(F("Rolloff: "));
            PRINT_CONTENT((const char*)(buffer + 1));
            frequencyRolloff = value;
        }
#endif
#endif
        else if (buffer[0] == 'b')
        {
            PRINT_CONTENT(F("Brightness: "));
            PRINT_CONTENT((const char*)(buffer + 1));
            setBrightness(value);
        }
    }
}

void loop(void)
{
    // Try to get a client which is connected.
    Adafruit_CC3000_ClientRef client = webServer.available();
    if (client && client.available() > 0)
    {
        SERIAL_PRINTLN("Req");

        char buffer[12];
        readCommand(client, buffer);

        client.fastrprint((const __FlashStringHelper*)website_http_data);
        PRINT_CONTENT((const __FlashStringHelper*)website_doctype_data);
        client.fastrprint((const __FlashStringHelper*)website_header_data);

#ifndef WEBSITE_NONE
        client.fastrprint((const __FlashStringHelper*)website_options_data);
        client.print(brightness, DEC);
        client.fastrprint((const __FlashStringHelper*)website_color_data);
        client.print(color, HEX);
#endif

        if ((buffer[0] != '\0') && (buffer[0] != ' '))
        {
            PRINT_CONTENT((const __FlashStringHelper*)website_command_data);
            processCommand(client, buffer);
        }

        PRINT_CONTENT((const __FlashStringHelper*)website_footer_data);

#ifdef WEBSITE_HEADER_SEND_LENGTH
        for (uint8_t remaining_length = WEBSITE_VARIABLE_LENGTH - strlen(buffer) - 1; remaining_length > 0; remaining_length--)
        {
            client.write('\0');
        }
#endif

        delay(5);
        client.close();
    }
    else if (state != STATE_SPECTRUM)
    {
#ifndef NO_EFFECTS
        random(0, 1);
#endif
        delay(5);
    }

    switch (state)
    {
        case STATE_COLOR_CHANGE:
            colorWipe();
            break;
#ifndef NO_EFFECTS
        case STATE_COLOR_FADE:
            fadeColors();
            break;
        case STATE_RAINBOW:
            rainbow();
            break;
        case STATE_RUN:
            run();
            break;
        case STATE_CYLON:
            cylon();
            break;
        case STATE_FIREWORKS:
            fireworks();
            break;
        case STATE_FLICKER:
            flicker();
            break;
#ifndef NO_SPECTRUM
        case STATE_SPECTRUM:
            spectrum();
            break;
#endif
#endif
        default:
            break;
    }
}
