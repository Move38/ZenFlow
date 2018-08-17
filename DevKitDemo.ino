enum modeStates {SPREAD, CONNECT};
byte currentMode;

enum commandStates {INERT, SEND_PERSIST, SEND_SPARKLE, RESOLVING};
byte commandState;//this is the state sent to neighbors
byte internalState;//this state is internal, and does not get sent

// Colors by hue
byte hues[7] = {0, 21, 42, 85, 110, 170, 210}; //the last color is a lime green color

byte currentHue;
byte transitionColor;
byte sparkleOffset[6] = {0, 3, 5, 1, 4, 2};

#define SEND_DELAY     100
#define SEND_DURATION  800

uint32_t timeOfSend = 0;

Timer sendTimer;
Timer transitionTimer;

Timer animTimer;
int animIncrement;

byte animFrame = 0;

byte sendData;

void setup() {
  // put your setup code here, to run once:
  currentMode = SPREAD;
  commandState = INERT;
  internalState = INERT;
  currentHue = 0;
  animIncrement = 150;
}

void loop() {
  // update the random value
  rand(1);

  // decide which loop to run
  if (currentMode == SPREAD) {//spread logic loops
    switch (internalState) {
      case INERT:
        inertLoop();
        break;
      case SEND_PERSIST:
        sendPersistLoop();
        break;
      case SEND_SPARKLE:
        sendSparkleLoop();
        break;
      case RESOLVING:
        resolvingLoop();
        break;
    }
  } else if (currentMode == CONNECT) {
    connectLoop();
  }


  //communicate full state
  sendData = (currentMode << 5) + (commandState << 3) + (currentHue);//bit-shifted data to fit in 6 bits
  setValueSentOnAllFaces(sendData);

  //do display work
  if (currentMode == SPREAD) {
    switch (internalState) {
      case RESOLVING:
      case INERT:
        inertDisplay();//both inert and resolving share the same display logic
        break;
      case SEND_PERSIST:
        sendPersistDisplay();
        break;
      case SEND_SPARKLE:
        sendSparkleDisplay();
        break;
    }
  } else if (currentMode == CONNECT) {
    connectDisplay();
  }
}


void inertLoop() {
  //if single clicked, move to SEND_PERSIST
  if (buttonSingleClicked()) {
    changeInternalState(SEND_PERSIST);
    currentHue = nextHue(currentHue);
  }

  //if double clicked, move to SEND_SPARKLE
  if (buttonDoubleClicked()) {
    changeInternalState(SEND_SPARKLE);
    currentHue = rand(COUNT_OF(hues) - 1); //generate a random color
  }

  //if long-pressed, move to CONNECT mode
  if (buttonLongPressed()) {
    currentMode = CONNECT;
  }

  //now we evaluate neighbors. if our neighbor is in either send state, move to that send state
  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) {
      byte neighborData = getLastValueReceivedOnFace(f);
      if (getCommandState(neighborData) == SEND_PERSIST) {
        changeInternalState(SEND_PERSIST);
        currentHue = getHue(neighborData);//you are going to take on the color of the commanding neighbor
      }
      if (getCommandState(neighborData) == SEND_SPARKLE) {
        changeInternalState(SEND_SPARKLE);
        currentHue = rand(COUNT_OF(hues) - 1); //generate a random color
      }
    }
  }
}

void sendPersistLoop() {
  if (isAlone()) {
    if (buttonSingleClicked()) {
      currentHue = nextHue(currentHue);
      changeInternalState(SEND_PERSIST);
    }
  }

  // button cleaning
  buttonSingleClicked();
  buttonDoubleClicked();
  buttonLongPressed();


  //first, check if it's been long enough to send the command
  if (sendTimer.isExpired()) {
    commandState = internalState;
  }

  //now check neighbors. If they have all moved into SEND_PERSIST or RESOLVING, you can move to RESOLVING
  //Only do this check if we are past the full display time
  if (transitionTimer.isExpired()) {
    bool canResolve = true;//default to true, set to false in the face loop
    FOREACH_FACE(f) {
      byte neighborData = getLastValueReceivedOnFace(f);//we do this before checking for expired so we can use it to evaluate mode below
      if (!isValueReceivedOnFaceExpired(f) && getMode(neighborData) == SPREAD) {//something is here, and in a compatible mode. We ignore the others
        if (getCommandState(neighborData) != SEND_PERSIST && getCommandState(neighborData) != RESOLVING) {//it is neither of the acceptable states
          canResolve = false;
        }
      }
    }//end of face loop

    //if we've survived and are stil true, we transition to resolving
    if (canResolve && !isAlone()) {
      changeInternalState(RESOLVING);
      commandState = RESOLVING;
    }
  }
}

