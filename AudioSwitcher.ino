#define REV1_0

#define DEBUG

// Number of inputs, external and internal
#ifdef REV1_1
#define NUMBER_OF_INPUTS 4
#else
#define NUMBER_OF_INPUTS 3
#endif

// Timeout for dimming LCD in milliseconds
#define INACTIVITY_TIMEOUT 20 * 1000

// SPI Chip-Select pin for digital pot for LCD backlight dimming
#define POT_CS_PIN 10

// Button inputs
#ifdef REV1_1
// Pin, analog level min, analog level max, callback()
#define BTN_INPUT_SEL   {A6, 650,  800, &select_next_input}
#define BTN_TOGGLE_MUTE {A6, 820, 1023, &toggle_mute}
#else
// Pin, callback()
#define BTN_INPUT_SEL   {4, &select_next_input}
#define BTN_TOGGLE_MUTE {3, &toggle_mute}

#endif

// Audio outpout relay trigger pin (mute)
#ifdef REV1_1
#define OUTPUT_PIN 2
#else
#define OUTPUT_PIN 8
#endif

#ifdef REV1_1
#else
#endif

// Audio input relay output trigger pins and descriptions
// of the inputs for the LCD
//
// Inputs 1...N
#ifdef REV1_1
#define INPUT_RELAY_PINS { 3, 4, 5, 6 };
#define INPUT_DESCRIPTIONS { "Desktop PC", "Laptop", "AUX", "Bluetooth" }
#else
#define INPUT_RELAY_PINS { 5, 6, 7 };
#define INPUT_DESCRIPTIONS { "Desktop PC", "Laptop", "AUX" }
#endif

// Digital output trigger pin for BT module power
#ifdef REV1_1
#define BT_PWR_PIN 7
#endif

#include <Arduino.h>
#include <SPI.h>
#include <EEPROM.h>
#include <Wire.h>
#include <FaBoLCD_PCF8574.h>

FaBoLCD_PCF8574 lcd(0x27);

// Required to forward-declare these for use with 'buttons'
void select_next_input();
void toggle_mute();

struct data_store {
  byte selected_input = 0;
  bool muted = false;
};

/**
 * Input button class to configure and detect button presses
 * via an analog input pin and a range of min/max analog reading
 * - OR -
 * a single digital pin, HIGH being pressed.
 */
struct button {
  bool pressed = false;
  bool muted = false;

  unsigned long m_debounce_millis;
  
  int pin;
  int a_val_min;
  int a_val_max;
  int m_debounce_delay;
  void (*fn_pressed)() = NULL;
  void (*fn_released)() = NULL;
  bool analog_mode = false;

  // Analog resistor-based input buttons
  button(int input_pin, int aval_min, int aval_max, void(*fnPressed)(void) = NULL, void(*fnReleased)(void) = NULL, int debounce_delay = 25):
    pin(input_pin), fn_pressed(fnPressed), fn_released(fnReleased), a_val_min(aval_min), a_val_max(aval_max), analog_mode(true), m_debounce_delay(debounce_delay)
  {}

  // Digital input buttons
  button(int input_pin, void(*fnPressed)(void) = NULL, void(*fnReleased)(void) = NULL, int debounce_delay = 25):
    pin(input_pin), fn_pressed(fnPressed), fn_released(fnReleased), m_debounce_delay(debounce_delay)
  {}

  void check() {
    bool btn_state;

    if (analog_mode) {
      auto aval = analogRead(pin);
      btn_state = aval >= a_val_min && aval <= a_val_max;
    } else {
      btn_state = digitalRead(pin) == HIGH;
    }

    if (btn_state && !pressed) {
      state_changed(true);
    } else if (!btn_state && pressed) {
      state_changed(false);
    }
  }

