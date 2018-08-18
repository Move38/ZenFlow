/*
   Message Debugger
*/

#include "Serial.h"

ServicePortSerial Serial;

void setup() {
  // put your setup code here, to run once:
  Serial.begin();
  setValueSentOnAllFaces(32);
}

void loop() {
  // put your main code here, to run repeatedly:

  setColor(dim(WHITE, 64));
  // read in message on each face
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      if (didValueOnFaceChange(f)) {
        byte data = getLastValueReceivedOnFace(f);
        Serial.print("mode: ");
        Serial.print(getMode(data));
        Serial.print("     command: ");
        Serial.print(getCommandState(data));
        Serial.print("     hue: ");
        Serial.println(getHue(data));
        setFaceColor(f, RED);
      }
    }
  }
}


/*
    Data parsing
*/
byte getMode(byte data) {
  return (data >> 5);               //the value in the first bit
}

byte getCommandState (byte data) {
  return ((data >> 3) & 3);         //the value in the second and third bits
}

byte getHue(byte data) {
  return (data & 7);                //the value in the fourth, fifth, and sixth bits
}
/*
    End Data parsing
*/


