/////////////////////////////////////
/// DHT code and helper functions ///
/////////////////////////////////////


// dewPoint function NOAA
// reference (1) : http://wahiduddin.net/calc/density_algorithms.htm
// reference (2) : http://www.colorado.edu/geography/weather_station/Geog_site/about.htm
double dewPoint(double celsius, double humidity)
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





////////////////////
/// global stuff ///
////////////////////

#include <dht.h>
//#include <SPI.h>
//#include <EtherCard.h>

dht DHT;
#define DHTREADFREQUENCY 20000    // read once every 20 sec
#define DOORDHTPIN 2
#define REARDHTPIN 3
#define FRONTDHTPIN 4
#define FANPIN 5

//static byte mac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };
//static byte ip[] = {192, 168, 10, 200};

long timerdht = 0;
double doorhumidity;
double doortemperature;
double doordewpoint;
double fronthumidity;
double fronttemperature;
double frontdewpoint;
double rearhumidity;
double reartemperature;
double reardewpoint;


#define FANTOGGLEDELTA 1
#define DEWPOINTDELTA 5

//////////////////////
/// main functions ///
//////////////////////


void setup()
{
  Serial.begin(115200);
  Serial.println("booting up...");
}

void loop()
{

  if ( millis() - timerdht >= DHTREADFREQUENCY ) {    // read DHT not more than once every DHTREADDELAY
    Serial.println("reading DHT...");
    timerdht = millis();

    if ( DHT.read(DOORDHTPIN) == DHTLIB_OK ) {        // known return states:   DHTLIB_ERROR_CHECKSUM   DHTLIB_ERROR_TIMEOUT    DHTLIB_OK
      doorhumidity    = DHT.humidity;
      doortemperature = DHT.temperature;
      doordewpoint    = dewPoint(doortemperature, doorhumidity);
    }
    if ( DHT.read(FRONTDHTPIN) == DHTLIB_OK ) {        // known return states:   DHTLIB_ERROR_CHECKSUM   DHTLIB_ERROR_TIMEOUT    DHTLIB_OK
      fronthumidity    = DHT.humidity;
      fronttemperature = DHT.temperature;
      frontdewpoint    = dewPoint(fronttemperature, fronthumidity);
    }
    if ( DHT.read(REARDHTPIN) == DHTLIB_OK ) {        // known return states:   DHTLIB_ERROR_CHECKSUM   DHTLIB_ERROR_TIMEOUT    DHTLIB_OK
      rearhumidity    = DHT.humidity;
      reartemperature = DHT.temperature;
      reardewpoint    = dewPoint(reartemperature, rearhumidity);
    }

    if ( doordewpoint < (reardewpoint - DEWPOINTDELTA - FANTOGGLEDELTA )) {
       // turn on fan if dewpoint outside is < dewpoint inside - the delta 

    } else if ( doordewpoint > (reardewpoint - DEWPOINTDELTA + FANTOGGLEDELTA ) ) {
       // turn off fan

    }

    Serial.println("\tDoor: ");
    Serial.print("\t\tHumidity (%): ");
    Serial.print((float)doorhumidity, 2);
    Serial.print("\tTemperature (C): ");
    Serial.print((float)doortemperature, 2);
    Serial.print("\tDewPoint (C): ");
    Serial.println((float)doordewpoint, 2);

    Serial.println("Front: ");
    Serial.print("\t\tHumidity (%): ");
    Serial.print((float)fronthumidity, 2);
    Serial.print("\tTemperature (C): ");
    Serial.print((float)fronttemperature, 2);
    Serial.print("\tDewPoint (C): ");
    Serial.println((float)frontdewpoint, 2);

    Serial.println("Rear: ");
    Serial.print("\t\tHumidity (%): ");
    Serial.print((float)rearhumidity, 2);
    Serial.print("\tTemperature (C): ");
    Serial.print((float)reartemperature, 2);
    Serial.print("\tDewPoint (C): ");
    Serial.println((float)reardewpoint, 2);
  }

  sleep(1000)




  // END DHT section

}



