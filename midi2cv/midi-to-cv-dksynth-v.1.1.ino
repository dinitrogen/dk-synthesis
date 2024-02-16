// Version 1.1 (updated 02/16/2024)
// Inspired by the midisplit_2000 code by Juha Kivekas (https://github.com/juhakivekas/midisplitter_2000) and
// the MIDI to CV code by Afrorack (https://www.youtube.com/watch?v=UYeJiPWZZNQ) and
// this LMNC thread: https://lookmumnocomputer.discourse.group/t/polyphonic-midi-to-cv/891/3
// MCP4922 data sheet: https://ww1.microchip.com/downloads/en/devicedoc/22250a.pdf

#include <SPI.h>
#include <MIDI.h>

MIDI_CREATE_DEFAULT_INSTANCE(); // Use default RX and TX pins for MIDI

#define CHANNELS 4  // Define the number of VCO channels and store them in an array
volatile byte pitches[CHANNELS];

int chipSelect = 9;  // Arduino pins that connects to the DACs (pin 3) - channels 1 and 2
int chipSelect2 = 10; // channels 3 and 4
float refVoltage = 5.03; // Reference voltage of the regulator. DAC output cannot be higher than reference voltage

int switchPin = 3;  // Switch for mono vs polyphonic mode
int modePin = 4; // Switches mono unison on/off or two different polyphonic logic modes (tbd)
int monophonicNote;
int monophonicDacInput;

// (Not currently using the following function but adopted these to the note on/off functions below)
//void splitNoteOnStack(byte channel, byte pitch, byte velocity) {
//  bool found = false;
//  int i;
//  // find a free/silent voice
//  for (i=0; i<CHANNELS; i++) {
//    if (pitches[i] == 0) {
//      found = true;
//      break;
//    }
//  }
//  // set channel note and send the mono note
//  if (found) {
//    pitches[i] = pitch;
//    MIDI.sendNoteOn(pitch, velocity, i+1);
//  }
//  // if there is no free channel found, events are dropped  
//}

// (Not currently using the following function but adopted these to the note on/off functions below)
//void splitNoteOff(byte channel, byte pitch, byte velocity) {
//  bool found = false;
//  int i;
//  // find voice with the incoming pitch
//  for (i=0; i<CHANNELS; i++) {
//    if (pitches[i] == pitch) {
//      found = true;
//      break;
//    }
//  }
//  // set channel note off
//  if (found) {
//    pitches[i] = 0;
//    MIDI.sendNoteOff(pitch, velocity, i+1);
//  }
//  // if the pitch wasn't found on any channel, then events are dropped
//}


void setup() {
  // set all channels to free/silent
  // memset(pitches, 0, CHANNELS);
  int j;
  for (j = 0; j < CHANNELS; j++) {
    pitches[j] = 0;
  }

  pinMode(chipSelect, OUTPUT);  // DAC 1
  digitalWrite(chipSelect, HIGH);

  pinMode(chipSelect2, OUTPUT); // DAC 2
  digitalWrite(chipSelect2, HIGH);
  
  pinMode(5, OUTPUT); //Gate 1
  pinMode(6, OUTPUT); //Gate 2
  pinMode(7, OUTPUT); // Gate 3
  pinMode(8, OUTPUT); // Gate 4

  pinMode(switchPin, INPUT); //Switch between "monophonic" (LOW) and "polyphonic" (HIGH)
  pinMode(modePin, INPUT);  //Switch between "solo/stack" (LOW) and "unison/cycle" (HIGH)
  
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);         
  SPI.setDataMode(SPI_MODE0);

  MIDI.begin(MIDI_CHANNEL_OMNI);  // Listen to all midi channels
  //MIDI.setHandleNoteOff(splitNoteOff);
  
  // These are in the midisplit code but I don't think are needed
  // MIDI.begin(1);
  // MIDI.turnThruOff();

  // MIDI.setHandleNoteOn(splitNoteOnStack);

  MIDI.setHandleNoteOn(doThisOnNoteOn); // Calls the function on note on (key pressed)
  MIDI.setHandleNoteOff(doThisOnNoteOff); // Calls the function on note off (key lifted)
  MIDI.setHandlePitchBend(doThisOnPitchBend); // Calls the function when controller pitch bend is touched
  MIDI.setHandleControlChange(doThisOnControlChange); // Callse the function when controller mod pad is touched
}

void loop() {
  // Testing DAC:
  //setDac(137, 0);
  //setDac(100, 1);
  MIDI.read();
  // Read incoming messages
}


