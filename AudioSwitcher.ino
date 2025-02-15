// Uncomment to enable serial print debugging
#define DEBUG

// 1 = SSD1306 128x64 with bottom part being yellow instead of cyan
// 2 = SH1106G 128x64 cyan
#define DISPLAY_TYPE 2

// Timeout for dimming LCD in milliseconds
// Doesn't work super well for OLEDs, but whatever.
#define INACTIVITY_TIMEOUT 5 * 1000

// Button inputs
// Pin, callback()
#define BTN_INPUT_SEL \
  { 3, &select_next_input }
#define BTN_TOGGLE_MUTE \
  { 2, &toggle_mute }
#define BTN_TOGGLE_LEDS \
  { 10, &toggle_leds }
#define NUM_BUTTONS 3

// Audio input relay output trigger pins and descriptions
// of the inputs for the LCD
//
// Inputs 1...N
#define RELAY_INPUT_1_PIN 6
#define RELAY_INPUT_2_PIN 7
#define RELAY_INPUT_3_PIN 8
#define RELAY_INPUT_4_PIN 9
#define INPUT_RELAY_PINS \
  { RELAY_INPUT_1_PIN, RELAY_INPUT_2_PIN, RELAY_INPUT_3_PIN };
#define INPUT_DESCRIPTIONS \
  { "Spotify", "Bluetooth", "Aux" }

// Delay, in ms, between switching inputs
#define SWITCH_DELAY 250

// Mute/output relay
#define OUTPUT_PIN 5

// Pin for enabling the relay LEDs.
// Comment out to disable LEDs.
#define LED_PIN 4

#include <Arduino.h>
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
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
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
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define SCREEN_TOP_BAR 24
#define LCD_INVERT 0
#define FONT_WIDTH 5
#define FONT_HEIGHT 7

#endif

