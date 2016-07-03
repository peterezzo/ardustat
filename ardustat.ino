/*
This is an experimental attempt at a slightly smarter thermostat for my home

There are currently 2 inputs, an up and down button
These shift the center point up and down
*/

#define SERIALDEBUG
#include <EEPROM.h>

// Arduino convention is usually byte, but even arduino.h uses uint8_t
// pins
const uint8_t analogTempSens = 0;
const uint8_t buttonUp = 2;
const uint8_t buttonDwn = 3;
const uint8_t relayFan = 11;
const uint8_t relayAC = 12;
const uint8_t relayHeat = 13;

// numeric to text shortcuts
const uint8_t off = 0;
const uint8_t on = 1;
const uint8_t cool = 2;
const uint8_t heat = 3;
const uint8_t fan = 4;
const uint8_t shutdown = 5;
const uint8_t cycle = 6;

// eeprom storage locations
const uint8_t eepCheckVal = 0xAA;
const int eepCheckAddr = 0;
const int eepModeAddr = 1;
const int eepCenterpointAddr = 2;

// temperature parameters scaled as degree F x 10
const int defaultCenterpoint = 690; // 69.0
const int hysteresis = 5; // 0.5
const int offset = 50; // 5

// HVAC system parameters
const unsigned long fanDelay = 60000; // ms, 1 minute
const unsigned long cycleOn = 900000; // ms, 15 minutes
const unsigned long cycleOff = 2700000; // ms, 45 minutes


// global variables used in the loops
int centerpoint;
uint8_t mode;

void setup(void) {
    // setup digital pins
    pinMode(buttonUp, INPUT); // is default
    pinMode(buttonDwn, INPUT); // is default
    pinMode(relayFan, OUTPUT);
    pinMode(relayAC, OUTPUT);
    pinMode(relayHeat, OUTPUT);

#if defined(SERIALDEBUG)
    // start serial monitor
    Serial.begin(9600);
#endif

    // get mode and temperature settings from eeprom or default values
    // we store a basic check pattern in first bit
    // if it's there, load the previous settings
    // otherwise use the defaults and write them to EEPROM
    // note that mode only stores either "on" or "off"
    if (eepCheckVal == EEPROM.read(eepCheckAddr)) {
#if defined(SERIALDEBUG)
        Serial.println(F("Using mode and temperature from EEPROM"));
#endif
        mode = EEPROM.read(eepModeAddr);
        centerpoint = getEEPROMint(eepCenterpointAddr);
    } else {
#if defined(SERIALDEBUG)
        Serial.println(F("Using mode and temperature from defaults"));
#endif
        mode = on;
        centerpoint = defaultCenterpoint;

        // EEPROM.clear();
        setEEPROMbyte(eepCheckAddr, eepCheckVal);
        setEEPROMbyte(eepModeAddr, mode);
        setEEPROMint(eepCenterpointAddr, centerpoint);
    }
}


void loop(void) {
    // variable declaration
    static uint8_t prevUp = HIGH, prevDwn = HIGH;
    uint8_t stateUp, stateDwn;
    int coolOn, coolOff, heatOn, heatOff, temperature;

    // check if we are pushing any buttons when the loop begins
    stateUp  = digitalRead(buttonUp);
    stateDwn = digitalRead(buttonDwn);

    // if this is first time button is pressed, move the setpoint
    // if both buttons are pressed, toggle system on and off and save to eeprom
    // write this as an interrupt someday?
    if ((stateUp == LOW) && (stateDwn == LOW) && (prevUp != LOW) && (prevDwn != LOW)) {
        if (mode == off) {
            mode = on;
        } else {
            mode = off;
        }
        setEEPROMbyte(eepModeAddr, mode);
    } else if ((stateUp == LOW) && (prevUp != LOW)) {
        centerpoint += 10; // +1.0 F
        setEEPROMint(eepCenterpointAddr, centerpoint);
    } else if ((stateDwn == LOW) && (prevDwn != LOW)) {
        centerpoint -= 10; // -1.0 F
        setEEPROMint(eepCenterpointAddr, centerpoint);
    }

    // store current button states in global vars for next loop through
    prevUp  = stateUp;
    prevDwn = stateDwn;

    // calculate the desired and current temperatures
    // we used a 1 degree swing around our desired temperature (hysteresis)
    coolOn = centerpoint + offset + hysteresis;
    coolOff = centerpoint + offset - hysteresis;
    heatOn = centerpoint - offset + hysteresis;
    heatOff = centerpoint - offset - hysteresis;
    temperature = getTempF(analogTempSens);

    // determine what mode we should be in
    // firing the relays is done in driveUnit function
    if (mode != off) {
        if ((temperature > coolOn) && (mode != cool)) {
            mode = cool;
        } else if ((temperature < heatOn) && (mode != heat)) {
            mode = heat;
        } else if ((temperature > heatOff) && (mode == heat)) {
            mode = shutdown;
        } else if ((temperature < coolOff) && (mode == cool)) {
            mode = shutdown;
        } else {
            mode = cycle;
        }
    }
    driveUnit(mode);

#if defined(SERIALDEBUG)
    // print the current state for debugging
    Serial.print(F("   deg F: "));
    Serial.print(temperature);
    Serial.print(F("   centerpoint:"));
    Serial.println(centerpoint);
#endif

    // slow the loop slightly
    delay(50); //ms
}


