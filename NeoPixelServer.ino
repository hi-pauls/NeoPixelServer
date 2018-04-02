#include <avr/wdt.h>
#include <SPI.h>
#include "Adafruit_NeoPixel.h"
#include "NeoPixelServerDefs.h"
#include "NeoPixelServerWebsite.h"
#include "Adafruit_CC3000.h"
#include "Adafruit_CC3000_Server.h"

#ifndef NO_SPECTRUM
    #include "FHT.h"
uint8_t pixels[NEOPIXEL_COUNT * 3] __attribute__ ((section (".noinit")));
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800, pixels);
Adafruit_CC3000 cc3000 = Adafruit_CC3000(CC3000_CS, CC3000_IRQ, CC3000_VBAT, SPI_CLOCK_DIV4);
Adafruit_CC3000_Server webServer(WEBSERVER_PORT);

#endif

struct DebugState
{
    int32_t rebootCount;
    int32_t disconnectCount;
    char debugStage;
    char prevDebugStage;
};

NO_INIT_ON_RESET DebugState debugState;

#define SETTINGS_PROPERTY(type, name, accessor)                                                   \
    public:                                                                                       \
        type get ## accessor() const { return name; }                                             \
        NO_INLINE void set ## accessor(type name) { this->name = name; this->updateChecksum(); }  \
    private:                                                                                      \
        type name

class Settings
{
public:
    char magic[SETTINGS_MAGIC_LENGTH + 1];

    NOINLINE bool boot()
    {
        // Check magic start and checksum
        if (magicMatch) DEBUG_PRINTLN('I');

        bool csMatch = checksum == calcChecksum();
        if (csMatch) DEBUG_PRINTLN('J');

        bool magicMatch = strcmp(magic, SETTINGS_MAGIC) == 0;
        bool versionMatch = VERSION == prevVersion;
        if (versionMatch) DEBUG_PRINTLN('K');

        if (initializedOnBoot = !(magicMatch && csMatch && versionMatch))
        {
            // Different guard begin, not initialized
            memcpy(magic, SETTINGS_MAGIC, SETTINGS_MAGIC_LENGTH);
            magic[SETTINGS_MAGIC_LENGTH] = 0;
            debugState.rebootCount = 0;
            debugState.disconnectCount = 0;

            initialized = false;
            lastChange = 0;
            state = STATE_NONE;

            brightness = 0;
            color = 0xFF0000;
            offset = 0;
            variance = 0;
            direction = 1;

    #ifndef NO_SPECTRUM
            cutoff = 100;
            sensitivity = 10;
            frequencyRolloff = 4;
    #endif
            debugState.prevDebugStage = '0';
        }
        else
        {
            debugState.rebootCount++;
        }

        prevVersion = VERSION;
        updateChecksum();
        return initializedOnBoot;
    }

    NOINLINE int16_t updateOffset()
    {
        offset += direction;
        updateChecksum();
        return offset;
    }

    NOINLINE void updateOffsetWrap(int16_t wrap)
    {
        offset = (offset + direction) % wrap;
        updateChecksum();
    }

    NOINLINE void invertDirection()
    {
        direction = -direction;
        updateChecksum();
    }

    NOINLINE bool checkTime(uint32_t timeout)
    {
        uint32_t current = millis();
        if ((current - lastChange) < timeout)
        {
            // Wait another receive cycle.
            return false;
        }

        setLastChange(current);
        return true;
    }

    NOINLINE void initState(uint8_t state)
    {
        this->state = state;
        offset = 0;
        direction = 1;
        updateChecksum();
    }

    bool initializedOnBoot;
    char prevVersion;

private:
    NOINLINE uint8_t calcChecksum()
    {
        uint8_t checksum = 0;
        uint8_t size = sizeof(Settings) - 4;
        uint8_t* dataPtr = (uint8_t*)(this);
        for (uint8_t i = 0; i < size; ++i, ++dataPtr)
        {
            checksum ^= *dataPtr;
        }

        return checksum;
    }

    void updateChecksum() { checksum = calcChecksum(); }

    // Settings
    SETTINGS_PROPERTY(uint8_t, brightness, Brightness);
    SETTINGS_PROPERTY(int32_t, color, Color);
    SETTINGS_PROPERTY(int32_t, variance, Variance);
    SETTINGS_PROPERTY(int8_t, direction, Direction);

#ifndef NO_SPECTRUM
    SETTINGS_PROPERTY(uint8_t, cutoff, Cutoff);
    SETTINGS_PROPERTY(uint8_t, sensitivity, Sensitivity);
    SETTINGS_PROPERTY(uint8_t, frequencyRolloff, FrequencyRolloff);
#endif

