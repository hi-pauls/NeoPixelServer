#include <avr/wdt.h>
#include <SPI.h>
#include "Adafruit_NeoPixel.h"
#include "NeoPixelServerDefs.h"
#include "NeoPixelServerWebsite.h"
#include "Adafruit_CC3000.h"
#include "Adafruit_CC3000_Server.h"

#ifndef NO_SPECTRUM
    #include "FHT.h"
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

    bool boot()
    {
        // Check magic start and checksum
        bool magicMatch = strcmp(magic, SETTINGS_MAGIC) == 0;
        bool versionMatch = VERSION == prevVersion;
        bool csMatch = checksum == calcChecksum();

        if (initializedOnBoot = !(magicMatch && csMatch && versionMatch))
        {
            DEBUG_PRINTLN('I');

            // Different guard begin, not initialized
            memcpy(magic, SETTINGS_MAGIC, SETTINGS_MAGIC_LENGTH);
            magic[SETTINGS_MAGIC_LENGTH] = 0;
            debugState.rebootCount = 0;
            debugState.disconnectCount = 0;

            initialized = false;
            state = STATE_NONE;

            brightness = 0;
            color = 0xFF0000;
            variance = 0;
            direction = 1;
            baseOffset = NEOPIXEL_OFFSET;
            pixelCount = NEOPIXEL_COUNT;
            offset = baseOffset;

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

        lastChange = 0;
        prevVersion = VERSION;
        updateChecksum();
        return initializedOnBoot;
    }

    NO_INLINE void resetOffset()
    {
        setOffset(baseOffset);
    }

    int16_t updateOffset()
    {
        setOffset(offset + direction);
        return offset;
    }

    NO_INLINE void updateOffsetWrap(int16_t wrap)
    {
        offset = baseOffset + (offset + direction - baseOffset) % wrap;
        updateChecksum();
    }

    void invertDirection()
    {
        direction = -direction;
        updateChecksum();
    }

    NO_INLINE bool checkTime(uint32_t timeout)
    {
        uint32_t current = millis();
        if ((current - lastChange) < timeout)
        {
            // Wait another receive cycle.
            return false;
        }

        lastChange = current;
        return true;
    }

    void initState(uint8_t state)
    {
        this->state = state;
        offset = baseOffset;
        direction = 1;
        updateChecksum();
    }

    bool initializedOnBoot;
    char prevVersion;

private:
    uint8_t calcChecksum()
    {
        uint8_t checksum = 0;
        size_t size = sizeof(Settings) - sizeof(lastChange) - sizeof(checksum);
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
    SETTINGS_PROPERTY(int16_t, baseOffset, BaseOffset);
    SETTINGS_PROPERTY(int16_t, pixelCount, PixelCount);

    uint32_t lastChange;
    uint8_t checksum;
};

// Runtime data
NO_INIT_ON_RESET Settings settings;
NO_INIT_ON_RESET uint8_t pixels[NEOPIXEL_COUNT * 3];

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800, pixels);
Adafruit_CC3000 cc3000 = Adafruit_CC3000(CC3000_PIN_CS, CC3000_PIN_IRQ, CC3000_PIN_VBAT, SPI_CLOCK_DIV4);
Adafruit_CC3000_Server webServer = Adafruit_CC3000_Server(WEBSERVER_PORT);
Adafruit_CC3000_ClientRef* client = NULL;

// Binary size optimization functions
int16_t _settings_updateOffset()
{
    return settings.updateOffset();
}

void _settings_resetOffset()
{
    settings.resetOffset();
}

void _settings_setState(uint8_t state)
{
    settings.setState(state);
}

void _settings_initState(uint8_t state)
{
    settings.initState(state);
}

NO_INLINE uint32_t _settings_getColor()
{
    return settings.getColor();
}

void _settings_setColor(uint32_t color)
{
    settings.setColor(color);
}

void _strip_show()
{
    strip.show();
}

void _client_print(char chr)
{
    client->print(chr);
}

void _client_print(int32_t value, uint8_t type)
{
    client->print(value, type);
}

