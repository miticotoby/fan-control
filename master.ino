#include "DHT.h"


#define OFF LOW
#define ON HIGH



//#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
#define OUTTYPE DHT22
#define INTYPE DHT22
//#define INTYPE DHT11

#define OUTPIN 2
#define INPIN 3


//Fan stuff
#define FANSWITCHFREQUENCY 5000 // switch not more often than once every 30sec
#define FANPIN 5

#define FANTOGGLEDELTA 2
#define DEWPOINTDELTA 5
#define DHTREADFREQUENCY 5000    // read once every 30 sec


static uint32_t timerfan = 0;


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




DHT outdht(OUTPIN, OUTTYPE);
DHT indht(INPIN, INTYPE);



void setup() {
  Serial.begin(115200);
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

  pinMode(FANPIN, OUTPUT);
  outdht.begin();
  indht.begin();
}

void loop() {

  float humidityOut = outdht.readHumidity();
  float tempOut = outdht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidityOut) || isnan(tempOut)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Compute heat index in Celsius (isFahreheit = false)
  float hiOut = outdht.computeHeatIndex(tempOut, humidityOut, false);
  float dewOut = dewPoint(tempOut, humidityOut);


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



  float humidityIn = indht.readHumidity();
  float tempIn = indht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidityIn) || isnan(tempIn)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Compute heat index in Celsius (isFahreheit = false)
  float hiIn = indht.computeHeatIndex(tempIn, humidityIn, false);
  float dewIn = dewPoint(tempIn, humidityIn);


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
    }
  }


  delay(DHTREADFREQUENCY);

}
