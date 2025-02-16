// Uncomment to enable serial prt_smallint debugging
#define DEBUG

// Uncomment to enable DEMO mode, where it cycles through inputs and mutes itself randomly
// and will ignore normal user input
// #define DEMO

// 1 = SSD1306 128x64 with bottom part being yellow instead of cyan
// 2 = SH1106G 128x64 cyan
#define DISPLAY_TYPE 1

// Timeout for dimming LCD in milliseconds
// Doesn't work super well for OLEDs, but whatever.
#define INACTIVITY_TIMEOUT 5 * 1000

#include "AudioSwitcher.h"

// Define the physical input buttons
button buttons[] = {
    {3, &select_next_input},
    {2, &toggle_mute},
    {10, &toggle_leds}};
const uint8_t number_of_buttons = sizeof(buttons) / sizeof(buttons[0]);

// Inputs can be reordered so switching order is different from physical order
t_signal_input inputs[] = {
    {true, 6, "Spotify", IMG_SPOTIFY},       // Phy #1
    {true, 9, "Int. BT", IMG_BLUETOOTH_INT}, // Phy #4
    {true, 8, "Aux", IMG_AUX},               // Phy #3
    {true, 7, "BT", IMG_BLUETOOTH_EXT}       // Phy #2
};
const uint8_t number_of_inputs = sizeof(inputs) / sizeof(inputs[0]);

unsigned long last_active_millis = 0;
unsigned long last_demo_millis = 0;
bool is_active = false;
bool is_initialized = false;

/**
 * Center text horizontally and vertically within the bounds of a given
 * box of rows.
 */
t_rect lcd_center_text(DISPLAY_LCD_TYPE &lcd, const char str[], uint8_t x, uint8_t y, uint8_t height, uint8_t color, uint8_t fsize_x, uint8_t fsize_y)
{
  if (fsize_y < 1)
  {
    fsize_y = fsize_x;
  }

  uint8_t text_width = (strlen(str) + 1) * (FONT_WIDTH * fsize_x);
  uint8_t text_height = (FONT_HEIGHT * fsize_y);

  t_rect bounding_box;
  bounding_box.x1 = (lcd.width() - x) / 2 - (text_width / 2);
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
void display_dim()
{
#if defined(DISPLAY_SSD1306_128X64)
  lcd.ssd1306_command(SSD1306_SETCONTRAST);
  lcd.ssd1306_command(0);
#elif defined(DISPLAY_SH1106G_128X64)
  lcd.oled_command(SH110X_SETCONTRAST);
  lcd.oled_command(0);
#endif
}

void setup()
{
#ifdef DEBUG
  Serial.begin(115200);
#endif

  uint8_t num_enabled_inputs = 0;
  for (uint8_t i = 0; i < number_of_inputs; i++)
  {
    if (!inputs[i].enabled)
    {
      continue;
    }

    pinMode(inputs[i].relay_pin, OUTPUT);
    num_enabled_inputs++;
  }

  if (num_enabled_inputs == 0)
  {
#ifdef DEBUG
    Serial.println(F("No inputs enabled. Please enable at least one output."));
#endif
    // This is an error state.
    for (;;)
      ; // Don't proceed, loop forever
  }

  pinMode(OUTPUT_PIN, OUTPUT);

#ifdef LED_PIN
  pinMode(LED_PIN, OUTPUT);
#endif

#ifdef DEMO
  randomSeed(analogRead(0));
#endif

#if defined(DISPLAY_SSD1306_128X64)
  auto display_begin = lcd.begin(SSD1306_SWITCHCAPVCC, 0x3C);
#elif defined(DISPLAY_SH1106G_128X64)
  auto display_begin = lcd.begin(0x3C);
#endif

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display_begin)
  { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }

  display_dim();

  lcd.invertDisplay(LCD_INVERT != 0);
  lcd.cp437(true); // Use full 256 char 'Code Page 437' font
  lcd.setRotation(90);

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  lcd.clearDisplay();

  uint8_t half_height = (lcd.height() - SCREEN_TOP_BAR) / 2;
  lcd_center_text(lcd, "Audio", 0, 0, half_height, DISPLAY_WHITE, 2);
  lcd_center_text(lcd, "Switcher", 0, half_height, half_height, DISPLAY_WHITE, 2);

  lcd.display();
  delay(1000);

  eeprom_read();
  switch_input(settings.selected_input);
  set_LEDs(settings.leds);

  has_activity();
  is_initialized = true;
}

