#include <SPI.h>
#include <EEPROM.h>
#include <OneButton.h>

// mode0: 8 Gates, 8 CV Pitch LEDS: 000
// mode1: 4 Gates (1-4), 4 Clock Gates (5-8 - 1/4, 1/8, 1/16, 1/2), 4 CV Pitch (1-4), 4 CV Mod (5-8) LEDS: 100 
// mode2: 4 Gates (1-4), 4 Clock Gates (5-8 - 1/4, 1/8, 1/16, 1/2), 4 CV Pitch (1-4), 4 CV Vel (5-8) LEDS: 110 
// mode3: 6 Clock Gates (1-6 - 1/4, 1/8, 1/16, 1/2), 7 Start Trig, 8 Stop Trig, 8 CV Mod LEDS: 111
// mode4: 4 Gates (1-4), 3 Clock Gates (5-7 - 1/4, 1/8, 1/2), 8 Start Trig, 4 CV Pitch (1-4), 4 CV Mod (5-8) LEDS: 001
// mode5: 4 Gates (1-4), 3 Clock Gates (5-7 - 1/4, 1/8, 1/2), 8 Start Trig, 4 CV Pitch (1-4), 4 CV Vel (5-8) LEDS: 011

int mode;

// Setup a new OneButton 
int sw = 8;
OneButton btn(sw, true);

const int pitchbend_value_positive = 1200; 
const int pitchbend_value_negative = -1200;
const float offset_pitch = 60;


const int LED = 13;

int cs_pin =  21;
//int gate_pin = 7;
int pitchbend_value[8];
int pitch_values[8];
int cs9 = 2;
byte shadow_gate;

byte clock_tick;
byte clock_value;
byte play_flag;
byte play_tick;

float voltage_range = 4.024 * 1200;

//////////////////

//int cs_pin =  21;
//int gate_pin[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
//int pitchbend_value[8];

int yellow_led = 5;
int yellow_led2 = 16;
int yellow_led3 = 17;

//byte clock_tick; 
//byte clock_value; 
//byte play_flag; 
//byte play_tick;

void setup() {
  SPI.begin();
  pinMode(cs9, OUTPUT);
  pinMode(sw, INPUT_PULLUP);

  btn.attachClick(clickBtn);
  btn.attachLongPressStart(longPressStartBtn);
  btn.attachLongPressStop(longPressStopBtn);
  btn.attachDuringLongPress(longPressDurationBtn);
  
  for (int i = 0; i < 4; i ++) {
    pinMode(cs_pin - i, OUTPUT);
    //pinMode(gate_pin[i] - 1, OUTPUT);
    digitalWriteFast(cs_pin - i, HIGH);
    delay(50);
  }

  for (int i = 0; i < 8; i ++) {
    writeGate(i, HIGH);
    delay(50);
  }

  delay(100);
  
  for (int i = 0; i < 8; i ++) {
    writeGate(i, LOW);
    delay(50);
  }
//pinMode(gate_pin[0], OUTPUT);
  delay(100);

  

  usbMIDI.setHandleNoteOn(OnNoteOn);
  usbMIDI.setHandleNoteOff(OnNoteOff);
  usbMIDI.setHandlePitchChange(OnPitchChange);
  usbMIDI.setHandleControlChange(OnControlChange);
  usbMIDI.setHandleRealTimeSystem(OnClock); 

  
  mode = EEPROM.read(0);

  pinMode(yellow_led, OUTPUT);
  pinMode(yellow_led2, OUTPUT);
  pinMode(yellow_led3, OUTPUT);

}

void loop() {
  btn.tick();
  displayMode();
  usbMIDI.read();
}

void OnNoteOn(byte channel, byte pitch, byte velocity) {
  
  if (mode == 0) {
    if (channel < 9) {
        pitch_values[channel - 1] = pitch;
        writeDAC(cs_pin - ((channel - 1) / 2), (channel) & 1, constrain(map((pitch - offset_pitch) * 100.0 + pitchbend_value[channel - 1], 0.0, voltage_range, 0.0, 4095.0), 0.0, 4095.0));
        if (velocity > 0) {
          writeGate(channel - 1, HIGH);
        }
        else {
          writeGate(channel - 1, LOW);
        }
    }
  }

  else if (mode == 1 || mode == 2 || mode == 4 || mode == 5) {
    if (channel < 5) {

      pitch_values[channel - 1] = pitch;
        writeDAC(cs_pin - ((channel - 1) / 2), (channel) & 1, constrain(map((pitch - offset_pitch) * 100.0 + pitchbend_value[channel - 1], 0.0, voltage_range, 0.0, 4095.0), 0.0, 4095.0));
        if (velocity > 0) {
          writeGate(channel - 1, HIGH);
        }
        else {
          writeGate(channel - 1, LOW);
        }
    }
  }
  
}

