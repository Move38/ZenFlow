enum modeStates {SPREAD, CONNECT};
byte currentMode;

enum commandStates {INERT, SEND_PERSIST, SEND_SPARKLE, RESOLVING};
byte commandState;//this is the state sent to neighbors
byte internalState;//this state is internal, and does not get sent

Color colors[8] = {RED, GREEN, BLUE, ORANGE, MAGENTA, YELLOW, CYAN, makeColorRGB(127, 255, 0)}; //the last color is a lime green color
byte currentColor;
byte transitionColor;
byte sparkleBrightness[6] = {0, 0, 0, 0, 0, 0};

Timer sendTimer;
byte sendIncrement = 200;
Timer displayTimer;
int displayIncrement = 1785;//the amount of time it takes to complete a full fade up

Timer animTimer;
int animIncrement;

byte animFrame = 0;

byte sendData;

void setup() {
  // put your setup code here, to run once:
  currentMode = SPREAD;
  commandState = INERT;
  internalState = INERT;
  currentColor = 0;
  animIncrement = 150;
}

void loop() {
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
  sendData = (currentMode << 5) + (commandState << 3) + (currentColor);//bit-shifted data to fit in 6 bits
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
    currentColor = nextColor(currentColor);;
  }

  //if double clicked, move to SEND_SPARKLE
  if (buttonDoubleClicked()) {
    changeInternalState(SEND_SPARKLE);
    currentColor = rand(7);//generate a random color
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
        currentColor = getColor(neighborData);//you are going to take on the color of the commanding neighbor
      }
      if (getCommandState(neighborData) == SEND_SPARKLE) {
        changeInternalState(SEND_SPARKLE);
        currentColor = rand(7);//generate a random color
      }
    }
  }
}

void sendPersistLoop() {
  //first, check if it's been long enough to send the command
  if (sendTimer.isExpired()) {
    commandState = internalState;
  }

  //now check neighbors. If they have all moved into SEND_PERSIST or RESOLVING, you can move to RESOLVING
  //Only do this check if we are past the full display time
  if (displayTimer.isExpired()) {
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
    if (canResolve) {
      changeInternalState(RESOLVING);
      commandState = RESOLVING;
    }
  }
}

void sendSparkleLoop() {
  //first, check if it's been long enough to send the command
  if (sendTimer.isExpired()) {
    commandState = internalState;
  }

  //now check neighbors. If they have all moved into SEND_SPARKLE or RESOLVING, you can move to RESOLVING
  //Only do this check if we are past the full display time
  if (displayTimer.isExpired()) {
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
  //all that happens in here is we wait for long press to move back to spread. That's it
  if (buttonLongPressed()) {
    currentMode = SPREAD;
    changeInternalState(INERT);
    animFrame = 0;
  }
}

/* 
 *  Data parsing
 */
byte getMode(byte data) {
  return (data >> 5);               //the value in the first bit
}

byte getCommandState (byte data) {
  return ((data >> 3) & 3);         //the value in the second and third bits
}

byte getColor(byte data) {
  return (data & 7);                //the value in the fourth, fifth, and sixth bits
}
/* 
 *  End Data parsing
 */




/* 
 *  Display Animations
 */
void inertDisplay() {
  setColor(OFF);
  FOREACH_FACE(f) {
    // minimum of 125, maximum of 255
    byte phaseShift = 60 * f;
    byte amplitude = 55;
    byte midline = 185;
    byte rate = 4;
    byte dimLevel = midline + amplitude * sin_d( (phaseShift + millis() / rate) % 360);
    
    setFaceColor(f, dim( colors[currentColor], dimLevel));
  }
}

void sendPersistDisplay() {
  if (animTimer.isExpired()) {
    animTimer.set(animIncrement);
    animFrame ++;
    int dimnessLevel = animFrame;
    if (dimnessLevel > 255) {
      dimnessLevel = 255;
    }
    setColor(dim(colors[currentColor], dimnessLevel));
  }
}

void sendSparkleDisplay() {
  if (animTimer.isExpired()) {
    animTimer.set(animIncrement);
    animFrame++;
    FOREACH_FACE(f) {
      //in this loop, we increment brightnesses, start flashes, and end flashes
      if (sparkleBrightness[f] == 255) { //this face is at full brightness. Set to 0
        sparkleBrightness[f] = 0;
      } else if (sparkleBrightness[f] > 0) { //not at 255, but in process. Increment by 17
        sparkleBrightness[f] += 15;
      } else if (sparkleBrightness[f] == 0) { //at 0. If we are on a multiple of five frame, randomly decide to start or not start it
        if (animFrame % 10 == 0) { //acceptable starting frame
          sparkleBrightness[f] += rand(1) * 15;//this is either 0 or 17
        }
      }//end of if/else

      //now that we've addressed the brightness of this pixel, we set it
      setColorOnFace(dim(WHITE, sparkleBrightness[f]), f);
    }
  }
}

void connectDisplay() {
  if (isAlone()) { //so this is a lonely blink. we just set it to full white
    setColor(WHITE);
  } else {
    setColor(OFF);//later in the loop we'll add the colors
  }

  FOREACH_FACE(f) {
    if (!isValueReceivedOnFaceExpired(f)) { //something there
      byte neighborData = getLastValueReceivedOnFace(f);
      //now we figure out what is there and what to do with it
      if (getMode(neighborData) == SPREAD) { //this neighbor is in spread mode. Just display the color they are on that face
        setColorOnFace(colors[getColor(neighborData)], f);
      } else if (getMode(neighborData) == CONNECT) { //this neighbor is in connect mode. Display a white connection
        setColorOnFace(WHITE, f);
      }
    }
  }//end of face loop
}




/* 
 *  Data parsing
 */
void changeInternalState(byte state) {
  //this is here for the moment of transition. mostly housekeeping
  switch (state) {
    case INERT:
      animIncrement = 150;
      animTimer.set(animIncrement);
      setColor(OFF);
      break;
    case SEND_PERSIST:
      animIncrement = 7;
      animTimer.set(animIncrement);
      sendTimer.set(sendIncrement);
      displayTimer.set(displayIncrement);
      animFrame = 0;
      setColor(OFF);
      break;
    case SEND_SPARKLE:
      animIncrement = 10;
      animTimer.set(animIncrement);
      sendTimer.set(sendIncrement);
      displayTimer.set(displayIncrement);
      animFrame = 0;
      setColor(OFF);
      break;
    case RESOLVING:
      animFrame = 0;
      animIncrement = 150;
      break;
  }

  internalState = state;
}

byte nextColor(byte col) {
  byte nextCol = col;
  nextCol++;
  if (nextCol == 8) {
    nextCol = 0;
  }
  return nextCol;
}

// Sin in degrees ( standard sin() takes radians )

float sin_d( uint16_t degrees ) {

    return sin( ( degrees / 360.0F ) * 2.0F * PI   );
}