/**
 * Set device active
 */
void has_activity()
{
  update_display();

  if (!is_active)
  {
    update_display();
  }

  is_active = true;
  last_active_millis = millis();
}

/**
 * Set device inactive
 */
void inactive()
{
  is_active = false;

  display_dim();

  lcd_dim();
  lcd.display();
}

void dither_box(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color)
{
  for (auto y = y1; y < y2; y += 2)
  {
    lcd.drawLine(x1, y, x2, y, color);
  }
  for (auto x = x1; x < x2; x += 2)
  {
    lcd.drawLine(x, y1, x, y2, color);
  }
}

void lcd_dim()
{
  dither_box(0, MUTE_TEXT_TOP, lcd.width(), lcd.height(), DISPLAY_BLACK);
  dither_box(1, 1, lcd.width() - 2, lcd.height() - SCREEN_TOP_BAR - 2, DISPLAY_BLACK);
}

void update_display_active_region()
{
  // Mute state
  if (settings.muted)
  {
    lcd.fillRect(0, MUTE_TEXT_TOP, lcd.width(), lcd.height(), DISPLAY_WHITE);

    lcd_center_text(lcd, "Mute", 0, MUTE_TEXT_TOP, SCREEN_TOP_BAR, DISPLAY_BLACK, 2);
  }
  else
  {
    lcd.fillRect(0, MUTE_TEXT_TOP, lcd.width(), lcd.height(), DISPLAY_BLACK);

    if (switching_inputs.is_switching)
    {
      lcd_center_text(lcd, "Wait", 0, MUTE_TEXT_TOP, SCREEN_TOP_BAR, DISPLAY_WHITE, 2);
    }
    else
    {
      lcd_center_text(lcd, "Active", 0, MUTE_TEXT_TOP, SCREEN_TOP_BAR, DISPLAY_WHITE, 2);
    }
  }
}
void display_write_switching_outputs()
{
  char text[20];
  sprintf(text, "#%d \x1A #%d", switching_inputs.from + 1, switching_inputs.to + 1);

  // Adjust width of font depending on the length of the string.
  uint8_t fx = strlen(text) < 10 ? 2 : 1;
  lcd_center_text(lcd, text, 0, 0, lcd.height() - SCREEN_TOP_BAR, DISPLAY_WHITE, fx, 4);
}

void update_display_input_text()
{
  // Clear drawing area and draw the bounding rect
  lcd.writeFillRect(0, 0, lcd.width(), lcd.height() - SCREEN_TOP_BAR, DISPLAY_BLACK);
  lcd.drawRect(0, 0, lcd.width(), lcd.height() - SCREEN_TOP_BAR, DISPLAY_WHITE);

  auto is_switching = switching_inputs.is_switching && switching_inputs.from != MAX_INPUTS;
  if (is_switching)
  {
    display_write_switching_outputs();
    return;
  }

  auto desc = inputs[settings.selected_input].desc;
  auto image = inputs[settings.selected_input].image;
  auto has_image = image.data != nullptr;

  uint8_t x_offset = has_image ? image.width + 2 : 0;
  uint8_t max_txt_len = has_image ? 8 : 10;

  // Adjust width of font depending on the length of the string.
  uint8_t fx = strlen(desc) < max_txt_len ? 2 : 1;
  auto br = lcd_center_text(lcd, desc, x_offset, 0, lcd.height() - SCREEN_TOP_BAR, DISPLAY_WHITE, fx, 4);

  if (has_image)
  {
    uint8_t bmp_offset = (lcd.height() - SCREEN_TOP_BAR) / 2 - (image.height / 2);
    lcd.drawBitmap(br.x1 - 2, bmp_offset, image.data, image.width, image.height, DISPLAY_WHITE);
  }
}

