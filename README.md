# oxa
To control the sublimation of oxalic dihydrate.

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