void sendSparkleLoop() {
  // button cleaning
  buttonSingleClicked();
  buttonDoubleClicked();
  buttonLongPressed();

  //first, check if it's been long enough to send the command
  if (sendTimer.isExpired()) {
    commandState = internalState;
  }

  FOREACH_FACE(f) {
    byte neighborData = getLastValueReceivedOnFace(f);
    if (getCommandState(neighborData) == SEND_PERSIST) {
      changeInternalState(SEND_PERSIST);
      currentHue = getHue(neighborData);
    }
  }

  //now check neighbors. If they have all moved into SEND_SPARKLE or RESOLVING, you can move to RESOLVING
  //Only do this check if we are past the full display time
  if (transitionTimer.isExpired()) {
    bool canResolve = true;//default to true, set to false in the face loop
    FOREACH_FACE(f) {
      byte neighborData = getLastValueReceivedOnFace(f);//we do this before checking for expired so we can use it to evaluate mode below
      if (!isValueReceivedOnFaceExpired(f) && getMode(neighborData) == SPREAD) {//something is here, and in a compatible mode. We ignore the others
        if (getCommandState(neighborData) != SEND_SPARKLE && getCommandState(neighborData) != RESOLVING) {//it is neither of the acceptable states
          canResolve = false;
        }
      }
    }//end of face loop

    //if we've survived and are stil true, we transition to resolving
    if (canResolve) {
      changeInternalState(RESOLVING);
      commandState = RESOLVING;
    }
  }
}

void resolvingLoop() {
  // button cleaning
  buttonSingleClicked();
  buttonDoubleClicked();
  buttonLongPressed();

  //check neighbors. If they have all moved into RESOLVING or INERT, you can move to INERT
  bool canInert = true;//default to true, set to false in the face loop
  FOREACH_FACE(f) {
    byte neighborData = getLastValueReceivedOnFace(f);//we do this before checking for expired so we can use it to evaluate mode below
    if (!isValueReceivedOnFaceExpired(f) && getMode(neighborData) == SPREAD) {//something is here, and in a compatible mode. We ignore the others
      if (getCommandState(neighborData) != RESOLVING && getCommandState(neighborData) != INERT) {//it is neither of the acceptable states
        canInert = false;
      }
    }
  }//end of face loop

  //if we've survived and are stil true, we transition to resolving
  if (canInert) {
    changeInternalState(INERT);
    commandState = INERT;
  }
}

