#include <avr/pgmspace.h>

// Uncomment to enable serial print debugging
// #define DEBUG

// Uncomment to enable DEMO mode, where it cycles through inputs and mutes itself randomly
// and will ignore normal user input
#define DEMO

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
  0x03, 0xff, 0xfc, 0x00, 0x03, 0xff, 0xfc, 0x00, 0x00, 0x3f, 0xc0, 0x00, 0x00, 0x3f, 0xc0, 0x00
};
// 'bt_pixel_28x28', 18x28px
const unsigned char input_bmp_bt_pixel[] PROGMEM = {
  0x07, 0xf8, 0x00, 0x0f, 0xfc, 0x00, 0x3f, 0xff, 0x00, 0x3f, 0xff, 0x00, 0x7f, 0x7f, 0x80, 0xff,
  0x3f, 0xc0, 0xff, 0x1f, 0xc0, 0xff, 0x0f, 0xc0, 0xff, 0x27, 0xc0, 0xf7, 0x33, 0xc0, 0xf3, 0x23,
  0xc0, 0xf9, 0x07, 0xc0, 0xfc, 0x0f, 0xc0, 0xfe, 0x1f, 0xc0, 0xfe, 0x3f, 0xc0, 0xfc, 0x1f, 0xc0,
  0xf8, 0x0f, 0xc0, 0xf1, 0x27, 0xc0, 0xf3, 0x33, 0xc0, 0xff, 0x27, 0xc0, 0xff, 0x0f, 0xc0, 0xff,
  0x1f, 0xc0, 0xff, 0x1f, 0xc0, 0x7f, 0x3f, 0x80, 0x3f, 0x7f, 0x00, 0x3f, 0xff, 0x00, 0x0f, 0xfc,
  0x00, 0x07, 0xf8, 0x00
};
// 'aux_pixel_28x28', 17x28px
const unsigned char input_bmp_aux_pixel[] PROGMEM = {
  0x04, 0x00, 0x00, 0x0e, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f,
  0x00, 0x00, 0x1f, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x1f, 0x01,
  0x80, 0x1f, 0x03, 0x00, 0x3f, 0x86, 0x00, 0xff, 0xe2, 0x00, 0xff, 0xe3, 0x00, 0xff, 0xe1, 0x80,
  0xff, 0xe0, 0x80, 0xff, 0xe0, 0x80, 0xff, 0xe3, 0x80, 0xff, 0xe2, 0x00, 0xff, 0xe2, 0x00, 0xff,
  0xe3, 0x00, 0xff, 0xe1, 0x00, 0xff, 0xe1, 0x00, 0xff, 0xe1, 0x00, 0xff, 0xe1, 0x00, 0x04, 0x03,
  0x00, 0x07, 0xfe, 0x00
};

struct t_image {
  const int width;
  const int height;
  const unsigned char *data;
};

// Audio input relay output trigger pins and descriptions of the inputs for the LCD
struct t_signal_input {
  const bool enabled;
  const int relay_pin;
  const char *desc;
  const t_image image;
};

t_signal_input inputs[] = {
  { true, 6, "Spotify", { 28, 28, input_bmp_spotify_pixel } },
  { true, 7, "BT", { 18, 28, input_bmp_bt_pixel } },
  { true, 8, "Aux", { 17, 28, input_bmp_aux_pixel } },
  { false, 9, "Int. BT", { 0, 0, nullptr } }
};
const int number_of_inputs = sizeof(inputs) / sizeof(inputs[0]);

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
void switch_input(int input, int old_input = -1);

struct data_store {
  byte selected_input = 0;
  bool muted = false;
  bool leds = false;
} settings;

struct t_input_switching {
  bool is_switching = false;
  int from = -1;
  int to = 0;
} switching_inputs;

struct t_rect {
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
  { 3, &select_next_input },
  { 2, &toggle_mute },
  { 10, &toggle_leds }
};
const int number_of_buttons = sizeof(button) / sizeof(buttons[0]);

unsigned long last_active_millis = 0;

bool is_active = false;

/**
 * Center text horizontally and vertically within the bounds of a given
 * box of rows.
 */
