#include <Homie.h>

const int PIN_RELAY = 12;
const int PIN_LED = 13;
const int PIN_BUTTON = 0;
const int PIN_MOTION = 14;

bool isInitializing = true;

unsigned long buttonDownTime = 0;
byte lastButtonState = 1;
byte buttonPressHandled = 0;

byte lastMotionState = 1;
byte motionHandled = 0;

const char PROPERTY[] = "state";
const char ON[] = "ON";
const char OFF[] = "OFF";

HomieNode switchNode("switch", "switch");
HomieNode motionNode("motion", "motion");

void setState(bool on) {
  const char* state = on ? ON : OFF;
  digitalWrite(PIN_RELAY, on ? HIGH : LOW);
  switchNode.setProperty(PROPERTY).send(state);
  Homie.getLogger() << "Switch is " << state << endl;
}

bool switchStateHandler(HomieRange range, String value) {
  if (value != ON && value != OFF) {
    return false;
  }

  setState(value == ON);
  return true;
}

void buttonPressed() {
  const bool on = digitalRead(PIN_RELAY) == LOW;
  setState(on);
}

void checkButtonPressed() {
  byte buttonState = digitalRead(PIN_BUTTON);
  if ( buttonState != lastButtonState ) {
    if (buttonState == LOW) {
      buttonDownTime     = millis();
      buttonPressHandled = 0;
    }
    else {
      unsigned long dt = millis() - buttonDownTime;
      if ( dt >= 90 && dt <= 900 && buttonPressHandled == 0 ) {
        buttonPressed();
        buttonPressHandled = 1;
      }
    }
    lastButtonState = buttonState;
  }
}

void checkMotion() {
  byte motionState = digitalRead(PIN_MOTION);
  if ( motionState != lastMotionState ) {
    lastMotionState = motionState;
    Homie.getLogger() << "Motion State: " << motionState << endl;
    motionNode.setProperty(PROPERTY).send(motionState ? ON : OFF);
  }
}

void onHomieEvent(const HomieEvent& event) {
  switch(event.type) {
    case HomieEventType::MQTT_CONNECTED:
      if (isInitializing) {
        isInitializing = false;
        setState(ON);
      }
      break;
  }
}

void loopHandler() {
  checkButtonPressed();
  checkMotion();
}

void setup() {
  Serial.begin(115200);
  Serial << endl << endl;

  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_BUTTON, INPUT);
  pinMode(PIN_MOTION, INPUT);

  digitalWrite(PIN_RELAY, HIGH);

  Homie_setFirmware("sonoff-motion", "1.0.0");
  Homie.setLedPin(PIN_LED, LOW).setResetTrigger(PIN_BUTTON, LOW, 10000);

  switchNode.advertise(PROPERTY).settable(switchStateHandler);

  Homie.setLoopFunction(loopHandler);
  Homie.onEvent(onHomieEvent);

  Homie.setup();
}

void loop() {
  Homie.loop();
}
