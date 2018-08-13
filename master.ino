#include "DHT.h"
#include <LiquidCrystal.h>


#define OFF LOW
#define ON HIGH


//DHT types supported:  DHT11       DHT21 (AM2301)     DHT22  (AM2302, AM2321)
#define OUTTYPE DHT22
#define INTYPE DHT22
#define OUTPIN 2
#define INPIN 3


//Fan stuff
#define FANPIN 5
#define FANTOGGLEDELTA 1
#define DEWPOINTDELTA 4
#define DHTREADFREQUENCY 15000    // read once every 30 sec
#define FANSWITCHFREQUENCY 15000 // switch not more often than once every 30sec

#define LCDREFRESH 1000



uint32_t timerfan = 0;
uint32_t timerdht = 0;
uint32_t timerlcd = 0;
bool fanstatus = false;

DHT outdht(OUTPIN, OUTTYPE);
DHT indht(INPIN, INTYPE);


const int backlight = 6;
const int rs = 8, en = 7, d4 = 12, d5 = 11, d6 = 10, d7 = 9;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);


float humidityOut, humidityIn, tempOut, tempIn, hiOut, hiIn, dewOut, dewIn;

// Calibrate sensors. some seem to be off quiet a bit
float humidityOutOffset = 15.0;
float humidityInOffset = 0.0;



// define crap for ADC converter to read buttons
#define DEBUGBUTTONS    // by defining this we'll get debug serial output for button values


struct button {
  int id;
  int low;  //low threshold on the A0 input
  int high; //high threshold on the A0 input
};

constexpr struct button b1 { 1,  720,  730 };  // select
constexpr struct button b2 { 2,  480,  500 };  // left
constexpr struct button b3 { 3,  145,  155 };  // up
constexpr struct button b4 { 4,  315,  330 };  // down

// Button debounce and ADC converting variables
int timer = 500;
int reading;
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
int tmpButtonState = LOW;    // the current reading from the input pin
unsigned long lastButtonRead = 0;
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned int debounceDelay = 50;    // the debounce time; increase if the output flickers
unsigned int analogReadDelay = 20;

bool b1state = OFF;
bool b2state = OFF;
bool b3state = OFF;
bool b4state = OFF;



void printConstants() {
  Serial.println("fan-control");
  Serial.print("Sensor Polling interval: ");
  Serial.print(DHTREADFREQUENCY);
  Serial.print("\tDewPointDelta: ");
  Serial.print(DEWPOINTDELTA);
  Serial.print("\tFanToggleDelta: ");
  Serial.print(FANTOGGLEDELTA);
  Serial.print("\tFanPin: ");
  Serial.println(FANPIN);
  Serial.print("DHT Sensor OUT: ");
  Serial.print(OUTTYPE);
  Serial.print("\tPin: ");
  Serial.println(OUTPIN);
  Serial.print("DHT Sensor  IN: ");
  Serial.print(INTYPE);
  Serial.print("\tPin: ");
  Serial.println(INPIN);
}

void printDhtSerial() {
  // outdoor values
  Serial.print("Humidity Out: ");
  Serial.print(humidityOut);
  Serial.print(" %\t");
  Serial.print("Temperature Out: ");
  Serial.print(tempOut);
  Serial.print(" *C\t");
  Serial.print("DewPoint Out: ");
  Serial.print(dewOut);
  Serial.print(" *C\t");
  Serial.print("Heat index Out: ");
  Serial.print(hiOut);
  Serial.println(" *C");

  // indoor values
  Serial.print("Humidity  In: ");
  Serial.print(humidityIn);
  Serial.print(" %\t");
  Serial.print("Temperature  In: ");
  Serial.print(tempIn);
  Serial.print(" *C\t");
  Serial.print("DewPoint  In: ");
  Serial.print(dewIn);
  Serial.print(" *C\t");
  Serial.print("Heat index  In: ");
  Serial.print(hiIn);
  Serial.println(" *C");
}


void printDhtLCD() {
  //lcd.clear();

  // outdoor values
  lcd.setCursor(0,0); //Start at character 0 on line 0
  lcd.print("Out: Hum:");
  lcd.print(humidityOut, 1);
  lcd.print("% Temp:");
  lcd.print(tempOut, 1);
  lcd.print(" Dew:");
  lcd.print(dewOut, 1);

  // indoor values
  lcd.setCursor(0,1); //Start at character 0 on line 0
  lcd.print("In:  Hum:");
  lcd.print(humidityIn, 1);
  lcd.print("% Temp:");
  lcd.print(tempIn, 1);
  lcd.print(" Dew:");
  lcd.print(dewIn, 1);
}