/**
 * Update display
 */
void update_display()
{
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
void eeprom_save()
{
#ifndef DEMO
  if (is_initialized)
  {
    EEPROM.put(0, settings);
  }
#endif
}

/**
 * Read saved EEPROM settings
 */
void eeprom_read()
{
  EEPROM.get(0, settings);

  settings.muted = bool(settings.muted);
  settings.leds = bool(settings.leds);

  if (settings.selected_input >= number_of_inputs)
  {
    settings.selected_input = 0;
  }
}

/**
 * Set relay state of the given pin
 */
void toggle_relay(uint8_t pin, uint8_t state)
{
  digitalWrite(pin, state);
}

/**
 * Switch input from 'settings.selected_input' to 'input'
 */
void switch_input(uint8_t input, uint8_t old_input)
{
  // Disable output
  if (!settings.muted)
  {
    set_mute(true);
    delay(20);
  }

  switching_inputs.is_switching = true;
  switching_inputs.from = old_input;
  switching_inputs.to = input;

  update_display();

  toggle_relay(inputs[settings.selected_input].relay_pin, LOW);
  delay(20);
  toggle_relay(inputs[input].relay_pin, HIGH);

  if (!settings.muted)
  {
    // Re-enable output
    delay(SWITCH_DELAY);
    set_mute(false);
  }

  switching_inputs.is_switching = false;

  settings.selected_input = input;
  eeprom_save();
}

/**
 * Set muted state
 */
void set_mute(bool muted)
{
  toggle_relay(OUTPUT_PIN, muted ? LOW : HIGH);
}

/**
 * Set indicator LED state
 */
void set_LEDs(bool state)
{
  toggle_relay(LED_PIN, state ? LOW : HIGH);
}

/**
 * Toggle muting of the output relay
 */
void toggle_mute()
{
  settings.muted = settings.muted ? false : true;
  set_mute(settings.muted);
  eeprom_save();
  has_activity();
}

/**
 * Toggle enable of indicator LEDs on the PCB
 */
void toggle_leds()
{
  settings.leds = settings.leds ? false : true;
#ifdef DEBUG
  Serial.print("settings.leds = ");
  Serial.println(settings.leds);
#endif
  set_LEDs(settings.leds);
  eeprom_save();
}

/**
 * Select next input, cycling through the available inputs
 */
void select_next_input()
{
  auto old_input = settings.selected_input;
  auto new_input = old_input;

  while (true)
  {
    new_input++;

    if (new_input == number_of_inputs)
    {
      new_input = 0;
    }

    if (inputs[new_input].enabled)
    {
      break;
    }
  }

  switch_input(new_input, old_input);
  has_activity();
}

/**
 * Select previous input, cycling through the available inputs
 */
void select_prev_input()
{
  auto old_input = settings.selected_input;
  auto new_input = old_input;

  while (true)
  {
    new_input--;

    if (new_input < 0)
    {
      new_input = number_of_inputs;
    }

    if (inputs[new_input].enabled)
    {
      break;
    }
  }

  switch_input(new_input, old_input);
  has_activity();
}

/**
 * Check buttons for events
 */
void check_buttons()
{
  for (uint8_t i = 0; i < number_of_buttons; i++)
  {
    buttons[i].check();
  }
}

void demo_mode()
{
  auto btn = random(number_of_buttons);
#ifdef DEBUG
  Serial.print("Random button #");
  Serial.println(btn);
#endif

  buttons[btn].trigger();
}

void loop()
{
#ifndef DEMO
  if (is_active && (millis() - last_active_millis) > INACTIVITY_TIMEOUT)
  {
    inactive();
  }

  check_buttons();
#endif

#ifdef DEMO
  if ((millis() - last_demo_millis) > 4000)
  {
    demo_mode();
    last_demo_millis = millis();
  }
#endif
}