void OnNoteOff(byte channel, byte pitch, byte velocity) {

  if (mode == 0) {
    if (channel < 9) {
      writeGate(channel - 1, LOW);
    }
  }

  else if (mode == 1 || mode == 2 || mode == 4 || mode == 5) {
    if (channel < 5) {
      writeGate(channel - 1, LOW);
    }
  }

  
}

void OnPitchChange (byte channel, int pitch_change) {

  if (mode == 0) {
    if (channel < 9) {
      pitchbend_value[channel - 1] = map(pitch_change, 0, 16383, pitchbend_value_negative, pitchbend_value_positive);
      writeDAC(cs_pin - channel + 1, 1, constrain(map((pitch_values[channel - 1] - offset_pitch) * 100.0 + pitchbend_value[channel - 1], 0.0, voltage_range, 0.0, 4095.0), 0.0, 4095.0));
    }
  }

  if (mode == 2) {
    //writeDAC(cs_pin - ((channel - 1) / 2), (channel - 1) & 1, map(pitch_change, 0, 16383, 0, 4095));
  }
}

void OnControlChange (byte channel, byte control, byte value) {
  if (mode == 1 || mode == 4) {
    if (channel > 4 && channel < 9) {
      if (control == 1) {
        writeDAC(cs_pin - ((channel - 1) / 2), (channel) & 1, map(value, 0, 127, 0, 4095));
      }
    }
  }

   if (mode == 3) {
    if (channel < 9) {
      if (control == 1) {
        writeDAC(cs_pin - ((channel - 1) / 2), (channel) & 1, map(value, 0, 127, 0, 4095));
      }
    }
  }

}

void OnClock(byte clockbyte) {
  if (mode == 1 || mode == 2) {
    if(clockbyte == 0xf8 && play_flag == 1) {
      ////digitalWriteFast(10, 1 - bitRead(clock_tick, 0)); // 12 ppqn
      writeGate(4, 1 - bitRead(clock_tick / 12, 0)); // quarter note 
      writeGate(5, 1 - bitRead(clock_tick / 6, 0)); // eigths note
      writeGate(6, 1 - bitRead(clock_tick / 3, 0)); // sixteenths note
      writeGate(7, 1 - bitRead(clock_tick / 24, 0)); // half note
      ////digitalWriteFast(gate_pin[5], 1 - bitRead(clock_tick / 48, 0)); // whole note
      ////digitalWriteFast(gate_pin[5], 1 - bitRead(clock_tick / 2, 0)); //  eigths t note
      ////digitalWriteFast(gate_pin[5], 1 - bitRead(clock_tick / 4, 0)); // quarter t note
      
      clock_tick ++; 
  
      if(clock_tick == 48) {
        clock_tick = 0; 
      }
  
      if(clock_tick == 3 && play_tick == 1) {
        play_tick = 0;
        //digitalWriteFast(11, LOW);
      }
    }
    
    if(clockbyte == 0xfa || clockbyte == 0xfb) { // start or continue bytes
      play_flag = 1; 
      play_tick = 1;
      clock_value = 0; 
      clock_tick = 0; 
      //digitalWriteFast(11, HIGH); 
    }
  
    if(clockbyte == 0xfc) { // stop byte
      ////digitalWriteFast(11, LOW);
      writeGate(4, LOW);
      writeGate(5, LOW);
      writeGate(6, LOW);
      writeGate(7, LOW); 

      play_flag = 0; 
    }
  }
  if (mode == 4) {
    if(clockbyte == 0xf8 && play_flag == 1) {
      //digitalWriteFast(0, 1 - bitRead(clock_tick, 0)); // 12 ppqn
      //digitalWriteFast(gate_pin[0], 1 - bitRead(clock_tick / 2, 0)); // sixteenths t note 
      //digitalWriteFast(gate_pin[1], 1 - bitRead(clock_tick / 3, 0)); // sixteenths note
      //digitalWriteFast(gate_pin[2], 1 - bitRead(clock_tick / 4, 0)); // eigths t note
      //digitalWriteFast(gate_pin[3], 1 - bitRead(clock_tick / 6, 0)); // eigths note
      //digitalWriteFast(gate_pin[4], 1 - bitRead(clock_tick / 8, 0)); // quarter t note
      //digitalWriteFast(gate_pin[5], 1 - bitRead(clock_tick / 12, 0)); // quarter note
      //digitalWriteFast(gate_pin[6], 1 - bitRead(clock_tick / 24, 0)); // half note
      
      clock_tick ++; 
  
      if(clock_tick == 48) {
        clock_tick = 0; 
      }
  
      if(clock_tick == 3 && play_tick == 1) {
        play_tick = 0;
        //digitalWriteFast(11, LOW);
      }
    }
    
    if(clockbyte == 0xfa || clockbyte == 0xfb) { // start or continue bytes
      play_flag = 1; 
      play_tick = 1;
      clock_value = 0; 
      clock_tick = 0; 
      //digitalWriteFast(11, HIGH); 
    }
  
    if(clockbyte == 0xfc) { // stop byte
      ////digitalWriteFast(11, LOW);
      //digitalWriteFast(gate_pin[-1], LOW);
      //digitalWriteFast(gate_pin[0], LOW);
      //digitalWriteFast(gate_pin[1], LOW);
      //digitalWriteFast(gate_pin[2], LOW);
      //digitalWriteFast(gate_pin[3], LOW);
      //digitalWriteFast(gate_pin[4], LOW);
      //digitalWriteFast(gate_pin[5], LOW);
      //digitalWriteFast(gate_pin[6], LOW); 

      play_flag = 0; 
    }
  }
}