int getTempF(int pin) {
    /* get the temperature
    Args:
        pin (int):  the Arduino analog input pin to read from
    Returns:
        temp (int):  scaled Farenheit temperature (10x float)
    */
    float voltage, tempC, tempF;

    // method 1 integer math, loses precision in the typical house range?
    // take in the ADC value, int scale voltage to 5V=500
    // using a TMP36 which is 10mV/C with a +0.5V offset
    //int adc0, tempC
    //int tempscale   = 500;
    //int tmp36offset = 50;
    //float tempF;
    //adc0    = analogRead(pin);
    //tempC   = ( ((adc0*2) + 1) * tempscale + 1024) / 2048 - tmp36offset;

    // method 2 use slower floats but simpler
    // multiplier value taken from sparkfun
    voltage = analogRead(pin)*0.004882814;

    tempC = (voltage - 0.5) * 100.0;
    tempF = (tempC * (9.0/5.0) + 32.0);

#if defined(SERIALDEBUG)
    Serial.print(F("voltage: "));
    Serial.print(voltage);
    Serial.print(F("   deg C: "));
    Serial.print(tempC);
#endif

    return int(tempF * 10.0);
}


void driveUnit(uint8_t unit) {
    /*
    this function controls outputs that control relays
    heat and cool spin up the fan 1 minute before engaging
    call this recursively to do so
    shutdown leaves the fan on 1 minute after

    Args:
        unit (uint8_t): which HVAC function to activate, or off for all off
    Returns:
        None

    wiring notes:
    red wire - 24V hot
    blue wire - 24V common
    green wire - fan
    yellow wire - cooling
    white wire - heating
    */
    static unsigned long timeFanOn = 0;
    unsigned long timeCurrent = millis();

    switch(unit) {
        case cool:
            // we must always be sure that fan is on before AC
            driveUnit(fan);
            // dwell for fanDelay ms if turning it on first time
            if ((timeCurrent - timeFanOn) > fanDelay) {
                // only write to pin if not already on
                if (digitalRead(relayAC) != HIGH) {
                    digitalWrite(relayAC, HIGH);
                }
            }
            break;
        case heat:
            // heat operates exactly the same way as cool
            driveUnit(fan);
            if ((timeCurrent - timeFanOn) > fanDelay) {
                if (digitalRead(relayHeat) != HIGH) {
                    digitalWrite(relayHeat, HIGH);
                }
            }
            break;
        case fan:
             // turn on fan and record time that we do so, only if fan not on
            if (digitalRead(relayFan) != HIGH) {
                timeFanOn = timeCurrent;
                digitalWrite(relayFan, HIGH);
            }
            break;
        case shutdown:
            // turn off other units, then purge vents for fanDelay ms
            // shutdown is only ever the mode set for exactly 1 loop run
            // it is followed by cycle, abuse the timing state var and let
            // cycle shut off the fan after fanDelay time
            timeFanOn = (timeCurrent - cycleOn) + fanDelay;
            digitalWrite(relayAC, LOW);
            digitalWrite(relayHeat, LOW);
            break;
        case cycle:
            // run the fan periodically
            if ((timeCurrent - timeFanOn) > cycleOff) {
                driveUnit(fan);
            } else if ((timeCurrent - timeFanOn) > cycleOn) {
                digitalWrite(relayFan, LOW);
            }
            break;
        default:
            // this is used for "off" and "on" and anything broken
            digitalWrite(relayFan, LOW);
            digitalWrite(relayAC, LOW);
            digitalWrite(relayHeat, LOW);
    }

#if defined(SERIALDEBUG)
    Serial.print(F("   mode "));
    Serial.print(mode);
    Serial.print(F("   unit "));
    Serial.print(unit);
    Serial.print(F("   timeCur "));
    Serial.print(timeCurrent);
    Serial.print(F("   timeFan "));
    Serial.print(timeFanOn);
#endif
}


int getEEPROMint(int address) {
    /*
    This function retrieves an int stored in EEPROM starting at address provided

    Arguments:
        address (int):  EEPROM byte to begin reading first byte from
    Returns:
        value (int):  integer stored in the EEPROM
    */
    return word(EEPROM.read(address), EEPROM.read(address + 1));
}


void setEEPROMint(int address, int value) {
    /*
    This function stores an int in EEPROM starting at address provided,
    but only if value is different than presently stored

    Arguments:
        address (int):  EEPROM byte to begin storage at
        value (int):  integer value to store
    Returns:
        None
    */
    setEEPROMbyte(address, highByte(value));
    setEEPROMbyte(address + 1, lowByte(value));
}


void setEEPROMbyte(int address, uint8_t value) {
    /*
    This function stores a byte in EEPROM at address provided,
    but only if value is different than presently stored

    Arguments:
        address (int):  EEPROM byte to begin storage at
        value (uint8_t):  byte value to store
    Returns:
        None
    */
    if (EEPROM.read(address) != value) {
        EEPROM.write(address, value);
    }
}
