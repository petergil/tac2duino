
////////////////////////////////////////////////////////////////////////
/// Present one (or two) tac-2 controller(s) as HID input device(s). ///
////////////////////////////////////////////////////////////////////////

// TODO: figure out how the HID axis work / are set. Currently they
//       get autoset to −32768 on first update.

// TODO: make sleep time adaptive based on processing time for update
//       of joystick state to make it more consistent.

// NOTE: To enable dual joysticks with linux you need to set some
//       quirks for the usbhid module:
//
// modprobe -r usbhid ; modprobe -v usbhid "quirks=0x2341:0x8037:0x040"
//
// or set it on the kernel command line:
//
// For the Arduino Leonardo
// usbhid.quirks=0x2341:0x8036:0x040
//
// For the Arduino Micro
// usbhid.quirks=0x2341:0x8037:0x040
//
// Format is:
// <vendor>:<product id>:<quirk>
// where 0x040 == HID_QUIRK_MULTI_INPUT
//
// info from: http://mheironimus.blogspot.se/2015/09/linux-support-for-arduino-leonardo.html

#include <Joystick.h>

// enable debug output (to serial)?
// also disables continous updates
//#define DEBUG

// Sleep time between each update
const uint8_t sleepmus = 50;

int tick = 0;
const int maxticks = 100;

// NOTE: masks need to be researched before changing button mappings.
//
// currently manually masking unused bits since if they are not set to
// use the pullup resistor they will flap.
const uint8_t state1mask = 0b10011111;
const uint8_t state2mask = 0b01110100;

uint8_t state1, state2, laststate1, laststate2 = 0;

// controller 1
// NOTE: digital in 1 and 0 are reversed as appears physically on the board
const uint8_t joy1n = 5;
const uint8_t joy1b[] = {1, 0, 2, 3, 4};

// controller 2
// NOTE: for some reason pin 5 & 7 do not register, using 10 & 16 instead
const uint8_t joy2n = 5;
const uint8_t joy2b[] = {6, 8, 9, 10, 16};

// ignore the rest of the inputs
//const uint8_t restn = 5;
//const uint8_t rest[] = {5, 7, 14, 18, 19};

// HID input represenations. ID needs to be unique per input, and 0x03 or higher.
// ID 0x01 and 0x02 are taken for mouse and keyboard by the HID library.
Joystick_ joy[2] = {
  Joystick_(0x03, JOYSTICK_TYPE_JOYSTICK, joy1n, 0, false,
            false, false, false, false, false, false, false, false, false),
  Joystick_(0x04, JOYSTICK_TYPE_JOYSTICK, joy2n, 0, false,
            false, false, false, false, false, false, false, false, false)
};

/**
 * Update the state of joystick nr j.
 */
void updateState(uint8_t j, uint8_t *button, uint8_t numbuttons){
  for(int iter = 0; iter < numbuttons; iter++){
      joy[j].setButton(iter, !digitalRead(button[iter]));
  }
}

/** 
 * Print a byte b as a binary literal
 */
void printBits(byte b)
{
  Serial.print("0b");
  for(int iter = 7; iter >= 0; iter--){
    Serial.print(bitRead(b, iter));
  }
}


void setup() {

  // initialize pins for controller 1
  for(int iter = 0; iter < joy1n; iter++){
    // configure as DigIn  with pull up
    pinMode(joy1b[iter], INPUT_PULLUP);
  }
  
  // initialize pins for controller 2
  for(int iter = 0; iter < joy2n; iter++){
    pinMode(joy2b[iter], INPUT_PULLUP);
  }

  // initialize pins for unused inputs
  //for(int iter = 0; iter < restn; iter++){
  //  pinMode(rest[iter], INPUT_PULLUP);
  //}

  joy[0].begin(false);
  joy[1].begin(false);

#ifdef DEBUG
  Serial.begin(9600);
#endif // DEBUG

}

void loop()
{

  // This is a quick & dirty way to check if any pins have been
  // triggered.
  state1 = ~PIND & state1mask;     // read Port D (D0-7), invert logic, mask relevant bits
  state2 = ~PINB & state2mask;     // read Port B (D8-13), invert logic, mask relevant bits

  if (state1 != laststate1 
      || state2 != laststate2
#ifndef DEBUG
      || tick >= maxticks
#endif // !DEBUG
      ) {        // time to update?

#ifdef DEBUG
    Serial.write("S1: ");
    printBits(state1);
    Serial.write(" S2: ");
    printBits(state2);
    Serial.write("\n");
#endif // DEBUG

    // update currently pressed buttons
    updateState(0, joy1b, joy1n);
    updateState(1, joy2b, joy2n);

    // send state over USB
    joy[0].sendState();
    joy[1].sendState();

    // save current state for next iteration
    laststate1 = state1;
    laststate2 = state2;
    tick = 0;
  }
  
  tick++;
  delayMicroseconds(sleepmus);
}
