#define ATMEGA32U4 1 

/////////////////////////////////////////////
// Bibliotecas

#ifdef ATMEGA328
#include <MIDI.h>
#elif ATMEGA32U4
#include "MIDIUSB.h"

#endif


/////////////////////////////////////////////
// Botones
const int N_BUTTONS = 12; 
const int BUTTON_ARDUINO_PIN[N_BUTTONS] = {10, 16, 14, 15, 6, 7, 8, 9, 2, 3, 4, 5}; 

//#define pin13 1 // solo descomentar si uso el 13
byte pin13index = 12; //* indice del pin 13 en el array buttonPin[] si se usa.

int buttonCState[N_BUTTONS] = {};     
int buttonPState[N_BUTTONS] = {};        

// debounce
unsigned long lastDebounceTime[N_BUTTONS] = {0}; 
unsigned long debounceDelay = 20;    

/////////////////////////////////////////////
// POTENCIOMETROS
const int N_POTS = 4; 
const int POT_ARDUINO_PIN[N_POTS] = {A3, A2, A1, A0}; 

int potCState[N_POTS] = {0}; 
int potPState[N_POTS] = {0}; 
int potVar = 0; 

int midiCState[N_POTS] = {0}; 
int midiPState[N_POTS] = {0}; 

const int TIMEOUT = 300; 
const int varThreshold = 10;
boolean potMoving = true; 
unsigned long PTime[N_POTS] = {0}; 
unsigned long timer[N_POTS] = {0}; 

/////////////////////////////////////////////
// midi
byte midiCh = 2; 
byte note = 36; 
byte cc = 11; 

/////////////////////////////////////////////
// SETUP
void setup() {

  // Baud Rate
  // 31250 para MIDI class compliant | 115200 para Hairless MIDI
  Serial.begin(115200); //*

#ifdef DEBUG
Serial.println("Debug mode");
Serial.println();
#endif

  // Buttons
  // inicializa botones con el pull up resistor
  for (int i = 0; i < N_BUTTONS; i++) {
    pinMode(BUTTON_ARDUINO_PIN[i], INPUT_PULLUP);
  }

#ifdef pin13 // inicializa EL pin 13 como una entrada
pinMode(BUTTON_ARDUINO_PIN[pin13index], INPUT);
#endif


}

/////////////////////////////////////////////
// LOOP
void loop() {

  buttons();
  potentiometers();

}

/////////////////////////////////////////////
// BOTONES
void buttons() {

  for (int i = 0; i < N_BUTTONS; i++) {

    buttonCState[i] = digitalRead(BUTTON_ARDUINO_PIN[i]);   // lee los pines del arduino

#ifdef pin13
if (i == pin13index) {
buttonCState[i] = !buttonCState[i]; 
}
#endif

    if ((millis() - lastDebounceTime[i]) > debounceDelay) {

      if (buttonPState[i] != buttonCState[i]) {
        lastDebounceTime[i] = millis();

        if (buttonCState[i] == LOW) {

          // Envia a nota MIDI de acuerdo con la placa escogida
#ifdef ATMEGA328
// ATmega328 (uno, mega, nano...)
MIDI.sendNoteOn(note + i, 127, midiCh); // note, velocity, channel

#elif ATMEGA32U4
// ATmega32U4 (micro, pro micro, leonardo...)
noteOn(midiCh, note + i, 127);  // channel, note, velocity
MidiUSB.flush();

#elif TEENSY
// Teensy
usbMIDI.sendNoteOn(note + i, 127, midiCh); // note, velocity, channel

#elif DEBUG
Serial.print(i);
Serial.println(": button on");
#endif

        }
        else {
          // Envia a nota MIDI OFF de acuerdo con la placa escogida
#ifdef ATMEGA328
// ATmega328 (uno, mega, nano...)
MIDI.sendNoteOn(note + i, 0, midiCh); // note, velocity, channel

#elif ATMEGA32U4
// ATmega32U4 (micro, pro micro, leonardo...)
noteOn(midiCh, note + i, 0);  // channel, note, velocity
MidiUSB.flush();

#elif TEENSY
// Teensy
usbMIDI.sendNoteOn(note + i, 0, midiCh); // note, velocity, channel

#elif DEBUG
Serial.print(i);
Serial.println(": button off");
#endif

        }
        buttonPState[i] = buttonCState[i];
      }
    }
  }
}

/////////////////////////////////////////////
// POTENTIOMETERS
void potentiometers() {

  for (int i = 0; i < N_POTS; i++) { // hace el loop de todos los potes

    potCState[i] = analogRead(POT_ARDUINO_PIN[i]);

    midiCState[i] = map(potCState[i], 0, 1023, 0, 127); // mapea la lectura del potCState para un valor utilizable em midi

    potVar = abs(potCState[i] - potPState[i]); // Calcula el valor absoluto entre la diferencia del estado atual y el anterior del pot

    if (potVar > varThreshold) { 
      PTime[i] = millis(); // almacena el tiempo anterior
    }

    timer[i] = millis() - PTime[i]; // Resetea el timer 11000 - 11000 = 0ms

    if (timer[i] < TIMEOUT) { 
      potMoving = true;
    }
    else {
      potMoving = false;
    }

    if (potMoving == true) { // si el pot ainda incia el movimiento, envia CC
      if (midiPState[i] != midiCState[i]) {

        // Envia el MIDI CC de acuerdo con la placa seleccionada
#ifdef ATMEGA328
// ATmega328 (uno, mega, nano...)
MIDI.sendControlChange(cc + i, midiCState[i], midiCh); // cc number, cc value, midi channel

#elif ATMEGA32U4
// ATmega32U4 (micro, pro micro, leonardo...)
controlChange(midiCh, cc + i, midiCState[i]); //  (channel, CC number,  CC value)
MidiUSB.flush();

#elif TEENSY
// Teensy
usbMIDI.sendControlChange(cc + i, midiCState[i], midiCh); // cc number, cc value, midi channel

#elif DEBUG
Serial.print("Pot: ");
Serial.print(i);
Serial.print(" ");
Serial.println(midiCState[i]);
//Serial.print("  ");
#endif

        potPState[i] = potCState[i]; // almacena la lectura atual del pot para comparar con la siguiente
        midiPState[i] = midiCState[i];
      }
    }
  }
}

/////////////////////////////////////////////
//  usando con ATmega32U4 (micro, pro micro, leonardo ...)
#ifdef ATMEGA32U4

// Arduino (pro)micro midi functions MIDIUSB Library
void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

#endif