void connectLoop() {
  // button cleaning
  buttonSingleClicked();
  buttonDoubleClicked();

  //all that happens in here is we wait for long press to move back to spread. That's it
  if (buttonLongPressed()) {
    currentMode = SPREAD;
    changeInternalState(INERT);
    animFrame = 0;
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




/*
    Display Animations
*/
void inertDisplay() {

  FOREACH_FACE(f) {
    // minimum of 125, maximum of 255
    byte phaseShift = 60 * f;
    byte amplitude = 55;
    byte midline = 185;
    byte rate = 4;
    byte brightness = midline + amplitude * sin_d( (phaseShift + millis() / rate) % 360);
    byte saturation = 255;

    Color faceColor = makeColorHSB(hues[currentHue], 255, brightness);
    setColorOnFace(faceColor, f);
  }
}

void sendPersistDisplay() {
  // go full white and then fade to new color
  uint32_t delta = millis() - timeOfSend;

  // show that we are charged up when alone
  if (isAlone()) {
    while(delta > SEND_DURATION * 3) {
      delta -= SEND_DURATION * 3;
    }
  }

  if (delta > SEND_DURATION) {
    delta = SEND_DURATION;
  }

  FOREACH_FACE(f) {
    // minimum of 125, maximum of 255
    byte phaseShift = 60 * f;
    byte amplitude = 55;
    byte midline = 185;
    byte rate = 4;
    byte brightness = midline + amplitude * sin_d( (phaseShift + millis() / rate) % 360);
    byte saturation = map_m(delta, 0, SEND_DURATION, 0, 255);

    Color faceColor = makeColorHSB(hues[currentHue], saturation, brightness);
    setColorOnFace(faceColor, f);
  }
}

void sendSparkleDisplay() {
  // go full white and then fade to new color, pixel by pixel
  uint32_t delta = millis() - timeOfSend;

  byte offset = 50;

  FOREACH_FACE(f) {

    // if the face has started it's glow
    uint16_t sparkleStart = sparkleOffset[f] * offset;
    uint16_t sparkleEnd = sparkleStart + SEND_DURATION - (6 * offset);

    if ( delta > sparkleStart ) {
      // minimum of 125, maximum of 255
      byte phaseShift = 60 * f;
      byte amplitude = 55;
      byte midline = 185;
      byte rate = 4;
      byte lowBri = midline + amplitude * sin_d( (phaseShift + millis() / rate) % 360);
      byte brightness;
      byte saturation;

      if ( delta < sparkleEnd ) {
        brightness = map_m(delta, sparkleStart, sparkleStart + SEND_DURATION - (6 * offset), 255, lowBri);
        saturation = map_m(delta, sparkleStart, sparkleStart + SEND_DURATION - (6 * offset), 0, 255);
      }
      else {
        brightness = lowBri;
        saturation = 255;
      }

      Color faceColor = makeColorHSB(hues[currentHue], saturation, brightness);
      setColorOnFace(faceColor, f);
    }
    else {
      // setColorOnFace(OFF, f);
    }
  }
}

void connectDisplay() {
  if (isAlone()) { //so this is a lonely blink. we just set it to full white
    // minimum of 125, maximum of 255
    byte amplitude = 30;
    byte midline = 100;
    byte rate = 3;
    byte brightness = midline + amplitude * sin_d( (millis() / rate) % 360);

    Color faceColor = makeColorHSB(0, 0, brightness);
    setColor(faceColor);

  } else {
    setColor(OFF);//later in the loop we'll add the colors
  }

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) { //something there
      byte neighborData = getLastValueReceivedOnFace(f);
      //now we figure out what is there and what to do with it
      if (getMode(neighborData) == SPREAD) { //this neighbor is in spread mode. Just display the color they are on that face
        Color faceColor = makeColorHSB(hues[getHue(neighborData)], 255, 255);
        setColorOnFace(faceColor, f);
      } else if (getMode(neighborData) == CONNECT) { //this neighbor is in connect mode. Display a white connection
        setColorOnFace(WHITE, f);
      }
    }
  }//end of face loop
}




/*
    Data parsing
*/
void changeInternalState(byte state) {
  //this is here for the moment of transition. mostly housekeeping
  switch (state) {
    case INERT:
      // nothing to do here
      break;
    case SEND_PERSIST:
      timeOfSend = millis();
      sendTimer.set(SEND_DELAY);
      transitionTimer.set(SEND_DURATION);
      break;
    case SEND_SPARKLE:
      timeOfSend = millis();
      sendTimer.set(SEND_DELAY);
      transitionTimer.set(SEND_DURATION);
      break;
    case RESOLVING:
      break;
  }

  internalState = state;
}


byte nextHue(byte h) {
  byte nextHue = (h + 1) % COUNT_OF(hues);
  return nextHue;
}

// Sin in degrees ( standard sin() takes radians )

float sin_d( uint16_t degrees ) {

  return sin( ( degrees / 360.0F ) * 2.0F * PI   );
}

// map value
long map_m(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
