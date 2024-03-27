#include <Arduino.h>
#include <TM1637Display.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Module connection pins (Digital Pins)
#define CLK 3
#define DIO 2
#define BUTTON_START 6
#define SW_CHOICE 5
#define RELAIS 4
#define ONE_WIRE 7

#define TEMPERATURE_PRECISION 9 // Lower resolution

#define STATE_TEMP false
#define STATE_TIME true
#define DEBUG true


// Create degree Celsius symbol:
const uint8_t celsius[] = {
  SEG_A | SEG_B | SEG_F | SEG_G,  // Circle
  SEG_A | SEG_D | SEG_E | SEG_F   // C
};

TM1637Display display(CLK, DIO);
OneWire tempSens(ONE_WIRE);

DallasTemperature sensor_temp(&tempSens);

int poti = A0;
int time_goal = 0;
int time_is = 0;
int time_left = 0;
int mintime = 0;
int maxtime = 10*60;
bool running = false;
bool state;
unsigned long starttime;

int temp_is = 0;
int temp_goal = 0;

int getmin(int nb){
  return nb / 60;
}

int getsec(int nb){
  return (nb - getmin(nb)*60);
}

void start_run(){
  running = true;
  display.clear();
  if(state == STATE_TIME){
    digitalWrite(RELAIS,HIGH);
    starttime = millis();
  }
  if(DEBUG) Serial.println("Start Run");
  while(!digitalRead(BUTTON_START)) {
    delay(10);
    if(DEBUG) Serial.println(" Delayloop");
  }
}

void sensor_debug(){
    // Grab a count of devices on the wire
  int numberOfDevices = sensor_temp.getDeviceCount();
  DeviceAddress tempDeviceAddress; // We'll use this variable to store a found device address

  // locate devices on the bus
  Serial.print("Locating devices...");
  
  Serial.print("Found ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensor_temp.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");
  
  // Loop through each device, print out address
  for(int i=0;i<numberOfDevices; i++)
  {
    // Search the wire for address
    if(sensor_temp.getAddress(tempDeviceAddress, i))
	{
		Serial.print("Found device ");
		Serial.print(i, DEC);
		Serial.print(" with address: ");
		printAddress(tempDeviceAddress);
		Serial.println();
		
		Serial.print("Setting resolution to ");
		Serial.println(TEMPERATURE_PRECISION, DEC);
		
		// set the resolution to TEMPERATURE_PRECISION bit (Each Dallas/Maxim device is capable of several different resolutions)
		sensor_temp.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);
		
		Serial.print("Resolution actually set to: ");
		Serial.print(sensor_temp.getResolution(tempDeviceAddress), DEC); 
		Serial.println();
	}else{
		Serial.print("Found ghost device at ");
		Serial.print(i, DEC);
		Serial.print(" but could not detect address. Check power and cabling");
	}
  }
}

// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  // method 1 - slower
  //Serial.print("Temp C: ");
  //Serial.print(sensors.getTempC(deviceAddress));
  //Serial.print(" Temp F: ");
  //Serial.print(sensors.getTempF(deviceAddress)); // Makes a second call to getTempC and then converts to Fahrenheit

  // method 2 - faster
  float tempC = sensor_temp.getTempC(deviceAddress);
  if(tempC == DEVICE_DISCONNECTED_C) 
  {
    Serial.println("Error: Could not read temperature data");
    return;
  }
  Serial.print("Temp C: ");
  Serial.print(tempC);
  Serial.print(" Temp F: ");
  Serial.println(DallasTemperature::toFahrenheit(tempC)); // Converts tempC to Fahrenheit
}


// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void check_time(){
  if(running && state == STATE_TIME){
    time_is = (millis()-starttime)/1000;
    time_left = time_goal - time_is;
    if(DEBUG){
      Serial.print(time_is);
      Serial.print(" --> ");
      Serial.println(time_goal);
    }
    if(time_is >= time_goal){ 
      end_run();
    }
  }
  if(running && state == STATE_TEMP){
    sensor_temp.requestTemperatures();
    temp_is = (int)sensor_temp.getTempCByIndex(0);
    if(DEBUG){
      Serial.print(temp_is);
      Serial.print(" --> ");
      Serial.println(temp_goal);
    }
    if(temp_is <= temp_goal){
      digitalWrite(RELAIS,HIGH);
      if(DEBUG) Serial.println(" Relais ein");
    }
    else{
      digitalWrite(RELAIS,LOW);
      if(DEBUG) Serial.println(" Relais aus");
    }
  }
}

void end_run(){
  running = false;
  display.clear();
  digitalWrite(RELAIS,LOW);
  if(DEBUG) Serial.println("End Run");
  while(!digitalRead(BUTTON_START)) {
    delay(10);
    if(DEBUG) Serial.println(" Delayloop");
  }
}

void update_pot(){
  if(!running){
    if(state == STATE_TIME){
        float meas = 0;
        int nb_meas = 50;
        for(int i = 0; i < nb_meas; ++i)
        {
          meas += (float)analogRead(poti);
          delay(1);
        }
        meas /= nb_meas;
        
        //pres = (int)(maxtime-mintime)/1024*pot_time+mintime;
        time_goal = (int)((maxtime-mintime)/1024.0 * (1024-(float)meas) + (float)mintime);
        //Serial.println(time_goal);
    }
    if(state == STATE_TEMP){
        float meas = 0;
        int nb_meas = 25;
        for(int i = 0; i < nb_meas; ++i)
        {
          meas += (float)analogRead(poti);
          delay(1);
        }
        meas /= nb_meas;
        
        temp_goal = (int)(99.0/1024.0 * (1024-(float)meas));
        //Serial.println(temp_goal);
    }
  }
}

void update_buttons(){
  bool tmp_start = digitalRead(BUTTON_START);
  bool tmp_mode = digitalRead(SW_CHOICE);

  if(DEBUG && 0){
    Serial.print("Running: ");
    Serial.print(running);
    Serial.print(" Start: ");
    Serial.print(tmp_start);
    Serial.print(" State: ");
    Serial.println(tmp_mode);
  }
    if (!tmp_start && !running) {
      start_run();
    }
    //tmp_start = digitalRead(BUTTON_START);
    else if (!tmp_start && running) {
      end_run();
    }
    if(tmp_mode && !running){
      state = STATE_TEMP;
    }
    if(!tmp_mode && !running){
      state = STATE_TIME;
    }
}

void state_show(){
  if(state == STATE_TEMP) Serial.print("Temperature ");
  else Serial.print("Time ");
  if(running) Serial.print("Run ");
  else Serial.print("Wait ");
  Serial.print("Relais: ");
  Serial.println(digitalRead(RELAIS));
}

void lcdwrite(){
  if(running && state == STATE_TIME){
    int displaytime = (getmin(time_left) * 100) + getsec(time_left);
    display.showNumberDecEx(displaytime, 0b11100000, true);
    display.setBrightness(0x01);
  }
  if(!running && state == STATE_TIME){
    int displaytime = (getmin(time_goal) * 100) + getsec(time_goal);
    display.showNumberDecEx(displaytime, 0b11100000, true);
    display.setBrightness(0x02);
  }
  if(running && state == STATE_TEMP){
    display.showNumberDec(temp_is, false, 2, 0);
    display.setSegments(celsius, 2, 2);
    display.setBrightness(0x03);
  }
  if(!running && state == STATE_TEMP){
    display.showNumberDec(temp_goal, false, 2, 0);
    display.setSegments(celsius, 2, 2);
    display.setBrightness(0x07);
  }
}

void setup() {
  // Debugging output
  Serial.begin(9600);
  //sensor_temp.begin();
  //if(DEBUG){sensor_debug();}
  pinMode(SW_CHOICE, INPUT);
  pinMode(BUTTON_START, INPUT);
  pinMode(RELAIS, OUTPUT);
  display.setBrightness(0x02); 
  display.clear();
}

void loop() {
  //if(DEBUG) state_show();
  update_pot();
  update_buttons();
  check_time();
  lcdwrite();
}
