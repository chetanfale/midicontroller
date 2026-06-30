 
// PIN CONFIGURATION
 
const int SHIFT_PIN          = 4;
const int GENERAL_TRIG_PIN   = 2;
const int SHIFT_LED_PIN      = 5;

const int BANK_BUTTON_PINS[] = {12, 10, 8, 6};
const int BANK_LED_PINS[]    = {13, 11, 9, 7};
const int NUM_BANKS          = 4;

// MIDI
const byte MIDI_CHAN     = 0;
const byte CMD_NOTE_ON  = 0x90 | MIDI_CHAN;
const byte CMD_NOTE_OFF = 0x80 | MIDI_CHAN;
const byte CMD_CC       = 0xB0 | MIDI_CHAN;

// Timing
const unsigned long DEBOUNCE_MS = 15;
const unsigned long PULSE_MS    = 15;

// MIDI mapping bases
const byte NOTE_BANK_BASE = 36;
const byte NOTE_GEN_BASE  = 60;
const byte CC_BASE        = 20;

// Pot hysteresis
const int MARGIN = 4;

 
// POTENTIOMETER
 
struct Potentiometer {
  int pin;
  int lastStableValue;
  int centerRaw;
};

Potentiometer pots[] = {
  {A0, -1, 0},
  {A1, -1, 0}, // uncomment when connected
  {A2, -1, 0}, // uncomment when connected
  {A3, -1, 0}  // uncomment when connected
};
const int NUM_POTS = sizeof(pots) / sizeof(pots[0]);

 
// STATE
 
int activeBank = 0;

// --- Shift ---
bool shiftState             = false;
int  lastShiftRaw           = HIGH;
unsigned long shiftDebounce = 0;
bool lastShiftState         = false;

// --- General pad ---
bool genState               = false;
int  lastGenRaw             = HIGH;
unsigned long genDebounce   = 0;
byte activeGenNote          = 0;

// --- Bank buttons ---
bool bankState[NUM_BANKS]             = {};
int  lastBankRaw[NUM_BANKS]           = {HIGH, HIGH, HIGH, HIGH};
unsigned long bankDebounce[NUM_BANKS] = {};
byte bankActiveNote[NUM_BANKS]        = {};

// --- Shift LED pulse ---
int  pulseValue     = 0;
int  pulseDir       = 5;
unsigned long lastPulseTime = 0;

 
// HELPERS
 
void sendMIDI(byte cmd, byte d1, byte d2) {
  Serial.write(cmd);
  Serial.write(d1);
  Serial.write(d2);
}

void updateBankLEDs() {
  for (int i = 0; i < NUM_BANKS; i++) {
    digitalWrite(BANK_LED_PINS[i], (i == activeBank) ? HIGH : LOW);
  }
}

bool readButton(int pin, int &lastRaw, unsigned long &debounceTime,
                bool &stableState, unsigned long now) {
  int reading = digitalRead(pin);
  if (reading != lastRaw) {
    debounceTime = now;
    lastRaw = reading;
  }
  if ((now - debounceTime) > DEBOUNCE_MS) {
    bool pressed = (reading == LOW);
    if (pressed != stableState) {
      stableState = pressed;
      return true;
    }
  }
  return false;
}

 
// SETUP
 
void setup() {
  Serial.begin(115200);

  pinMode(SHIFT_PIN, INPUT_PULLUP);
  pinMode(GENERAL_TRIG_PIN, INPUT_PULLUP);
  pinMode(SHIFT_LED_PIN, OUTPUT);

  for (int i = 0; i < NUM_BANKS; i++) {
    pinMode(BANK_BUTTON_PINS[i], INPUT_PULLUP);
    pinMode(BANK_LED_PINS[i], OUTPUT);
    digitalWrite(BANK_LED_PINS[i], LOW);
  }

  updateBankLEDs();

  // Cold-boot calibration
  for (int i = 0; i < NUM_POTS; i++) {
    int raw = analogRead(pots[i].pin);
    pots[i].centerRaw       = raw;
    pots[i].lastStableValue = constrain(map(raw, 0, 1023, 0, 128), 0, 127);
  }
}

void loop() {
  unsigned long now = millis();

   
  // 1. POTENTIOMETERS
   
  for (int i = 0; i < NUM_POTS; i++) {
    int rawCurrent    = analogRead(pots[i].pin);
    int currentScaled = constrain(map(rawCurrent, 0, 1023, 0, 128), 0, 127);

    if (abs(rawCurrent - pots[i].centerRaw) > MARGIN) {
      if (currentScaled != pots[i].lastStableValue) {
        pots[i].lastStableValue = currentScaled;
        pots[i].centerRaw       = rawCurrent;

        byte cc = CC_BASE + (activeBank * NUM_POTS) + i;
        sendMIDI(CMD_CC, cc, currentScaled);
      }
    }
  }

   
  // 2. SHIFT BUTTON
   
  readButton(SHIFT_PIN, lastShiftRaw, shiftDebounce, shiftState, now);

   
  // 3. BANK BUTTONS
   
  for (int i = 0; i < NUM_BANKS; i++) {
    if (!readButton(BANK_BUTTON_PINS[i], lastBankRaw[i], bankDebounce[i], bankState[i], now))
      continue;

    if (bankState[i]) {
      if (shiftState) {
        bankActiveNote[i] = NOTE_BANK_BASE + (activeBank * NUM_BANKS) + i;
        sendMIDI(CMD_NOTE_ON, bankActiveNote[i], 127);
      } else {
        activeBank = i;
        updateBankLEDs();
      }
    } else {
      if (bankActiveNote[i] != 0) {
        sendMIDI(CMD_NOTE_OFF, bankActiveNote[i], 0);
        bankActiveNote[i] = 0;
      }
    }
  }

   
  // 4. GENERAL PAD
   
  if (readButton(GENERAL_TRIG_PIN, lastGenRaw, genDebounce, genState, now)) {
    if (genState) {
      activeGenNote = shiftState ? (NOTE_GEN_BASE + 1) : NOTE_GEN_BASE;
      sendMIDI(CMD_NOTE_ON, activeGenNote, 127);
    } else {
      if (activeGenNote != 0) {
        sendMIDI(CMD_NOTE_OFF, activeGenNote, 0);
        activeGenNote = 0;
      }
    }
  }

   
  // 5. SHIFT LED — pulse on hold, clean reset on release
   
  if (shiftState) {
    if (now - lastPulseTime >= PULSE_MS) {
      lastPulseTime = now;
      pulseValue   += pulseDir;
      if (pulseValue <= 0 || pulseValue >= 255) {
        pulseDir   = -pulseDir;
        pulseValue  = constrain(pulseValue, 0, 255);
      }
      analogWrite(SHIFT_LED_PIN, pulseValue);
    }
  } else {
    if (lastShiftState) {
      analogWrite(SHIFT_LED_PIN, 0);
      pulseValue = 0;
      pulseDir   = 5;
    }
  }
  lastShiftState = shiftState;
}