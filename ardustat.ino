// Thermostat 0.1 POC

const byte tempsens  = 0;
const byte buttonUp  = 2;
const byte buttonDwn = 3;
const byte drive     = 13;

const byte off  = 0;
const byte cool = 1;
const byte heat = 2;
const int tempscale   = 500;
const int tmp36offset = 50;

const float hysteresis = 0.5;

byte mode = cool;
byte setpoint = 70;
byte prevUp, prevDwn;

void setup() {
    // setup pins
    pinMode(buttonUp,  INPUT);
    pinMode(buttonDwn, INPUT);
    pinMode(drive,     OUTPUT);

    // start serial monitor
    Serial.begin(9600);
}

void loop() {
    // get if we are pushing any buttons
    byte stateUp, stateDwn;
    stateUp  = digitalRead(buttonUp);
    stateDwn = digitalRead(buttonDwn);

    // adjust the setpoint if so desired
    if ( (stateUp == LOW) && (stateDwn == LOW) ) {
        // change from cool to warm, off??
        if (mode == cool) {
            mode = heat;
        } else {
            mode = cool;
        }
    } else if ((stateUp == LOW) && !(prevUp == LOW)) {
        // increase temperature setting for new button press
        setpoint++;
    } else if ((stateDwn == LOW) && !(prevDwn == LOW)) {
        // decrease temperature setting for new button press
        setpoint--;
    }

    // store button states
    prevUp  = stateUp;
    prevDwn = stateDwn;

    // get the temperature
    // method 1 integer math, loses precision in the typical house range?
    // take in the ADC value, int scale voltage to 5V=500
    // using a TMP36 which is 10mV/C with a +0.5V offset
    //int adc0, tempC
    //float tempF;
    //adc0    = analogRead(tempsens);
    //tempC   = ( ((adc0*2) + 1) * tempscale + 1024) / 2048 - tmp36offset;
    // method 2 use much slower floats
    float voltage, tempC, tempF;
    voltage = analogRead(tempsens)*0.004882814;
    tempC   = (voltage - 0.5) * 100.0;
    tempF   = tempC * (9.0/5.0) + 32.0;
    
    // set the drive on or off
    switch (mode) {
        case cool:
            if (tempF > setpoint) {
              digitalWrite(drive, HIGH);
            } else if (tempF < (setpoint-hysteresis)) {
              digitalWrite(drive, LOW);
            }
            break;
        case heat:
            if (tempF < setpoint) {
                digitalWrite(drive, HIGH);
            } else if (tempF > (setpoint+hysteresis)) {
                digitalWrite(drive, LOW);
            }
            break;
    }   
    
    // print the current state
    //Serial.print(F("adc0: "));
    //Serial.print(adc0);
    Serial.print(F("   voltage: "));
    Serial.print(voltage);
    Serial.print(F("   deg C: "));
    Serial.print(tempC);
    Serial.print(F("   deg F: "));
    Serial.print(tempF);
    Serial.print(F("   setpoint:"));
    Serial.println(setpoint);
    
    // slow the loop slightly
    delay(50); //ms
}

/*float getVoltage(int pin) {
    // this was taken from sparkfun
    return (analogRead(pin) * 0.004882814);
}*/