    // State
    SETTINGS_PROPERTY(bool, initialized, Initialized);
    SETTINGS_PROPERTY(uint8_t, state, State);
    SETTINGS_PROPERTY(int16_t, offset, Offset);
    SETTINGS_PROPERTY(uint32_t, lastChange, LastChange);

    uint8_t checksum;
};

// Runtime data
NO_INIT_ON_RESET Settings settings;
NO_INIT_ON_RESET uint8_t pixels[NEOPIXEL_COUNT * 3];

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800, pixels);
Adafruit_CC3000 cc3000 = Adafruit_CC3000(CC3000_PIN_CS, CC3000_PIN_IRQ, CC3000_PIN_VBAT, SPI_CLOCK_DIV4);
Adafruit_CC3000_Server webServer = Adafruit_CC3000_Server(WEBSERVER_PORT);

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

void setupErrorReboot(char error)
{
    DEBUG_PRINTLN(error);
    if (!settings.getInitialized())
    {
        settings.setColor(0xFF0000);
        settings.setOffset(0);
        setBrightness(0xFF);

        // Reset the watchdog timer a final time before going into the endless loop
        wdt_reset();

        while (true)
        {
            // Don't reset the watchdog timer, just reboot after 8s
            ERROR();
            delay(20);
        }
    }
    else
    {
        delay(1000);
    }
}

void setupErrorFrame()
{
    if (!settings.getInitialized())
    {
        ERROR();
    }

    wdt_reset();
    delay(20);
}

void setupStep()
{
    wdt_reset();
    delay(1000);
    wdt_reset();
}

bool displayConnectionDetails()
{
    uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;

    if(! cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
    {
        return false;
    }

#if !defined(NO_SERIAL) && !defined(CC3000_TINY_DRIVER)
    cc3000.printIPdotsRev(ipAddress);
    Serial.println();
#endif

    return true;
}

void setupServer()
{
    wdt_reset();

    DEBUG_PRINTLN('T');
    if (! cc3000.begin())
    {
        setupErrorReboot('G'); // Init
        return;
    }

    setupStep();
    DEBUG_PRINTLN('U');
    if (! cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY))
    {
        setupErrorReboot('H'); // Associate
        return;
    }

    // Block until DHCP address data is available, or forever, if it isn't.
    DEBUG_PRINTLN('V');
    if (!settings.getInitialized())
    {
        settings.setColor(0xFF);
        settings.setOffset(0);
        setBrightness(NEOPIXEL_BRIGHTNESS);
    }

    setupStep();
    DEBUG_PRINTLN('W');
    while (! cc3000.checkDHCP())
    {
        setupErrorFrame();
    }

    // Display the IP address DNS, Gateway, etc.
    DEBUG_PRINTLN('X');
    while (! displayConnectionDetails())
    {
        setupErrorFrame();
    }

    // Start listening for connections
    setupStep();
    DEBUG_PRINTLN('Y');
    webServer.begin();
    settings.setInitialized(true);
}

