#ifndef AUDIOSWITCHER_H
#define AUDIOSWITCHER_H
#include <avr/pgmspace.h>

// Uncomment to enable serial print debugging
#define DEBUG

// 1 = SSD1306 128x64 with bottom part being yellow instead of cyan
// 2 = SH1106G 128x64 cyan
#define DISPLAY_TYPE 2

// Timeout for dimming LCD in milliseconds
// Doesn't work super well for OLEDs, but whatever.
#define INACTIVITY_TIMEOUT 5 * 1000

// 'spotify_pixel_28x28', 28x28px
const unsigned char input_bmp_spotify_pixel[] PROGMEM = {
    0x00, 0x3f, 0xc0, 0x00, 0x00, 0x3f, 0xc0, 0x00, 0x03, 0xff, 0xfc, 0x00, 0x03, 0xff, 0xfc, 0x00,
    0x0f, 0xff, 0xff, 0x00, 0x0f, 0xff, 0xff, 0x00, 0x3f, 0xff, 0xff, 0xc0, 0x3f, 0xff, 0xff, 0xc0,
    0x3c, 0x00, 0x03, 0xc0, 0xfc, 0x00, 0x03, 0xf0, 0xf3, 0xff, 0xfc, 0xf0, 0xf3, 0xff, 0xfc, 0xf0,
    0xff, 0x00, 0x0f, 0xf0, 0xff, 0x00, 0x0f, 0xf0, 0xfc, 0xff, 0xf3, 0xf0, 0xfc, 0xff, 0xf3, 0xf0,
    0xff, 0xc0, 0x3f, 0xf0, 0xff, 0xc0, 0x3f, 0xf0, 0xff, 0x3f, 0xcf, 0xf0, 0x3f, 0x3f, 0xcf, 0xc0,
    0x3f, 0xff, 0xff, 0xc0, 0x3f, 0xff, 0xff, 0xc0, 0x0f, 0xff, 0xff, 0x00, 0x0f, 0xff, 0xff, 0x00,
    0x03, 0xff, 0xfc, 0x00, 0x03, 0xff, 0xfc, 0x00, 0x00, 0x3f, 0xc0, 0x00, 0x00, 0x3f, 0xc0, 0x00};
// 'bt_pixel_28x28', 18x28px
const unsigned char input_bmp_bt_pixel[] PROGMEM = {
    0x07, 0xf8, 0x00, 0x0f, 0xfc, 0x00, 0x3f, 0xff, 0x00, 0x3f, 0xff, 0x00, 0x7f, 0x7f, 0x80, 0xff,
    0x3f, 0xc0, 0xff, 0x1f, 0xc0, 0xff, 0x0f, 0xc0, 0xff, 0x27, 0xc0, 0xf7, 0x33, 0xc0, 0xf3, 0x23,
    0xc0, 0xf9, 0x07, 0xc0, 0xfc, 0x0f, 0xc0, 0xfe, 0x1f, 0xc0, 0xfe, 0x3f, 0xc0, 0xfc, 0x1f, 0xc0,
    0xf8, 0x0f, 0xc0, 0xf1, 0x27, 0xc0, 0xf3, 0x33, 0xc0, 0xff, 0x27, 0xc0, 0xff, 0x0f, 0xc0, 0xff,
    0x1f, 0xc0, 0xff, 0x1f, 0xc0, 0x7f, 0x3f, 0x80, 0x3f, 0x7f, 0x00, 0x3f, 0xff, 0x00, 0x0f, 0xfc,
    0x00, 0x07, 0xf8, 0x00};
// 'aux_pixel_28x28', 17x28px
const unsigned char input_bmp_aux_pixel[] PROGMEM = {
    0x04, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f,
    0x00, 0x00, 0x1f, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x1f, 0x01,
    0x80, 0x1f, 0x03, 0x00, 0x3f, 0x86, 0x00, 0xff, 0xe2, 0x00, 0xff, 0xe3, 0x00, 0xff, 0xe1, 0x80,
    0xff, 0xe0, 0x80, 0xff, 0xe0, 0x80, 0xff, 0xe3, 0x80, 0xff, 0xe2, 0x00, 0xff, 0xe2, 0x00, 0xff,
    0xe3, 0x00, 0xff, 0xe1, 0x00, 0xff, 0xe1, 0x00, 0xff, 0xe1, 0x00, 0xff, 0xe1, 0x00, 0x04, 0x03,
    0x00, 0x07, 0xfe, 0x00};

struct t_image
{
    const int width;
    const int height;
    const unsigned char *data;
};

// Audio input relay output trigger pins and descriptions of the inputs for the LCD
struct t_signal_input
{
    const bool enabled;
    const int relay_pin;
    const char *desc;
    const t_image image;
};

t_signal_input inputs[] = {
    {true, 6, "Spotify", {28, 28, input_bmp_spotify_pixel}},
    {true, 7, "BT", {18, 28, input_bmp_bt_pixel}},
    {true, 8, "Aux", {17, 28, input_bmp_aux_pixel}},
    {false, 9, "Int. BT", {0, 0, nullptr}}};
const int number_of_inputs = sizeof(inputs) / sizeof(inputs[0]);