  void state_changed(bool state) {
    if (m_debounce_millis > 0 && m_debounce_millis + m_debounce_delay > millis()) {
      m_debounce_millis = 0;
#ifdef DEBUG
      Serial.print("DEBOUNCE: Button on pin "); 
      Serial.println(pin); 
#endif
      return;
    }

    m_debounce_millis = millis();
    pressed = state;

    if (pressed) {
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
    Serial.print("PRESS: ");
    Serial.print("Button on pin "); 
    Serial.println(pin); 
#endif
  }
};

// Define the physical input buttons
button buttons[] = {
  BTN_INPUT_SEL,
  BTN_TOGGLE_MUTE
};

const char *inputDescriptions[] = INPUT_DESCRIPTIONS;
const int inputRelays[] = INPUT_RELAY_PINS;
bool bluetoothPowerState = false;
data_store settings;

unsigned long last_active_millis = 0;
bool is_active = false;

void setup() {
  pinMode(POT_CS_PIN, OUTPUT);
  digitalWrite(POT_CS_PIN, HIGH);

  for (auto i = 0; i < NUMBER_OF_INPUTS; i++) {
    pinMode(inputRelays[i], OUTPUT);
  }

  pinMode(OUTPUT_PIN, OUTPUT);

#ifdef BT_PWR_PIN
  pinMode(BT_PWR_PIN, OUTPUT);
#endif

#ifdef DEBUG
  Serial.begin(115200);
#endif

  SPI.begin();

  // Set up the LCD's number of columns and rows
  lcd.begin(16, 2);
  lcd.print(" Audio Switcher ");

  read_eeprom();
  switch_input(settings.selected_input);

  has_activity();

  delay(1000);
}

/**
 * Set device active
 */
void has_activity() {
  update_display();

#ifdef DEBUG
  for (auto i = 0; i < NUMBER_OF_INPUTS; i++) {
    Serial.print("Input ");
    Serial.print(i+1);
    Serial.print(" = ");

    if (digitalRead(inputRelays[i])) {
      Serial.println("On");
    } else {
      Serial.println("Off");
    }
  }

  Serial.print("Output = ");

  if (digitalRead(OUTPUT_PIN)) {
    Serial.println("On");
  } else {
    Serial.println("Off");
  }

#endif

  if (!is_active) {
    fade_backlight(1);
  }

  is_active = true;
  last_active_millis = millis();
}

/**
 * Set device inactive
 */
void inactive() {
  is_active = false;
  fade_backlight(-1);
}

/**
 * Fade backlight of LCD in or out
 * 'step' > 0 = fade in
 * 'step' < 0 = fade out
 */
void fade_backlight(int step) {
  const auto neg = step < 1 ? 127 : 0;

  digitalWrite(POT_CS_PIN, LOW);
  // Values below 64 isn't very perceptive
  for (auto i = 64; i < 127; i++) {
    const auto brightness = 127 - abs(neg - i);
    SPI.transfer(0x00);
    SPI.transfer(brightness);
    delay(4);
  }
  digitalWrite(POT_CS_PIN, HIGH);
}

/**
 * Update the LCD display
 */
void update_display() {
  char buf[17];

  auto num_chars = sprintf(buf, "%d: %s", settings.selected_input + 1, inputDescriptions[settings.selected_input]);
  // Pad with spaces to clear previous line of longer strings
  memset(buf + num_chars, ' ', sizeof(buf) - num_chars);

  lcd.setCursor(0, 0);
  lcd.print(buf);

  lcd.setCursor(0, 1);

  if (settings.muted) {
    lcd.print("  --- Mute ---  ");
  } else {
    lcd.print("                ");
  }
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

  if (settings.selected_input >= NUMBER_OF_INPUTS) {
    settings.selected_input = 0;
  }
}

/**
 * Switch input from 'settings.selected_input' to 'input'
 * Will also turn power on/off to the BT Audio Module when
 * relevant.
 */
void switch_input(int input) {
  // Disable output
  if (!settings.muted) {
    setMute(true);
  }

  digitalWrite(inputRelays[settings.selected_input], LOW);
  digitalWrite(inputRelays[input], HIGH);

#ifdef BT_PWR_PIN
  // BT input is always the last input
  if (input == (NUMBER_OF_INPUTS - 1)) {
    digitalWrite(BT_PWR_PIN, HIGH);
    bluetoothPowerState = true;
  } else if (bluetoothPowerState) {
    digitalWrite(BT_PWR_PIN, LOW);
    bluetoothPowerState = false;
  }
#endif

  if (!settings.muted) {
    // Re-enable output
    //delay(25);
    setMute(false);
  }

  settings.selected_input = input;
  save_eeprom();
}

/**
 * Set muted state
 */
void setMute(bool muted) {
  digitalWrite(OUTPUT_PIN, muted ? LOW : HIGH);
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
 * Select next input, cycling through the available inputs
 */
void select_next_input() {
  auto new_input = settings.selected_input + 1;

  if (new_input == NUMBER_OF_INPUTS) {
    new_input = 0;
  }

  switch_input(new_input);
  has_activity();
}

/**
 * Check buttons for events
 */
void check_buttons() {
  for (auto i = 0; i < (int)(sizeof(buttons) / sizeof(button)); i++) {
    buttons[i].check();
  }
}

void loop() {
  if (is_active && (millis() - last_active_millis) > INACTIVITY_TIMEOUT) {
    inactive();
  }

  check_buttons();
}