#define MUTE_TEXT_TOP SCREEN_HEIGHT - SCREEN_TOP_BAR

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
DISPLAY_LCD_TYPE lcd(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Required to forward-declare these for use with 'buttons'
void select_next_input();
void toggle_mute();
void toggle_leds();

struct data_store {
  byte selected_input = 0;
  bool muted = false;
  bool leds = false;
};

/**
 * Input button class to configure and detect button presses
 * via an analog input pin and a range of min/max analog reading
 * - OR -
 * a single digital pin, HIGH being pressed.
 */
struct button {
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
    : pin(input_pin), fn_pressed(fnPressed), fn_released(fnReleased), m_debounce_delay(debounce_delay) {
    // Using the internal pullup means we can use only one grounding resistor
    // for all the buttons, instead of one per button/pin.
    pinMode(input_pin, INPUT_PULLUP);
  }

  /**
   * Check state of the button
   */
  bool check() {
    if (!m_state) {
      if (check_state()) {
        state_changed(true);
        return true;
      }
    } else {
      if (!check_state()) {
        state_changed(false);
      }
    }

    return false;
  }

  /**
   * Check the state of the pin depending on the input mode
   */
  bool check_state() {
    if (analog_mode) {
      auto aval = analogRead(pin);
      return aval >= a_val_min && aval <= a_val_max;
    }

    return digitalRead(pin) == LOW;
  }

  void state_changed(bool state) {
    if (m_debounce_millis > 0 && m_debounce_millis + m_debounce_delay > millis()) {
      m_debounce_millis = 0;
      return;
    }

    m_debounce_millis = millis();

    m_state = state;

    if (m_state) {
      if (fn_pressed != NULL) {
        (*fn_pressed)();
      }

#ifdef DEBUG
      Serial.print("PRESS: ");
#endif
    } else {
      if (fn_released != NULL) {
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

// Define the physical input buttons
button buttons[] = {
  BTN_INPUT_SEL,
  BTN_TOGGLE_MUTE,
  BTN_TOGGLE_LEDS
};

const char *inputDescriptions[] = INPUT_DESCRIPTIONS;
const int inputRelays[] = INPUT_RELAY_PINS;
data_store settings;

unsigned long last_active_millis = 0;
unsigned long timer1 = 0;

bool is_active = false;
bool is_switching_output = false;
int number_of_inputs = sizeof(inputRelays) / sizeof(inputRelays[0]);

/**
 * Center text horizontally and vertically within the bounds of a given
 * box of rows.
 */
void lcd_center_text(DISPLAY_LCD_TYPE &lcd, const char str[], int y = 0, int height = SCREEN_HEIGHT, int color = DISPLAY_WHITE, int fsize_x = 1, int fsize_y = 0) {
  if (fsize_y < 1) {
    fsize_y = fsize_x;
  }

  auto offset_x = SCREEN_WIDTH / 2 - ((strlen(str) + 1) * (FONT_WIDTH * fsize_x) / 2);
  auto offset_y = (height / 2 - (FONT_HEIGHT * fsize_y) / 2);

  lcd.setCursor(offset_x, y + offset_y);
  lcd.setTextColor(color);
  lcd.setTextSize(fsize_x, fsize_y);
  lcd.print(str);
}

// Dim the contrast as much a possible.
void display_dim() {
#if defined(DISPLAY_SSD1306_128X64)
  lcd.ssd1306_command(SSD1306_SETCONTRAST);
  lcd.ssd1306_command(0);
#elif defined(DISPLAY_SH1106G_128X64)
  lcd.oled_command(SH110X_SETCONTRAST);
  lcd.oled_command(0);
#endif
}

void setup() {
  for (auto i = 0; i < number_of_inputs; i++) {
    pinMode(inputRelays[i], OUTPUT);
  }

  pinMode(OUTPUT_PIN, OUTPUT);

#ifdef LED_PIN
  pinMode(LED_PIN, OUTPUT);
  //digitalWrite(LED_PIN, HIGH);
#endif

#ifdef DEBUG
  Serial.begin(115200);
#endif

#ifdef DEMO
  randomSeed(analogRead(0));
#endif

  SPI.begin();

#if defined(DISPLAY_SSD1306_128X64)
  auto display_begin = lcd.begin(SSD1306_SWITCHCAPVCC, 0x3C);
#elif defined(DISPLAY_SH1106G_128X64)
  auto display_begin = lcd.begin(0x3C);
#endif

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display_begin) {  // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;  // Don't proceed, loop forever
  }

#if defined(DISPLAY_SSD1306_128X64) || defined(DISPLAY_SH1106G_128X64)
  display_dim();
#endif

  lcd.invertDisplay(LCD_INVERT != 0);
  lcd.cp437(true);  // Use full 256 char 'Code Page 437' font
  lcd.setRotation(90);

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  lcd.clearDisplay();

  auto half_height = (SCREEN_HEIGHT - SCREEN_TOP_BAR) / 2;
  lcd_center_text(lcd, "Audio", 0, half_height, DISPLAY_WHITE, 2);
  lcd_center_text(lcd, "Switcher", half_height, half_height, DISPLAY_WHITE, 2);

  lcd.display();
  delay(1000);

  //read_eeprom();
  switch_input(settings.selected_input);
  setLEDs(settings.leds);

  has_activity();
}

/**
 * Get the description of an input, if available
 */
const char *get_input_desc(unsigned int input) {
  if (input >= (sizeof(inputDescriptions) / sizeof(inputDescriptions[0]))) {
    return "(no desc)";
  }

  return inputDescriptions[input];
}

/**
 * Set device active
 */
void has_activity() {
  update_display();

  if (!is_active) {
    update_display();
  }

  is_active = true;
  last_active_millis = millis();
}

/**
 * Set device inactive
 */
void inactive() {
  is_active = false;

  display_dim();

  lcd_dim();
  lcd.display();
}

void dither_box(int x1, int y1, int x2, int y2, int color) {
  for (auto y = y1; y < y2; y += 2) {
    lcd.drawLine(x1, y, x2, y, color);
  }
  for (auto x = x1; x < x2; x += 2) {
    lcd.drawLine(x, y1, x, y2, color);
  }
}

void lcd_dim() {
  dither_box(0, MUTE_TEXT_TOP, SCREEN_WIDTH, SCREEN_HEIGHT, DISPLAY_BLACK);
  dither_box(1, 1, SCREEN_WIDTH - 2, SCREEN_HEIGHT - SCREEN_TOP_BAR - 2, DISPLAY_BLACK);
}

void update_display_active_region() {
  // Mute state
  if (settings.muted) {
    lcd.fillRect(0, MUTE_TEXT_TOP, SCREEN_WIDTH, SCREEN_HEIGHT, DISPLAY_WHITE);

    lcd_center_text(lcd, "Mute", MUTE_TEXT_TOP, SCREEN_TOP_BAR, DISPLAY_BLACK, 2);
  } else {
    lcd.fillRect(0, MUTE_TEXT_TOP, SCREEN_WIDTH, SCREEN_HEIGHT, DISPLAY_BLACK);

    if (is_switching_output) {
      lcd_center_text(lcd, "Wait", MUTE_TEXT_TOP, SCREEN_TOP_BAR, DISPLAY_WHITE, 2);
    } else {
      lcd_center_text(lcd, "Active", MUTE_TEXT_TOP, SCREEN_TOP_BAR, DISPLAY_WHITE, 2);
    }
  }
}

/**
 * Update display
 */
void update_display() {
  lcd.clearDisplay();

  update_display_active_region();
  auto desc = get_input_desc(settings.selected_input);
  // Adjust width of font depending on the length of the string.
  auto fx = strlen(desc) < 10 ? 2 : 1;
  lcd.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT - SCREEN_TOP_BAR, DISPLAY_WHITE);
  lcd_center_text(lcd, desc, 0, SCREEN_HEIGHT - SCREEN_TOP_BAR, DISPLAY_WHITE, fx, 4);

  lcd.display();
}
/**
 * Save settings to EEPROM
 * This isn't really super great, as it has a number of cycles
 * it can be written to before degrading.
 */
void save_eeprom() {
  EEPROM.put(0, settings);
}

/**
 * Read saved EEPROM settings
 */
void read_eeprom() {
  EEPROM.get(0, settings);

  settings.muted = bool(settings.muted);
  settings.leds = bool(settings.leds);

  if (settings.selected_input >= number_of_inputs) {
    settings.selected_input = 0;
  }
}

/**
 * Set relay state of the given pin
 */
void toggleRelay(int pin, int state) {
  digitalWrite(pin, state);
}

/**
 * Switch input from 'settings.selected_input' to 'input'
 */
void switch_input(int input) {
  // Disable output
  if (!settings.muted) {
    setMute(true);
    delay(20);
  }

  is_switching_output = true;
  update_display();

  toggleRelay(inputRelays[settings.selected_input], LOW);
  delay(20);
  toggleRelay(inputRelays[input], HIGH);

  if (!settings.muted) {
    // Re-enable output
    delay(SWITCH_DELAY);
    setMute(false);
  }

  is_switching_output = false;

  settings.selected_input = input;
  save_eeprom();
}

/**
 * Set muted state
 */
void setMute(bool muted) {
  toggleRelay(OUTPUT_PIN, muted ? LOW : HIGH);
}

/**
 * Set indicator LED state
 */
void setLEDs(bool state) {
  toggleRelay(LED_PIN, state ? LOW : HIGH);
}

/**
 * Toggle muting of the output relay
 */
void toggle_mute() {
  settings.muted = settings.muted ? false : true;
  setMute(settings.muted);
  save_eeprom();
  has_activity();
}

/**
 * Toggle enable of indicator LEDs on the PCB
 */
void toggle_leds() {
  settings.leds = settings.leds ? false : true;
  Serial.print("settings.leds = ");
  Serial.println(settings.leds);
  setLEDs(settings.leds);
  save_eeprom();
}

/**
 * Select next input, cycling through the available inputs
 */
void select_next_input() {
  auto new_input = settings.selected_input + 1;

  if (new_input == number_of_inputs) {
    new_input = 0;
  }

  switch_input(new_input);
  has_activity();
}

/**
 * Check buttons for events
 */
void check_buttons() {
  for (auto i = 0; i < NUM_BUTTONS; i++) {
    buttons[i].check();
  }
}

void loop() {
  if (is_active && (millis() - last_active_millis) > INACTIVITY_TIMEOUT) {
    inactive();
  }

  check_buttons();
}
