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
const int THRESHOLD = 512;      // ← CHANGED: Works with INPUT_PULLUP
const int DEBOUNCE_DELAY = 10;  // milliseconds

// ============================================
// CALIBRATION MODE
// ============================================
const bool CALIBRATION_MODE = true;  // SET TO false AFTER FINDING THRESHOLD

// ============================================
// SENSOR STATE TRACKING
// ============================================
bool lastState[30] = {false};
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
  Serial.begin(9600);
  
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
  // Scan all 30 sensors
  for (int sensorIndex = 0; sensorIndex < 30; sensorIndex++) {
    
    // Read the sensor value from the correct multiplexer
    int sensorValue = readSensor(sensorIndex);
    
    // ===== CALIBRATION MODE =====
    if (CALIBRATION_MODE) {
      Serial.print("Sensor ");
      Serial.print(sensorIndex);
      Serial.print(": ");
      Serial.println(sensorValue);
      delay(100);
      continue;
    }
    
    // ===== NORMAL MODE =====
    bool currentState = (sensorValue < THRESHOLD);
    
    if (currentState != lastState[sensorIndex]) {
      lastDebounceTime[sensorIndex] = millis();
    }
    
    if ((millis() - lastDebounceTime[sensorIndex]) > DEBOUNCE_DELAY) {
      if (currentState != lastState[sensorIndex]) {
        lastState[sensorIndex] = currentState;
        
        if (currentState) {
          // Pedal pressed - Send MIDI Note On via Serial
          sendMIDI(0x90, BASE_NOTE + sensorIndex, 100);
          Serial.print("Pedal ");
          Serial.print(sensorIndex);
          Serial.print(" - Note ON: ");
          Serial.println(BASE_NOTE + sensorIndex);
        } else {
          // Pedal released - Send MIDI Note Off via Serial
          sendMIDI(0x80, BASE_NOTE + sensorIndex, 0);
          Serial.print("Pedal ");
          Serial.print(sensorIndex);
          Serial.print(" - Note OFF: ");
          Serial.println(BASE_NOTE + sensorIndex);
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
}

// ============================================
// SEND MIDI MESSAGE OVER SERIAL (for Hairless MIDI)
// ============================================
void sendMIDI(byte command, byte pitch, byte velocity) {
  // MIDI messages are 3 bytes: Status, Data 1, Data 2
  // Status byte = command + (channel - 1)
  byte status = command | ((MIDI_CHANNEL - 1) & 0x0F);
  
  Serial.write(status);
  Serial.write(pitch);
  Serial.write(velocity);
}