void _client_fastrprint(const __FlashStringHelper* str)
{
    client->fastrprint(str);
}

#define _sopt_getOffset()                     settings.getOffset()
#define _sopt_setOffset(offset)               settings.setOffset(offset)
#define _sopt_updateOffset()                  _settings_updateOffset()
#define _sopt_resetOffset()                   _settings_resetOffset()
#define _sopt_getBaseOffset()                 settings.getBaseOffset()
#define _sopt_getPixelCount()                 settings.getPixelCount()
#define _sopt_getState()                      settings.getState()
#define _sopt_setState(state)                 _settings_setState(state)
#define _sopt_initState(state)                _settings_initState(state)
#define _sopt_getBrightness()                 settings.getBrightness()
#define _sopt_getColor()                      _settings_getColor()
#define _sopt_setColor(color)                 _settings_setColor(color)
#define _sopt_show()                          _strip_show()
#define _sopt_getInitialized()                settings.getInitialized()
#define _sopt_checkTime(timeout)              settings.checkTime(timeout)
#define _sopt_clientPrint(chr)                _client_print(chr)
#define _sopt_clientPrintValue(value, type)   _client_print(value, type)
#define _sopt_clientPrintString(fstr)         _client_fastrprint(fstr)

void setPixelColor(int16_t index, uint32_t color)
{
    if (_sopt_getBaseOffset() <= index && index < _sopt_getPixelCount())
    {
        strip.setPixelColor(index, color);
    }
}

void setStripColor(int16_t start, int16_t end, uint32_t color)
{
    for (int16_t i = end - 1; i >= start; --i)
    {
        strip.setPixelColor(i, color);
    }
}