// Delay, in ms, between switching inputs
#define SWITCH_DELAY 250

// Mute/output relay
#define OUTPUT_PIN 5

// Pin for enabling the relay LEDs.
// Comment out to disable LEDs.
#define LED_PIN 4

#include <SPI.h>
#include <EEPROM.h>
#include <Wire.h>
#include <Adafruit_GFX.h>

#if DISPLAY_TYPE == 1
#define DISPLAY_SSD1306_128X64
#define DISPLAY_LCD_TYPE Adafruit_SSD1306
#define DISPLAY_WHITE SSD1306_WHITE
#define DISPLAY_BLACK SSD1306_BLACK

#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define SCREEN_TOP_BAR 16
#define LCD_INVERT 0
#define FONT_WIDTH 5
#define FONT_HEIGHT 7

#elif DISPLAY_TYPE == 2
#define DISPLAY_SH1106G_128X64
#define DISPLAY_LCD_TYPE Adafruit_SH1106G
#define DISPLAY_WHITE SH110X_WHITE
#define DISPLAY_BLACK SH110X_BLACK

#include <Adafruit_SH110X.h>
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define SCREEN_TOP_BAR 24
#define LCD_INVERT 0
#define FONT_WIDTH 5
#define FONT_HEIGHT 7

#endif

#define MUTE_TEXT_TOP SCREEN_HEIGHT - SCREEN_TOP_BAR

struct data_store
{
    byte selected_input = 0;
    bool muted = false;
    bool leds = false;
} settings;

struct t_input_switching
{
    bool is_switching = false;
    int from = -1;
    int to = 0;
} switching_inputs;

struct t_rect
{
    int x1;
    int y1;
    int x2;
    int y2;
};

/**
 * Input button class to configure and detect button presses
 * via an analog input pin and a range of min/max analog reading
 * - OR -
 * a single digital pin, HIGH being pressed.
 */
struct button
{
    bool m_state = false;
    unsigned long m_debounce_millis;

    int pin;
    int a_val_min;
    int a_val_max;
    void (*fn_pressed)() = NULL;
    void (*fn_released)() = NULL;
    int m_debounce_delay;
    bool analog_mode = false;

    // Analog resistor-based input buttons
    button(int input_pin, int aval_min, int aval_max, void (*fnPressed)() = NULL, void (*fnReleased)() = NULL, int debounce_delay = 100)
        : pin(input_pin), a_val_min(aval_min), a_val_max(aval_max), fn_pressed(fnPressed), fn_released(fnReleased), m_debounce_delay(debounce_delay), analog_mode(true) {}

    // Digital input buttons
    button(int input_pin, void (*fnPressed)() = NULL, void (*fnReleased)() = NULL, int debounce_delay = 100)
        : pin(input_pin), fn_pressed(fnPressed), fn_released(fnReleased), m_debounce_delay(debounce_delay)
    {
        // Using the internal pullup means we can use only one grounding resistor
        // for all the buttons, instead of one per button/pin.
        pinMode(input_pin, INPUT_PULLUP);
    }

    /**
     * Check state of the button
     */
    bool check()
    {
        if (!m_state)
        {
            if (check_state())
            {
                state_changed(true);
                return true;
            }
        }
        else
        {
            if (!check_state())
            {
                state_changed(false);
            }
        }

        return false;
    }

    /**
     * Check the state of the pin depending on the input mode
     */
    bool check_state()
    {
        if (analog_mode)
        {
            auto aval = analogRead(pin);
            return aval >= a_val_min && aval <= a_val_max;
        }

        return digitalRead(pin) == LOW;
    }

    void state_changed(bool state)
    {
        if (m_debounce_millis > 0 && m_debounce_millis + m_debounce_delay > millis())
        {
            m_debounce_millis = 0;
            return;
        }

        m_debounce_millis = millis();

        m_state = state;

        if (m_state)
        {
            if (fn_pressed != NULL)
            {
                (*fn_pressed)();
            }

#ifdef DEBUG
            Serial.print("PRESS: ");
#endif
        }
        else
        {
            if (fn_released != NULL)
            {
                (*fn_released)();
            }
#ifdef DEBUG
            Serial.print("RELEASE: ");
#endif
        }

#ifdef DEBUG
        Serial.print("Button on pin ");
        Serial.println(pin);
#endif
    }
};


// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
DISPLAY_LCD_TYPE lcd(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//void setup();
t_rect lcd_center_text(DISPLAY_LCD_TYPE &lcd, const char str[], int x = 0, int y = 0, int height = SCREEN_HEIGHT, int color = DISPLAY_WHITE, int fsize_x = 1, int fsize_y = 0);
void has_activity();
void inactive();
void dither_box(int x1, int y1, int x2, int y2, int color);
void lcd_dim();
void update_display_active_region();
void display_write_switching_outputs();
void update_display_input_text();
void update_display();
void eeprom_save();
void eeprom_read();
void toggle_relay(int pin, int state);
void switch_input(int input, int old_input = -1);
void set_mute(bool muted);
void set_LEDs(bool state);
void toggle_mute();
void toggle_leds();
void select_next_input();
void select_prev_input();
void check_buttons();
void random_action();
//void loop();

#endif