void setup()
{
    debugState.prevDebugStage = debugState.debugStage;
    debugState.debugStage = '0';

    // Enable the watchdog timer
    wdt_reset();
    wdt_enable(WDTO_8S);
    delay(4000);
    wdt_reset();

#ifndef NO_SERIAL
    Serial.begin(9600);
#ifdef SERIAL_WAIT
    while (! Serial)
    {
        wdt_reset();
        delay(20);
    }
#endif
#endif
    SERIAL_PRINTLN(VERSION);
    SERIAL_PRINTLN(debugState.prevDebugStage);

    settings.boot();

    // Initialize the strip.
    strip.begin();

    if (settings.initializedOnBoot)
    {
        memset(pixels, 0, NEOPIXEL_COUNT * 3);

#ifndef START_ENABLED
        setBrightness(NEOPIXEL_BRIGHTNESS);
        strip.show();
#else
#ifdef __AVR_ATmega32U4__
        // Make sure to reduce brightness, in case a large strip
        // is being powered through USB. Only works on ATmega32u4
        int8_t brightness = (UDADDR & _BV(ADDEN)) ? NEOPIXEL_USB_BRIGHTNESS : (NEOPIXEL_BRIGHTNESS - 0x40);
#else
        // Always use USB brightness, just to be on the safe side
        int8_t brightness = NEOPIXEL_USB_BRIGHTNESS;
#endif

        settings.setColor(0xFFCC66);
        setBrightness(brightness);

        for (int i = 0; i < NEOPIXEL_COUNT; i++)
        {
            wdt_reset();
            colorWipe();
            delay(NEOPIXEL_COLOR_DELAY);
        }
#endif
    }
    else
    {
        strip.initBrightness(settings.getBrightness());
    }

#ifndef NO_SPECTRUM
    // Initialize the microphone
    DEBUG_PRINTLN('L');
    setupMicrophone();
#endif

    // Reset the watchdog timer
    DEBUG_PRINTLN('M');
    setupServer();

    if (settings.initializedOnBoot)
    {
        // Initialize the strip.
        DEBUG_PRINTLN('N');

#ifndef START_ENABLED
        settings.setColor(0xFF00);
        settings.initState(STATE_RUN);
#else
        settings.setColor(0xFFCC66);
        settings.initState(STATE_COLOR_CHANGE);
#endif
    }
    else
    {
        DEBUG_PRINTLN('O');
    }

    DEBUG_PRINTLN('P');
}

void setBrightness(uint8_t bright)
{
    if (bright > NEOPIXEL_MAX_BRIGHTNESS)
    {
        bright = NEOPIXEL_MAX_BRIGHTNESS;
    }

    strip.initBrightness(bright);
    settings.setBrightness(bright);

    if (settings.getState() == STATE_NONE)
    {
        settings.setState(STATE_COLOR_CHANGE);
        settings.setOffset(0);
    }
}

// Fill the dots one after the other with a color
void colorWipe()
{
    if (! settings.checkTime(NEOPIXEL_COLOR_DELAY))
    {
        return;
    }

    strip.setPixelColor(settings.getOffset(), settings.getColor());
    strip.show();

    if (settings.updateOffset() > NEOPIXEL_COUNT)
    {
        // Done!
        settings.setState(STATE_NONE);
    }
}

#ifndef NO_RAINBOW
void rainbow()
{
    if (! settings.checkTime(NEOPIXEL_RAINBOW_DELAY))
    {
        return;
    }

    int16_t offset = settings.getOffset();
    for(uint8_t i = 0; i < NEOPIXEL_COUNT; i++)
    {
        strip.setPixelColor(i, Wheel((i + offset) & 255));
    }

    strip.show();
    settings.updateOffsetWrap(256);
}
#endif

#ifndef NO_FADE
// Slightly different, this makes the rainbow equally distributed throughout
void fadeColors()
{
    if (! settings.checkTime(NEOPIXEL_RAINBOW_DELAY))
    {
        return;
    }

    int16_t offset = settings.getOffset();
    for(uint8_t i = 0; i < NEOPIXEL_COUNT; i++)
    {
        strip.setPixelColor(i, Wheel(((i * 256 / NEOPIXEL_COUNT) + offset) & 255));
    }

    strip.show();
    settings.updateOffsetWrap(256);
}
#endif

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos)
{
    if(WheelPos < 85)
    {
        WheelPos *= 3;
        return strip.Color(WheelPos, 255 - WheelPos, 0);
    }
    else if(WheelPos < 170)
    {
        WheelPos -= 85;
        WheelPos *= 3;
        return strip.Color(255 - WheelPos, 0, WheelPos);
    }
    else
    {
        WheelPos -= 170;
        WheelPos *= 3;
        return strip.Color(0, WheelPos, 255 - WheelPos);
    }
}

void run()
{
    if (! settings.checkTime(NEOPIXEL_RAINBOW_DELAY))
    {
        return;
    }

    int32_t color = settings.getColor();
    int16_t offset = settings.getOffset();
    int8_t direction = settings.getDirection();

    strip.setPixelColor(offset, color);
    strip.setPixelColor(offset - direction, (color >> 1) & 0x7F7F7F);
    strip.setPixelColor(offset - (direction * 2), (color >> 2) & 0x3F3F3F);
    strip.setPixelColor(offset - (direction * 3), (color >> 3) & 0x1F1F1F);
    strip.setPixelColor(offset - (direction * 4), 0);
    strip.show();

    offset = settings.updateOffset();
    if ((offset < -4) || (offset > NEOPIXEL_COUNT + 4))
    {
        settings.setState(STATE_NONE);
    }
}