t_rect lcd_center_text(DISPLAY_LCD_TYPE &lcd, const char str[], int x = 0, int y = 0, int height = SCREEN_HEIGHT, int color = DISPLAY_WHITE, int fsize_x = 1, int fsize_y = 0) {
  if (fsize_y < 1) {
    fsize_y = fsize_x;
  }

  auto text_width = (strlen(str) + 1) * (FONT_WIDTH * fsize_x);
  auto text_height = (FONT_HEIGHT * fsize_y);

  t_rect bounding_box;
  bounding_box.x1 = (SCREEN_WIDTH - x) / 2 - (text_width / 2);
  bounding_box.y1 = (height / 2 - text_height / 2);
  bounding_box.x2 = bounding_box.x1 + text_width;
  bounding_box.y2 = bounding_box.y1 + text_height;

  lcd.setCursor(x + bounding_box.x1, y + bounding_box.y1);
  lcd.setTextColor(color);
  lcd.setTextSize(fsize_x, fsize_y);
  lcd.print(str);

  return bounding_box;
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
#ifdef DEBUG
  Serial.begin(115200);
#endif

  int num_enabled_inputs = 0;
  for (auto i = 0; i < number_of_inputs; i++) {
    if (!inputs[i].enabled) {
      continue;
    }

    pinMode(inputs[i].relay_pin, OUTPUT);
    num_enabled_inputs++;
  }

  if (num_enabled_inputs == 0) {
#ifdef DEBUG
    Serial.println(F("No inputs enabled. Please enable at least one output."));
#endif
    // This is an error state.
    for (;;)
      ;  // Don't proceed, loop forever
  }

  pinMode(OUTPUT_PIN, OUTPUT);

#ifdef LED_PIN
  pinMode(LED_PIN, OUTPUT);
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
  lcd_center_text(lcd, "Audio", 0, 0, half_height, DISPLAY_WHITE, 2);
  lcd_center_text(lcd, "Switcher", 0, half_height, half_height, DISPLAY_WHITE, 2);

  lcd.display();
  delay(1000);

  //read_eeprom();
  switch_input(settings.selected_input);
  setLEDs(settings.leds);

  has_activity();
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

    lcd_center_text(lcd, "Mute", 0, MUTE_TEXT_TOP, SCREEN_TOP_BAR, DISPLAY_BLACK, 2);
  } else {
    lcd.fillRect(0, MUTE_TEXT_TOP, SCREEN_WIDTH, SCREEN_HEIGHT, DISPLAY_BLACK);

    if (switching_inputs.is_switching) {
      lcd_center_text(lcd, "Wait", 0, MUTE_TEXT_TOP, SCREEN_TOP_BAR, DISPLAY_WHITE, 2);
    } else {
      lcd_center_text(lcd, "Active", 0, MUTE_TEXT_TOP, SCREEN_TOP_BAR, DISPLAY_WHITE, 2);
    }
  }
}
void display_write_switching_outputs() {
  char text[20];
  sprintf(text, "#%d \x1A #%d", switching_inputs.from + 1, switching_inputs.to + 1);

  // Adjust width of font depending on the length of the string.
  auto fx = strlen(text) < 10 ? 2 : 1;
  lcd_center_text(lcd, text, 0, 0, SCREEN_HEIGHT - SCREEN_TOP_BAR, DISPLAY_WHITE, fx, 4);
}

void update_display_input_text() {
  // Clear drawing area and draw the bounding rect
  lcd.writeFillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT - SCREEN_TOP_BAR, DISPLAY_BLACK);
  lcd.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT - SCREEN_TOP_BAR, DISPLAY_WHITE);

  auto is_switching = switching_inputs.is_switching && switching_inputs.from > -1;
  if (is_switching) {
    display_write_switching_outputs();
    return;
  }

  auto desc = inputs[settings.selected_input].desc;
  auto image = inputs[settings.selected_input].image;
  auto has_image = image.data != nullptr;

  auto x_offset = has_image ? image.width + 2 : 0;
  unsigned int max_txt_len = has_image ? 8 : 10;

  // Adjust width of font depending on the length of the string.
  auto fx = strlen(desc) < max_txt_len ? 2 : 1;
  auto br = lcd_center_text(lcd, desc, x_offset, 0, SCREEN_HEIGHT - SCREEN_TOP_BAR, DISPLAY_WHITE, fx, 4);

  if (has_image) {
    auto bmp_offset = (SCREEN_HEIGHT - SCREEN_TOP_BAR) / 2 - (image.height / 2);
    lcd.drawBitmap(br.x1 - 2, bmp_offset, image.data, image.width, image.height, DISPLAY_WHITE);
  }
}

/**
 * Update display
 */
void update_display() {
  lcd.clearDisplay();

  update_display_active_region();
  update_display_input_text();

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
void switch_input(int input, int old_input) {
  // Disable output
  if (!settings.muted) {
    setMute(true);
    delay(20);
  }

  switching_inputs.is_switching = true;
  switching_inputs.from = old_input;
  switching_inputs.to = input;

  update_display();

  toggleRelay(inputs[settings.selected_input].relay_pin, LOW);
  delay(20);
  toggleRelay(inputs[input].relay_pin, HIGH);

  if (!settings.muted) {
    // Re-enable output
    delay(SWITCH_DELAY);
    setMute(false);
  }

  switching_inputs.is_switching = false;

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
#ifdef DEBUG
  Serial.print("settings.leds = ");
  Serial.println(settings.leds);
#endif
  setLEDs(settings.leds);
  save_eeprom();
}

/**
 * Select next input, cycling through the available inputs
 */
void select_next_input() {
  auto old_input = settings.selected_input;
  auto new_input = old_input;

  while (true) {
    new_input++;

    if (new_input == number_of_inputs) {
      new_input = 0;
    }

    if (inputs[new_input].enabled) {
      break;
    }
  }

  switch_input(new_input, old_input);
  has_activity();
}

/**
 * Check buttons for events
 */
void check_buttons() {
  for (int i = 0; i < number_of_buttons; i++) {
    buttons[i].check();
  }
}

void random_action() {
  auto val = random(1, 50);

  if (val < 40) {
    select_next_input();
  } else {
    toggle_mute();
  }
}

unsigned long last_demo_millis = 0;

void loop() {
  if (is_active && (millis() - last_active_millis) > INACTIVITY_TIMEOUT) {
    inactive();
  }

#ifndef DEMO
  check_buttons();
#endif

#ifdef DEMO
  if ((millis() - last_demo_millis) > 4000) {
    random_action();
    last_demo_millis = millis();
  }
#endif
}