float dewPoint(float celsius, float humidity)
{
  // (1) Saturation Vapor Pressure = ESGG(T)
  double RATIO = 373.15 / (273.15 + celsius);
  double RHS = -7.90298 * (RATIO - 1);
  RHS += 5.02808 * log10(RATIO);
  RHS += -1.3816e-7 * (pow(10, (11.344 * (1 - 1/RATIO ))) - 1) ;
  RHS += 8.1328e-3 * (pow(10, (-3.49149 * (RATIO - 1))) - 1) ;
  RHS += log10(1013.246);

        // factor -3 is to adjust units - Vapor Pressure SVP * humidity
  double VP = pow(10, RHS - 3) * humidity;

        // (2) DEWPOINT = F(Vapor Pressure)
  double T = log(VP/0.61078);   // temp var
  return (241.88 * T) / (17.558 - T);
}



void setup() {
  Serial.begin(115200);
  printConstants();

  // init sensors and peripherials
  outdht.begin();
  indht.begin();
  pinMode(FANPIN, OUTPUT);

  // init LCD setup section
  pinMode(backlight, OUTPUT);
  digitalWrite(backlight, ON);

  lcd.begin(16,2);   // initialize the lcd for 16 chars 2 lines, turn on backlight
  lcd.print("HoiHoi");
  lcd.setCursor(0,1);
  lcd.print("debug on serial...");
  delay(2000);
  lcd.clear();

}




void loop() {
  if ( millis() > timerlcd ) {
    lcd.scrollDisplayLeft();
    timerlcd = millis() + LCDREFRESH;
  }


  if ( millis() > timerdht ) {    // switch fan not more than once every FANSWITCHFREQUENCY
    Serial.print("DHT ... ");

    //// Read sensors
    humidityOut = outdht.readHumidity();
    tempOut = outdht.readTemperature();
    humidityIn = indht.readHumidity();
    tempIn = indht.readTemperature();

    // Check if any reads failed and exit early (to try again).
    if (isnan(humidityOut) || isnan(tempOut)) {
      Serial.println("Failed to read Outdoor sensor!");
      return;
    } else if (isnan(humidityIn) || isnan(tempIn)) {
      Serial.println("Failed to read Indoor sensor!");
      return;
    } else {
      Serial.println("OK");
      timerdht = millis() + DHTREADFREQUENCY;

      // Adjust values by constant
      humidityOut += humidityOutOffset;
      humidityIn += humidityInOffset;

      // Compute heat index and dewpoint
      dewOut = dewPoint(tempOut, humidityOut);
      dewIn = dewPoint(tempIn, humidityIn);
      hiOut = outdht.computeHeatIndex(tempOut, humidityOut, false);
      hiIn = indht.computeHeatIndex(tempIn, humidityIn, false);

      printDhtSerial();   //// printing it all on the Serial if we actually got values
      printDhtLCD();      //// print new values on LCD
    }
  
  }




  //// deciding what to do with the FAN
  if ( millis() > timerfan ) {    // switch fan not more than once every FANSWITCHFREQUENCY
    Serial.print("FAN ...");
    timerfan = millis() + FANSWITCHFREQUENCY;
    if ( dewOut < (dewIn - DEWPOINTDELTA - FANTOGGLEDELTA )) {
       // turn on fan if dewpoint outside is < dewpoint inside - the delta 
       digitalWrite(FANPIN, ON);
       Serial.println(" ON");
    } else if ( dewOut > (dewIn - DEWPOINTDELTA + FANTOGGLEDELTA ) ) {
       // turn off fan
       digitalWrite(FANPIN, OFF);
       Serial.println(" OFF");
    } else {
       Serial.println(" unchanged due to flap protection");
    }
  }




  if ((millis() - lastButtonRead) > analogReadDelay ) {
    lastButtonRead = millis();

    reading = analogRead(A0);
    //Serial.println(reading);

    if      (reading>=b1.low && reading<=b1.high) tmpButtonState = b1.id;       //Read switch 1
    else if (reading>=b2.low && reading<=b2.high) tmpButtonState = b2.id;       //Read switch 2
    else if (reading>=b3.low && reading<=b3.high) tmpButtonState = b3.id;       //Read switch 3
    else if (reading>=b4.low && reading<=b4.high) tmpButtonState = b4.id;       //Read switch 4
    else    tmpButtonState = LOW;                                               //No button is pressed;

    if (tmpButtonState != lastButtonState) {
      lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
      buttonState = tmpButtonState;
    }
    lastButtonState = tmpButtonState;


    switch(buttonState){
      case LOW:
        break;
      case b1.id:
        b1state = !b1state;
        break;
    }

#ifdef DEBUGBUTTONS
    if ( reading > 100 ) {
      char buffer[100];
      sprintf(buffer, "reading %d\tbutton: %d\r\n", reading, buttonState);
      Serial.print(buffer);
    }
#endif

  }

}
