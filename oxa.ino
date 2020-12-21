/*
  To control the sublimation of oxalic dihydrate.

  title: OXA
  version: 1.0
  date: 19.12.2020
  author: matthias.engelbracht
  lizenz: MIT

  Hardware:
  - ARDUINO NANO
  - LM2596 Dc12V->Dc5V power supply
  - 22 Ohm resistance
  - PT100
  - 2 relays module for fan and heater output
  - H4 car lamp for heating with 12V ~115W
  - button (NO)
  - buzzer

  Setps:
  - press button to start 
  - if temperature is higher than 100°C, cancel
  - switch on fan
  - wait for heat switch on delay
  - heating up to ~170°C for ~3 minutes
  - switch off the heater
  - keep fan going for an other minute
  - switch fan off and wait some time untill everything cooled down
  - give 10 secound a sound
  
  - stop at any time possible by button press
*/

#include <AutoPID.h>

#define cycleTime 25

#define HEATER_PIN 2
#define FAN_PIN 3
#define BUTTON_PIN 5
#define SIGNAL_PIN 4
#define TEMPERATURE_PIN A1 //5V -> 22ohm A1-Input -> PT100 0V

#define TEMPERATURE_ARRAY_LENGHT 100

#define TEMPERATURE_SWITCH_OF_VALUE 910

#define SWITCH_ON_DELAY         3000
#define TOTAL_HEATING_TIME    240000
#define FAN_SWITCH_OFF_DELAY   60000
#define COOL_DOWN_DELAY        30000

#define HEAT_PULS_WIDTH         3000
#define KP                          .10
#define KI                          .0003
#define KD                         0

#define debug                   false //true

bool heatRelayState = false, relayOn = LOW, relayOff = HIGH, impuls1s = false, buttonImpuls = false, button = false, buttonTemp = false;
int oxaStep = 0, temperatureValuesIndex = 0;
double temperature, setPoint;
unsigned long timeAtCycleStart, timeAtCycleEnd, actualCycleTime, modeTimerStart, headTimerStart, mesureTimerStart, timerOneRun, lastTempUpdate, timeImpuls1s = 0;
float temperatureValues [TEMPERATURE_ARRAY_LENGHT] = {0}, temperatureValuesSum = 0.0;

AutoPIDRelay heatPidRelais(&temperature, &setPoint, &heatRelayState, HEAT_PULS_WIDTH, KP, KI, KD);

void pip(int pipTime = 100, int breakTime = 200, int pipCount = 1) {
  for (int i = 0; i < pipCount; i++ )
  {
    digitalWrite(SIGNAL_PIN, HIGH);
    delay(pipTime);
    digitalWrite(SIGNAL_PIN, LOW);
    delay(breakTime);
  }
}

void readTemperatur() {
  temperatureValues[temperatureValuesIndex] = analogRead(TEMPERATURE_PIN);

  if (temperatureValuesIndex >= TEMPERATURE_ARRAY_LENGHT - 1) temperatureValuesIndex = 0;
  else  temperatureValuesIndex ++;

  temperatureValuesSum = 0.0;
  for (int idx = 0; idx < (TEMPERATURE_ARRAY_LENGHT); idx++ )
  {
    temperatureValuesSum += temperatureValues[idx];
  }
  temperature = temperatureValuesSum / float(TEMPERATURE_ARRAY_LENGHT);
}

void resetAll() {
  oxaStep = 0;
  buttonImpuls = false;
  heatRelayState = false;
  heatPidRelais.stop();
  digitalWrite(HEATER_PIN, relayOff);
  digitalWrite(FAN_PIN, relayOff);
  pip(50, 100, 5);
}

void setup() {
  Serial.begin(9600);
  delay(10);

  pinMode(HEATER_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SIGNAL_PIN, OUTPUT);

  digitalWrite(HEATER_PIN, relayOff);
  digitalWrite(FAN_PIN, relayOff);

  int initValue = analogRead(TEMPERATURE_PIN);
  for (int t = 0; t < (TEMPERATURE_ARRAY_LENGHT); t++ )
  {
    temperatureValues[t] = initValue;
  }

  heatPidRelais.setBangBang(20, 1);
  setPoint = TEMPERATURE_SWITCH_OF_VALUE;

  buttonTemp = button = digitalRead(BUTTON_PIN);
  resetAll();
}

void loop() {
  timeAtCycleStart = millis();

  if ((timeAtCycleStart - timeImpuls1s) > 1000)
  {
    impuls1s = true;
    timeImpuls1s = timeAtCycleStart;
  }
  else
  {
    impuls1s = false;
  }

  // button: start/reset
  button = digitalRead(BUTTON_PIN);
  if ((button == false) and (buttonTemp == true))
  {
    buttonImpuls = true;
  }
  else
  {
    buttonImpuls = false;
  }
  buttonTemp = button;

  if ((oxaStep > 0) and (buttonImpuls == true))
  {
    resetAll();
  }

  readTemperatur();

  // oxa step chain
  switch (oxaStep) {
    case 0: // idle
      if (buttonImpuls == true)
      {       
        if (temperature > 885)  //885 ~ 100°C
        {
          Serial.println(int(temperature));
          resetAll();
          break;
        }
        pip(500);
        oxaStep++;
        modeTimerStart = millis();
        timerOneRun = modeTimerStart;
        digitalWrite(FAN_PIN, relayOn);
      }
      break;
    case 1: // fan on
      if (millis() - modeTimerStart > SWITCH_ON_DELAY)
      {
        oxaStep++;
        modeTimerStart = millis();
      }
      break;

    case 2: // heat phase heating
      if (debug and impuls1s)
      {
        Serial.print((millis() - modeTimerStart) / 1000);
        Serial.print(";");
        Serial.print(int(temperature));
        Serial.print(";");
        Serial.println(heatRelayState);
      }

      if (millis() - modeTimerStart > TOTAL_HEATING_TIME)
      {
        heatPidRelais.stop();
        heatRelayState = false;
        digitalWrite(HEATER_PIN, relayOff);

        oxaStep++;
        modeTimerStart = millis();        
      }
      else {
        heatPidRelais.run();
      }

      break;
    case 3: // fan out
      if (millis() - modeTimerStart > FAN_SWITCH_OFF_DELAY)
      {
        digitalWrite(FAN_PIN, relayOff);
        oxaStep++;
        modeTimerStart = millis();        
      }
      else {
        if (debug and impuls1s)
        {
          Serial.print((millis() - modeTimerStart) / 1000);
          Serial.print(";");
          Serial.println(int(temperature));
        }
      }
      break;
    case 4: // wait a moment
      if (millis() - modeTimerStart > COOL_DOWN_DELAY)
      {
        oxaStep++;
      }
      break;

    case 5: // ready signal
      pip(100, 900, 10);
      resetAll();
      break;
  }

  digitalWrite(HEATER_PIN, !heatRelayState);

  actualCycleTime = timeAtCycleEnd - timeAtCycleStart;
  if (actualCycleTime < cycleTime)
  {
    delay(cycleTime - actualCycleTime);
  }
}
