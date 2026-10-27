/*
  Church Organ Pedal MIDI Controller
  - 30 Hall effect sensors
  - 2 x 16-channel multiplexers (CD74HC4067)
  - Arduino Nano
  - Uses Serial communication for Hairless MIDI bridge
  - MIDI Channel 1 (standard for bass pedals)
  - Base note: C2 = 36 (adjustable)
*/

// ============================================
// HARDWARE PINS
// ============================================
// MUX Control Pins (D4 - D7)
const int S0 = 4;
const int S1 = 5;
const int S2 = 6;
const int S3 = 7;

// MUX Analog Input Pins
const int MUX1_SIG = A0;  // Sensors 0-15
const int MUX2_SIG = A1;  // Sensors 16-29

// ============================================
// MIDI SETTINGS (Hauptwerk Standard)
// ============================================
const int MIDI_CHANNEL = 1;     // MIDI Channel 1 (Standard for pedals)
const int BASE_NOTE = 36;       // 36 = C2 (lowest pedal note)

// ============================================
// SENSOR SETTINGS (CALIBRATE THESE!)
// ============================================
const int THRESHOLD = 1;      // ← CHANGED: Works with INPUT_PULLUP
const int DEBOUNCE_DELAY = 10;  // milliseconds
// ============================================
// MIDI VELOCITY
// ============================================
const int midi_velocity = 100; // 0-127

// ============================================
// CALIBRATION MODE
// ============================================
const bool CALIBRATION_MODE = false;  // SET TO false AFTER FINDING THRESHOLD
const bool DEBUG_MODE = false; // SET to false before connecting to synthesizer
// ============================================
// SENSOR STATE TRACKING
// ============================================
bool lastState[30]        = {false};   // debounced, confirmed state
bool rawState[30]         = {false};   // last raw read
unsigned long lastDebounceTime[30] = {0};

// ============================================
// SETUP
// ============================================
void setup() {
  // Configure MUX control pins
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  
  // Configure analog input pins with PULL-UPS (FIX for open-collector sensors)
  pinMode(MUX1_SIG, INPUT_PULLUP);  // ← CHANGED
  pinMode(MUX2_SIG, INPUT_PULLUP);  // ← CHANGED
  
  // Start serial for calibration and MIDI bridge
  //Serial.begin(9600);
  Serial.begin(31250);

  
  if (CALIBRATION_MODE) {
    Serial.println("╔══════════════════════════════════════════════════════════╗");
    Serial.println("║         HALL SENSOR CALIBRATION MODE                     ║");
    Serial.println("╚══════════════════════════════════════════════════════════╝");
    Serial.println();
    delay(3000);
  }
}

// ============================================
// MAIN LOOP
// ============================================
void loop() {
  for (int sensorIndex = 0; sensorIndex < 30; sensorIndex++) {

    int  sensorValue  = readSensor(sensorIndex);
    bool currentState = (sensorValue == 1);

    // If the raw reading changed, reset the debounce timer
    if (currentState != rawState[sensorIndex]) {
      lastDebounceTime[sensorIndex] = millis();
      rawState[sensorIndex]         = currentState;
    }

    // Only accept the new state once it has been stable long enough
    if ((millis() - lastDebounceTime[sensorIndex]) > DEBOUNCE_DELAY) {

      // Edge detection: only act when the *confirmed* state changes
      if (currentState != lastState[sensorIndex]) {
        lastState[sensorIndex] = currentState;

        if (currentState) {
          sendMIDI(0x90, BASE_NOTE + sensorIndex, midi_velocity);   // Note On
          if (DEBUG_MODE) {
            Serial.print("Pedal "); Serial.print(sensorIndex);
            Serial.print(" - Note ON: "); Serial.println(BASE_NOTE + sensorIndex);
          }
        } else {
          sendMIDI(0x80, BASE_NOTE + sensorIndex, 0);    // Note Off
          if (DEBUG_MODE) {
            Serial.print("Pedal "); Serial.print(sensorIndex);
            Serial.print(" - Note OFF: "); Serial.println(BASE_NOTE + sensorIndex);
          }
        }
      }
    }
  }

  if (!CALIBRATION_MODE) {
    delay(5);
  }
}

// ============================================
// READ SENSOR FROM CORRECT MULTIPLEXER
// ============================================
int readSensor(int sensorIndex) {
  int channel;
  int rawValue;
  
  if (sensorIndex < 16) {
    channel = sensorIndex;
    setMUXChannel(channel);
    delayMicroseconds(10);
    //rawValue = analogRead(MUX1_SIG);
    rawValue = digitalRead(MUX1_SIG);
  } else {
    channel = sensorIndex - 16;
    setMUXChannel(channel);
    delayMicroseconds(10);
    //rawValue = analogRead(MUX2_SIG);
    rawValue = digitalRead(MUX2_SIG);
  }
  
  return rawValue;
}

// ============================================
// SET MULTIPLEXER CHANNEL (0-15)
// ============================================
void setMUXChannel(int channel) {
  digitalWrite(S0, channel & 1);
  digitalWrite(S1, (channel >> 1) & 1);
  digitalWrite(S2, (channel >> 2) & 1);
  digitalWrite(S3, (channel >> 3) & 1);

  if (DEBUG_MODE == true){
    Serial.print("channel: ");
    Serial.println(channel);
    Serial.print("S0: ");
    Serial.println(channel & 1);
    Serial.print("S1: ");
    Serial.println((channel >> 1) & 1);
    Serial.print("S2: ");
    Serial.println((channel >> 2) & 1);
    Serial.print("S3: ");
    Serial.println((channel >> 3) & 1);
  }
}

// ============================================
// SEND MIDI MESSAGE OVER SERIAL (for Hairless MIDI)
// ============================================
void sendMIDI(byte command, byte pitch, byte velocity) {
  // MIDI messages are 3 bytes: Status, Data 1, Data 2
  // Status byte = command + (channel - 1)
  byte status = command | ((MIDI_CHANNEL - 1) & 0x0F);
  if (DEBUG_MODE == true){
    Serial.print("MIDI message: ");
    Serial.println(status);
    Serial.print("Pitch: ");
    Serial.println(pitch);
    Serial.print("Velocity:");
    Serial.println(velocity);
  }
  Serial.write(status);
  Serial.write(pitch);
  Serial.write(velocity);
}