void doThisOnPitchBend(byte channel, int val) {
  // monophonic solo mode
  if(digitalRead(switchPin) == LOW && digitalRead(modePin) == LOW) {
     int bentDacInput = monophonicDacInput + (val * 0.05);
     setDac(bentDacInput, 0);
     // monophonic unison mode
   } else if (digitalRead(switchPin) == LOW && digitalRead(modePin) == HIGH) {
     int i;
     for (i=0; i<CHANNELS; i++) {
       int bentDacInput = monophonicDacInput + (val * 0.05);
       setDac(bentDacInput, i);
     }
   // polyphonic mode
  } else if (digitalRead(switchPin == HIGH)) {
    int i;
    for (i=0; i<CHANNELS; i++) {
      if (pitches[i] != 0) {
        float voltPerOctave = (pitches[i] - 24) * 0.0833;
        int dacInput = (voltPerOctave / refVoltage) * 4096; 
        int bentDacInput = dacInput + (val * 0.05);
        setDac(bentDacInput, i);
      }
    }
  }
}

void doThisOnControlChange(byte channel, byte controllerNum, byte value) {
  // monophonic solo
  if (digitalRead(switchPin) == LOW && digitalRead(modePin) == LOW) {
      setDac(value*30, 2);
  // polyphonic and cycle mode (utilize "pitch" outs 3 and 4 for modulating pitch outs 1 and 2 respectively)
  } else if (digitalRead(switchPin) == HIGH && digitalRead(modePin) == HIGH) {
      setDac(value*30, 2);
      setDac(value*30, 3);
    }
}

void doThisOnNoteOn(byte channel, byte pitch, byte velocity) {
  // noteOnSequence = noteOnSequence + 1;
  // if (noteOnSequence > 2) noteOnSequence = 1;
  
  float voltPerOctave = (pitch - 24) * 0.0833;
  int dacInput = (voltPerOctave / refVoltage) * 4096;

  if(digitalRead(switchPin) == HIGH) {  //polyphonic mode
    bool found = false;
    int i;
    //find a free/silent voice
    for (i=0; i<CHANNELS; i++) {
      if (pitches[i] == 0) {
        found = true;
        break;
      }
    }

    if (found) {
      digitalWrite(i+5, LOW);
      pitches[i] = pitch;
      setDac(dacInput, i);
      digitalWrite(i+5, HIGH);
    }
    //if there is no free channel found, events are dropped
    
  } else { //monophonic mode
    digitalWrite(5, LOW);
    monophonicNote = pitch;
    monophonicDacInput = dacInput;
    setDac(dacInput, 0);
    digitalWrite(5, HIGH);

    if (digitalRead(modePin) == HIGH) { // monophonic unison mode
      //digitalWrite(6, LOW);
      //setDac(dacInput, 1);
      //digitalWrite(6, HIGH);

      int i;
      for (i = 1; i < CHANNELS; i++) {
        digitalWrite(i+5, LOW);
        setDac(dacInput, i);
        digitalWrite(i+5, HIGH); 
      }
    
    }   
  }
}

void doThisOnNoteOff(byte channel, byte pitch, byte velocity) {//Handles note off correctly for previously played noted

  if(digitalRead(switchPin) == HIGH) {

      bool found = false;
      int i;
      //find voice with the incoming pitch
      for (i = 0; i<CHANNELS; i++) {
        if(pitches[i] == pitch) {
          found = true;
          break;
        }
      }
      //set channel note off
      if(found) {
        pitches[i] = 0;
        MIDI.sendNoteOff(pitch, velocity, i+1);
        digitalWrite(i+5, LOW);
      }
  } else {
    if(monophonicNote == pitch) {
      MIDI.sendNoteOff(1, pitch, 0);
      digitalWrite(5, LOW);
    }
    
    if(digitalRead(modePin) == HIGH) { //monophonic unison
      // digitalWrite(6, LOW);

      int i;
      for (i=1; i<CHANNELS; i++) {
        digitalWrite(i+5, LOW);
      }
    }
  }
}

void setDac(int value, int channel) { // Function to pass MIDI note to DAC
  byte defaultRegister = 0b00110000;
  byte primaryByte = (value >> 8) | defaultRegister;
  if (channel == 1 || channel == 3) {
    primaryByte |= 0b10000000;
  }
  byte secondaryByte = value;
  noInterrupts();

  if(channel == 0 || channel == 1) {
    digitalWrite(chipSelect, LOW);
  }
  else {
    digitalWrite(chipSelect2, LOW);
  }
  SPI.transfer(primaryByte);
  SPI.transfer(secondaryByte);
  
  if(channel == 0 || channel == 1) {
    digitalWrite(chipSelect, HIGH);
  } else {
    digitalWrite(chipSelect2, HIGH);
  }
  
  interrupts();

}