#ifndef NO_CYLON
void cylon()
{
    run();

    int16_t offset = settings.getOffset();
    if (offset < 0 || offset > NEOPIXEL_COUNT)
    {
        settings.invertDirection();
        settings.updateOffset();
    }
}
#endif

#ifndef NO_FIREWORKS
void fireworks()
{
    int32_t color = settings.getColor();
    if (color < 1 && random(0, 300) < 1)
    {
        color = strip.Color(random(0, 255), random(0, 255), random(0, 255));
        settings.setColor(color);
    }

    if (color > 0)
    {
        run();
    }

    if (settings.getState() == STATE_NONE)
    {
        settings.initState(STATE_FIREWORKS);
        settings.setColor(0);
    }
}
#endif

#ifndef NO_FLICKER
void flicker()
{
    if (! settings.checkTime(NEOPIXEL_FLICKER_DELAY))
    {
        return;
    }

    int32_t color = settings.getColor();
    int32_t variance = settings.getVariance();

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
#endif

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
        sample = sample * settings.getSensitivity() / 10;

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
        cut = settings.getCutoff() + (32 >> i) - ((i * settings.getFrequencyRolloff()) >> 1);
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

    int16_t offset = settings.updateOffset();
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
        settings.setOffset(4);
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
        wdt_reset();
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

int32_t hexStrToInt(char* str)
{
    int32_t result = 0;
    for (; *str != 0; ++str, result <<= 4)
    {
        char b = *str;
        if (b >= '0' && b <= '9')
            result += b - '0';
        else if (b >= 'a' && b <= 'f')
            result += b - 'a' + 10;
        else if (b >= 'A' && b <= 'F')
            result += b - 'A' + 10;
        else
            break;
    }

    return result;
}

void processCommand(Adafruit_CC3000_ClientRef client, char* buffer)
{
    wdt_reset();

#ifndef NO_EFFECTS
    SERIAL_PRINTLN((const char*) buffer);
    if (strcmp(buffer, "st") == 0)
    {
        PRINT_CONTENT(F("Stop"));
        FIX_BRIGHTNESS();
        settings.setState(STATE_NONE);
    }
#ifndef NO_RAINBOW
    else if (strcmp(buffer, "rb") == 0)
    {
        PRINT_CONTENT(F("Rainbow"));
        FIX_BRIGHTNESS();
        settings.initState(STATE_RAINBOW);
    }
#endif
#ifndef NO_FADE
    else if (strcmp(buffer, "fd") == 0)
    {
        PRINT_CONTENT(F("Fade"));
        FIX_BRIGHTNESS();
        settings.initState(STATE_COLOR_FADE);
    }
#endif
#ifndef NO_RUN
    else if (strcmp(buffer, "rn") == 0)
    {
        PRINT_CONTENT(F("Run"));
        FIX_BRIGHTNESS();
        settings.initState(STATE_RUN);
    }
#endif
#ifndef NO_CYLON
    else if (strcmp(buffer, "cy") == 0)
    {
        PRINT_CONTENT(F("Cylon"));
        FIX_BRIGHTNESS();
        settings.initState(STATE_CYLON);
    }
#endif
#ifndef NO_FIREWORKS
    else if (strcmp(buffer, "fw") == 0)
    {
        PRINT_CONTENT(F("Fireworks"));
        FIX_BRIGHTNESS();
        settings.initState(STATE_FIREWORKS);
    }
#endif
#ifndef NO_CANDLE
    else if (strcmp(buffer, "cn") == 0)
    {
        PRINT_CONTENT(F("Candle"));
        FIX_BRIGHTNESS();
        settings.setState(STATE_FLICKER);
        settings.setColor(0xBF3F00);
        settings.setVariance(0x402800);
    }
#endif
#ifndef NO_SPECTRUM
    else if (strcmp(buffer, "sp") == 0)
    {
        PRINT_CONTENT(F("Spectrum"));
        strip.setBrightness(255);
        settings.initState(STATE_SPECTRUM);
    }
#endif
    else
#endif
    if (strlen(buffer) > 1)
    {
        if (buffer[0] == 'c')
        {
            PRINT_CONTENT(F("Color: "));
            settings.initState(STATE_COLOR_CHANGE);
            settings.setColor(hexStrToInt(++buffer));
        }
#ifndef NO_FLICKER
        else if (buffer[0] == 'f')
        {
            PRINT_CONTENT(F("F: "));
            FIX_BRIGHTNESS();
            settings.setState(STATE_FLICKER);
            settings.setVariance(hexStrToInt(++buffer));
        }
#endif
#ifndef NO_SPECTRUM
        else if (buffer[0] == 's')
        {
            PRINT_CONTENT(F("S: "));
            settings.setSensitivity(atoi(++buffer));
            client.print(settings.getSensitivity(), DEC);
        }
        else if (buffer[0] == 'n')
        {
            PRINT_CONTENT(F("C: "));
            settings.setCutoff(atoi(++buffer));
            client.print(settings.getCutoff(), DEC);
        }
        else if (buffer[0] == 'r')
        {
            PRINT_CONTENT(F("R: "));
            settings.setFrequencyRolloff(atoi(++buffer));
            client.print(settings.getFrequencyRolloff(), DEC);
         }
#endif
        else if (buffer[0] == 'b')
        {
            PRINT_CONTENT(F("Brightness: "));
            setBrightness(atoi(++buffer));
            client.print(settings.getBrightness(), DEC);
        }
    }
}

void checkStatus()
{
    wdt_reset();
    if (cc3000.getStatus() == STATUS_DISCONNECTED)
    {
        DEBUG_PRINTLN('S');
        debugState.disconnectCount++;
        cc3000.stop();

        for (int i = 5; i > 0; --i)
        {
            wdt_reset();
            delay(1000);
        }

        setupServer();
    }
}

void loop()
{
    // Reset the watchdog timer
    checkStatus();

    uint8_t state = settings.getState();

    // Try to get a client which is connected.
    wdt_reset();
    Adafruit_CC3000_ClientRef client = webServer.available();
    if (client && client.available() > 0)
    {
        SERIAL_PRINTLN("R");

        char buffer[12];
        readCommand(client, buffer);

        wdt_reset();
        client.fastrprint((const __FlashStringHelper*)website_http_data);
        PRINT_CONTENT((const __FlashStringHelper*)website_doctype_data);
        client.fastrprint((const __FlashStringHelper*)website_header_data);

#if WEBSITE_TYPE != WEBSITE_NONE
        wdt_reset();
        client.fastrprint((const __FlashStringHelper*)website_options_data);
        client.print(settings.getBrightness(), DEC);
        client.fastrprint((const __FlashStringHelper*)website_color_data);
        client.print(settings.getColor(), HEX);
        wdt_reset();
        client.fastrprint((const __FlashStringHelper*)website_version_data);
        client.print(VERSION);
        client.fastrprint((const __FlashStringHelper*)website_reboot_data);
        client.print((int32_t)debugState.rebootCount, DEC);
        wdt_reset();
        client.fastrprint((const __FlashStringHelper*)website_disconnect_data);
        client.print((int32_t)debugState.disconnectCount, DEC);
        client.fastrprint((const __FlashStringHelper*)website_prevState_data);
        client.print(debugState.prevDebugStage);
#endif

        if ((buffer[0] != '\0') && (buffer[0] != ' '))
        {
            PRINT_CONTENT((const __FlashStringHelper*)website_command_data);
            processCommand(client, buffer);
        }

        wdt_reset();
        PRINT_CONTENT((const __FlashStringHelper*)website_footer_data);

#ifdef WEBSITE_HEADER_SEND_LENGTH
        for (uint8_t remaining_length = WEBSITE_VARIABLE_LENGTH - strlen(buffer) - 2; remaining_length > 0; remaining_length--)
        {
            client.write('\0');
        }
#endif

        client.write('\0');
        delay(5);
        wdt_reset();
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
#ifndef NO_FADE
        case STATE_COLOR_FADE:
            fadeColors();
            break;
#endif
#ifndef NO_RAINBOW
        case STATE_RAINBOW:
            rainbow();
            break;
#endif
#ifndef NO_RUN
        case STATE_RUN:
            run();
            break;
#endif
#ifndef NO_CYLON
        case STATE_CYLON:
            cylon();
            break;
#endif
#ifndef NO_FIREWORKS
        case STATE_FIREWORKS:
            fireworks();
            break;
#endif
#ifndef NO_FLICKER
        case STATE_FLICKER:
            flicker();
            break;
#endif
#ifndef NO_SPECTRUM
        case STATE_SPECTRUM:
            spectrum();
            break;
#endif
        default:
            break;
    }
}