#ifndef NO_SPECTRUM
void setupMicrophone()
{
    // Get the analog input pin for the microphone
    uint8_t pin = SPECTRUM_IN_PIN;
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
    if (!_sopt_getInitialized())
    {
        _sopt_setColor(0xFF0000);
        _sopt_resetOffset();
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
    if (!_sopt_getInitialized())
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
    if (!_sopt_getInitialized())
    {
        _sopt_setColor(0xFF);
        _sopt_resetOffset();
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
        setStripColor(0, NEOPIXEL_COUNT, 0);

#ifndef START_ENABLED
        setBrightness(NEOPIXEL_BRIGHTNESS);
        _sopt_show();
#else
#ifdef __AVR_ATmega32U4__
        // Make sure to reduce brightness, in case a large strip
        // is being powered through USB. Only works on ATmega32u4
        int8_t brightness = (UDADDR & _BV(ADDEN)) ? NEOPIXEL_USB_BRIGHTNESS : (NEOPIXEL_BRIGHTNESS - 0x40);
#else
        // Always use USB brightness, just to be on the safe side
        int8_t brightness = NEOPIXEL_USB_BRIGHTNESS;
#endif

        _sopt_setColor(0xFFCC66);
        setBrightness(brightness);

        for (int i = _sopt_getBaseOffset(); i < _sopt_getPixelCount(); i++)
        {
            wdt_reset();
            colorWipe();
            delay(NEOPIXEL_COLOR_DELAY);
        }
#endif
    }
    else
    {
        strip.initBrightness(_sopt_getBrightness());
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
        _sopt_setColor(0xFF00);
        _sopt_initState(STATE_RUN);
#else
        _sopt_setColor(0xFFCC66);
        _sopt_initState(STATE_COLOR_CHANGE);
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

    if (_sopt_getState() == STATE_NONE)
    {
        _sopt_setState(STATE_COLOR_CHANGE);
        _sopt_resetOffset();
    }
}

// Fill the dots one after the other with a color
void colorWipe()
{
    if (! _sopt_checkTime(NEOPIXEL_COLOR_DELAY))
    {
        return;
    }

    setPixelColor(_sopt_getOffset(), _sopt_getColor());
    _sopt_show();

    if (_sopt_updateOffset() > _sopt_getPixelCount())
    {
        // Done!
        _sopt_setState(STATE_NONE);
    }
}

#ifndef NO_RAINBOW
void rainbow()
{
    if (! _sopt_checkTime(NEOPIXEL_RAINBOW_DELAY))
    {
        return;
    }

    int16_t offset = _sopt_getOffset();
    for(uint8_t i = _sopt_getBaseOffset(); i < _sopt_getPixelCount(); i++)
    {
        setPixelColor(i, Wheel((i + offset) & 255));
    }

    _sopt_show();
    settings.updateOffsetWrap(256);
}
#endif

#ifndef NO_FADE
// Slightly different, this makes the rainbow equally distributed throughout
void fadeColors()
{
    if (! _sopt_checkTime(NEOPIXEL_RAINBOW_DELAY))
    {
        return;
    }

    int16_t offset = _sopt_getOffset();
    for(uint8_t i = _sopt_getBaseOffset(); i < _sopt_getPixelCount(); i++)
    {
        setPixelColor(i, Wheel(((i * 256 / _sopt_getPixelCount()) + offset) & 255));
    }

    _sopt_show();
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
    if (! _sopt_checkTime(NEOPIXEL_RAINBOW_DELAY))
    {
        return;
    }

    int32_t color = _sopt_getColor();
    int16_t offset = _sopt_getOffset();
    int8_t direction = settings.getDirection();

    setPixelColor(offset, color);
    setPixelColor(offset - direction, (color >> 1) & 0x7F7F7F);
    setPixelColor(offset - (direction * 2), (color >> 2) & 0x3F3F3F);
    setPixelColor(offset - (direction * 3), (color >> 3) & 0x1F1F1F);
    setPixelColor(offset - (direction * 4), 0);
    _sopt_show();

    offset = _sopt_updateOffset();
    if ((offset < _sopt_getBaseOffset() - 4) || (offset > _sopt_getPixelCount() + 4))
    {
        _sopt_setState(STATE_NONE);
    }
}

#ifndef NO_CYLON
void cylon()
{
    run();

    int16_t offset = _sopt_getOffset();
    if (offset < _sopt_getBaseOffset() || offset > _sopt_getPixelCount())
    {
        settings.invertDirection();
        _sopt_updateOffset();
    }
}
#endif

#ifndef NO_FIREWORKS
void fireworks()
{
    int32_t color = _sopt_getColor();
    if (color < 1 && random(0, 300) < 1)
    {
        color = strip.Color(random(0, 255), random(0, 255), random(0, 255));
        _sopt_setColor(color);
    }

    if (color > 0)
    {
        run();
    }

    if (_sopt_getState() == STATE_NONE)
    {
        _sopt_initState(STATE_FIREWORKS);
        _sopt_setColor(0);
    }
}
#endif

#ifndef NO_FLICKER
void flicker()
{
    if (! _sopt_checkTime(NEOPIXEL_FLICKER_DELAY))
    {
        return;
    }

    int32_t color = _sopt_getColor();
    int32_t variance = settings.getVariance();

    uint8_t varr = (variance >> 16) & 0xFF;
    uint8_t varg = (variance >> 8) & 0xFF;
    uint8_t varb = variance & 0xFF;

    for (uint8_t i = _sopt_getBaseOffset(); i < _sopt_getPixelCount(); i++)
    {
        uint8_t r = ((color >> 16) & 0xFF) + random(-varr, varr);
        uint8_t g = ((color >> 8) & 0xFF) + random(-varg, varg);
        uint8_t b = (color & 0xFF) + random(-varb, varb);
        setPixelColor(i, strip.Color(r, g, b));
    }

    _sopt_show();
}
#endif

#ifndef NO_STROBE
void strobe()
{
    if (! _sopt_checkTime(NEOPIXEL_STROBE_DELAY))
    {
        return;
    }

    int16_t offset = _sopt_getBaseOffset();
    int16_t count = _sopt_getPixelCount();
    setStripColor(offset, count, _settings_getColor());
    _sopt_show();
    setStripColor(offset, count, 0);
    _sopt_show();
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

    int16_t offset = _sopt_updateOffset();
    if (b > 80)
    {
        offset = 0;
    }

    int16_t baseOffset = _sopt_getBaseOffset();
    bool trigger = false;
    if (offset < 4)
    {
        trigger = true;
    }
    else if (offset > 7)
    {
        _sopt_setOffset(baseOffset + 4);
        trigger = true;
    }


    if (trigger)
    {
        for (uint8_t i = _sopt_getPixelCount() - 1; i > baseOffset; i--)
        {
            setPixelColor(i, strip.getPixelColor(i - 1));
        }
    }

    setPixelColor(baseOffset, strip.Color(r, g, b));
    _sopt_show();
}
#endif

void readCommand(char* buffer)
{
    uint8_t offset = 0;
    char current = '\0';

    while (true)
    {
        wdt_reset();
        if (client->available())
        {
            current = client->read();
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

void processCommand(char* buffer)
{
    wdt_reset();

#ifndef NO_EFFECTS
    SERIAL_PRINTLN((const char*) buffer);
    if (strcmp(buffer, "st") == 0)
    {
        PRINT_CONTENT(F("Stop"));
        FIX_BRIGHTNESS();
        _sopt_setState(STATE_NONE);
    }
#ifndef NO_RAINBOW
    else if (strcmp(buffer, "rb") == 0)
    {
        PRINT_CONTENT(F("Rainbow"));
        FIX_BRIGHTNESS();
        _sopt_initState(STATE_RAINBOW);
    }
#endif
#ifndef NO_FADE
    else if (strcmp(buffer, "fd") == 0)
    {
        PRINT_CONTENT(F("Fade"));
        FIX_BRIGHTNESS();
        _sopt_initState(STATE_COLOR_FADE);
    }
#endif
#ifndef NO_RUN
    else if (strcmp(buffer, "rn") == 0)
    {
        PRINT_CONTENT(F("Run"));
        FIX_BRIGHTNESS();
        _sopt_initState(STATE_RUN);
    }
#endif
#ifndef NO_CYLON
    else if (strcmp(buffer, "cy") == 0)
    {
        PRINT_CONTENT(F("Cylon"));
        FIX_BRIGHTNESS();
        _sopt_initState(STATE_CYLON);
    }
#endif
#ifndef NO_FIREWORKS
    else if (strcmp(buffer, "fw") == 0)
    {
        PRINT_CONTENT(F("Fireworks"));
        FIX_BRIGHTNESS();
        _sopt_initState(STATE_FIREWORKS);
    }
#endif
#ifndef NO_CANDLE
    else if (strcmp(buffer, "cn") == 0)
    {
        PRINT_CONTENT(F("Candle"));
        FIX_BRIGHTNESS();
        _sopt_setState(STATE_FLICKER);
        _sopt_setColor(0xBF3F00);
        settings.setVariance(0x402800);
    }
#endif
#ifndef NO_STROBE
    else if (strcmp(buffer, "sr") == 0)
    {
        PRINT_CONTENT(F("Strobe"));
        strip.initBrightness(255);
        _sopt_initState(STATE_STROBE);
    }
#endif
#ifndef NO_SPECTRUM
    else if (strcmp(buffer, "sp") == 0)
    {
        PRINT_CONTENT(F("Spectrum"));
        strip.initBrightness(255);
        _sopt_initState(STATE_SPECTRUM);
    }
#endif
    else
#endif
    if (strlen(buffer) > 1)
    {
        if (buffer[0] == 'c')
        {
            PRINT_CONTENT(F("Color: #"));
            _sopt_initState(STATE_COLOR_CHANGE);
            uint32_t color = hexStrToInt(++buffer);
            _sopt_setColor(color);
            _sopt_clientPrintValue(color, HEX);
        }
        else if (buffer[0] == 'o')
        {
            PRINT_CONTENT(F("Offset: "));
            int16_t offset = atoi(++buffer);
            settings.setBaseOffset(offset);
            _sopt_clientPrintValue(offset, DEC);

            setStripColor(0, offset, 0);
            _sopt_show();
        }
        else if (buffer[0] == 'p')
        {
            PRINT_CONTENT(F("Pixels: "));
            int16_t length = atoi(++buffer);
            settings.setPixelCount(length);
            _sopt_clientPrintValue(length, DEC);

            setStripColor(length, NEOPIXEL_COUNT, 0);
            _sopt_show();
        }
#ifndef NO_FLICKER
        else if (buffer[0] == 'f')
        {
            PRINT_CONTENT(F("Flicker: #"));
            FIX_BRIGHTNESS();
            _sopt_setState(STATE_FLICKER);
            uint32_t variance = hexStrToInt(++buffer);
            settings.setVariance(variance);
            _sopt_clientPrintValue(variance, HEX);
        }
#endif
#ifndef NO_SPECTRUM
        else if (buffer[0] == 's')
        {
            PRINT_CONTENT(F("Sensitivity: "));
            uint8_t sensitivity = atoi(++buffer);
            settings.setSensitivity(sensitivity);
            _sopt_clientPrintValue(sensitivity, DEC);
        }
        else if (buffer[0] == 'n')
        {
            PRINT_CONTENT(F("Cutoff: "));
            uint8_t cutoff = atoi(++buffer);
            settings.setCutoff(cutoff);
            _sopt_clientPrintValue(cutoff, DEC);
        }
        else if (buffer[0] == 'r')
        {
            PRINT_CONTENT(F("Rolloff: "));
            uint8_t frequencyRolloff = atoi(++buffer);
            settings.setFrequencyRolloff(frequencyRolloff);
            _sopt_clientPrintValue(frequencyRolloff, DEC);
         }
#endif
        else if (buffer[0] == 'b')
        {
            PRINT_CONTENT(F("Brightness: "));
            setBrightness(atoi(++buffer));
            _sopt_clientPrintValue(_sopt_getBrightness(), DEC);
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

    uint8_t state = _sopt_getState();

    // Try to get a client which is connected.
    wdt_reset();
    Adafruit_CC3000_ClientRef c = webServer.available();
    client = &c;
    if (client && client->available() > 0)
    {
        SERIAL_PRINTLN("R");

        char buffer[12];
        readCommand(buffer);

        wdt_reset();
        _sopt_clientPrintString((const __FlashStringHelper*)website_http_data);
        PRINT_CONTENT((const __FlashStringHelper*)website_doctype_data);
        _sopt_clientPrintString((const __FlashStringHelper*)website_header_data);

#if WEBSITE_TYPE != WEBSITE_NONE
        wdt_reset();
        _sopt_clientPrintString((const __FlashStringHelper*)website_options_data);
        _sopt_clientPrintValue(_sopt_getBrightness(), DEC);
        _sopt_clientPrintString((const __FlashStringHelper*)website_color_data);
        _sopt_clientPrintValue(_sopt_getColor(), HEX);
        wdt_reset();
        _sopt_clientPrintString((const __FlashStringHelper*)website_version_data);
        _sopt_clientPrint(VERSION);
        _sopt_clientPrintString((const __FlashStringHelper*)website_reboot_data);
        _sopt_clientPrintValue(debugState.rebootCount, DEC);
        wdt_reset();
        _sopt_clientPrintString((const __FlashStringHelper*)website_disconnect_data);
        _sopt_clientPrintValue(debugState.disconnectCount, DEC);
        _sopt_clientPrintString((const __FlashStringHelper*)website_prevState_data);
        _sopt_clientPrint(debugState.prevDebugStage);
#endif

        if ((buffer[0] != '\0') && (buffer[0] != ' '))
        {
            PRINT_CONTENT((const __FlashStringHelper*)website_command_data);
            processCommand(buffer);
        }

        wdt_reset();
        PRINT_CONTENT((const __FlashStringHelper*)website_footer_data);

#ifdef WEBSITE_HEADER_SEND_LENGTH
        for (uint8_t remaining_length = WEBSITE_VARIABLE_LENGTH - strlen(buffer) - 2; remaining_length > 0; remaining_length--)
        {
            client->write('\0');
        }
#endif

        client->write('\0');
        delay(5);
        wdt_reset();
        client->close();
        client = NULL;
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
#ifndef NO_STROBE
        case STATE_STROBE:
            strobe();
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