void writeDAC (int cs, int dac, int val) {
  digitalWriteFast(cs, LOW);
  dac = dac & 1;
  val = val & 4095;
  SPI.transfer(dac << 7 | 0 << 5 | 1 << 4 | val >> 8);
  SPI.transfer(val & 255);
  digitalWriteFast(cs, HIGH);
}

void writeGate(byte bit_number, byte bit_value) {
  bitWrite(shadow_gate, bit_number, bit_value);
  digitalWriteFast(cs9, LOW);
  SPI.transfer(shadow_gate);
  digitalWriteFast(cs9, HIGH);
}


void clickBtn() {
  Serial.println("clickBtn");
  mode++;
  if(mode == 6){mode=0;}
}

void longPressStartBtn() {
  Serial.println("longPressStartBtn");
}

void longPressStopBtn() {
  Serial.println("longPressStopBtn");
  EEPROM.write(0, mode);
}

void longPressDurationBtn() {
  Serial.println("longPressDurationBtn");


  digitalWrite(yellow_led, HIGH);
  digitalWrite(yellow_led2, HIGH);
  digitalWrite(yellow_led3, HIGH);
  delay(100);
  digitalWrite(yellow_led, LOW);
  digitalWrite(yellow_led2, LOW);
  digitalWrite(yellow_led3, LOW);
  delay(100);
}

void displayMode() {
  if (mode == 0) {
    digitalWrite(yellow_led, LOW);
    digitalWrite(yellow_led2, LOW);
    digitalWrite(yellow_led3, LOW);
  } else if(mode == 1) {
    digitalWrite(yellow_led, HIGH);
    digitalWrite(yellow_led2, LOW);
    digitalWrite(yellow_led3, LOW);
  } else if(mode == 2) {
    digitalWrite(yellow_led, HIGH);
    digitalWrite(yellow_led2, HIGH);
    digitalWrite(yellow_led3, LOW);
  } else if(mode == 3) {
    digitalWrite(yellow_led, HIGH);
    digitalWrite(yellow_led2, HIGH);
    digitalWrite(yellow_led3, HIGH);
  } else if(mode == 4) {
    digitalWrite(yellow_led, LOW);
    digitalWrite(yellow_led2, LOW);
    digitalWrite(yellow_led3, HIGH);
  } else if(mode == 5) {
    digitalWrite(yellow_led, LOW);
    digitalWrite(yellow_led2, HIGH);
    digitalWrite(yellow_led3, HIGH);
  }  
}
