/*
This is an experimental attempt at a slightly smarter thermostat for my home

There are currently 2 inputs, an up and down button
These shift the center point up and down
*/

// Arduino convention is byte, but could be uint8_t or unsigned char
// pins
const byte tempsens  = 0;
const byte buttonUp  = 2;
const byte buttonDwn = 3;
const byte relayFan  = 11;
const byte relayAC   = 12;
const byte relayHeat = 13;

// numeric to text shortcuts
const byte off  = 0;
const byte cool = 1;
const byte heat = 2;
const byte fan  = 3;

// parameters
const float hysteresis = 0.5;
const float offset = 5.0;

// these values get used in the loop
byte centerpoint, mode, prevUp, prevDwn;


void setup() {
    // setup pins
    pinMode(tempsens,  INPUT);
    pinMode(buttonUp,  INPUT);
    pinMode(buttonDwn, INPUT);
    pinMode(relayFan,  OUTPUT);
    pinMode(relayAC,   OUTPUT);
    pinMode(relayHeat, OUTPUT);

    // start serial monitor
    Serial.begin(9600);

    // when we boot up set a reasonable mode and temperature
    centerpoint = 69;
    mode = cool;
}


void loop() {
    // check if we are pushing any buttons when the loop begins
    byte stateUp  = digitalRead(buttonUp);
    byte stateDwn = digitalRead(buttonDwn);

    // if this is first time button is pressed, move the setpoint
    // if both buttons are pressed, toggle system on and off
    if ( (stateUp == LOW) && (stateDwn == LOW) ) {
        if (mode == off) {
            mode = cool;
        } else {
            mode = off;
        }
    } else if ((stateUp == LOW) && !(prevUp == LOW)) {
        centerpoint++;
    } else if ((stateDwn == LOW) && !(prevDwn == LOW)) {
        centerpoint--;
    }

    // store current button states in global vars for next loop through
    prevUp  = stateUp;
    prevDwn = stateDwn;

    // calculate the desired and current temperatures
    // TODO: use floating point math until i can get a handle on integer scaling
    // speed is not really a big deal here
    float coolpoint = centerpoint + offset;
    float heatpoint = centerpoint - offset;
    float temperature = getTempF(tempsens);

    // we used a 1 degree swing around our desired temperature
    // firing the relays is done in driveUnit function
    if (temperature > (coolpoint + hysteresis)) {
        driveUnit(cool);
    } else if (temperature < (heatpoint - hysteresis)) {
        driveUnit(heat);
    } else if (temperature < (coolpoint - hysteresis)) {
        driveUnit(off);
    } else if (temperature < (heatpoint + hysteresis)) {
        driveUnit(off);
    }

    // print the current state for debugging
    //Serial.print(F("adc0: "));
    //Serial.print(adc0);
    Serial.print(F("   deg F: "));
    Serial.print(temperature);
    Serial.print(F("   centerpoint:"));
    Serial.println(centerpoint);

    // slow the loop slightly
    delay(50); //ms
}


float getTempF(int pin) {
    // get the temperature
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
    float voltage, tempC;
    voltage = analogRead(pin)*0.004882814;
    tempC   = (voltage - 0.5) * 100.0;

    return (tempC * (9.0/5.0) + 32.0);
}


void driveUnit(byte unit) {
   // this function controls outputs that control relays
   // heat and cool spin up the fan 1 minute before engaging
   // call this recursively to do so

   // wiring notes
   // red wire - 24V hot
   // blue wire - 24V common
   // green wire - fan
   // yellow wire - cooling
   // white wire - heating
   switch(unit){
    case cool:
        driveUnit(fan);
        delay(60000); //ms
        digitalWrite(relayAC, HIGH);
        break;
    case heat:
        driveUnit(fan);
        delay(60000); //ms
        digitalWrite(relayHeat, HIGH);
        break;
    case fan:
        digitalWrite(relayFan, HIGH);
        break;
    default:
        digitalWrite(relayFan, LOW);
        digitalWrite(relayAC, LOW);
        digitalWrite(relayHeat, LOW);
        break;
    }
}
