#define ETHERNET 1


#include <dht.h>
#if ETHERNET
#include <SPI.h>
#include <EtherCard.h>
#endif

dht DHT;

#define OFF LOW
#define ON HIGH


#define FANTOGGLEDELTA 1
#define DEWPOINTDELTA 5
#define DHTREADFREQUENCY 20000    // read once every 20 sec
#define DOORDHT 2
#define REARDHT 3
#define FRONTDHT 4

#define FANSWITCHFREQUENCY 120000 // switch not more often than once every 2 min
#define FAN 5




#if ETHERNET
byte Ethernet::buffer[500];
static const byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };
//static byte ip[] = {192, 168, 10, 200};
//const byte dstip[] = {192, 168, 10, 2};
const char destination[] PROGMEM = "loxone";
const unsigned int srcport PROGMEM = 1234; 
const unsigned int dstport PROGMEM = 1234; 
#endif


static uint32_t timerdht = 0;
static uint32_t timerfan = 0;
double doorhumidity;
double doortemperature;
double doordewpoint;
double fronthumidity;
double fronttemperature;
double frontdewpoint;
double rearhumidity;
double reartemperature;
double reardewpoint;










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









void printline(char *sensor, double hum, double temp, double dew) {
    Serial.print("\t");
    Serial.print(sensor);
    Serial.println(":");
    Serial.print("\t\tHumidity (%): ");
    Serial.print((float)hum, 2);
    Serial.print("\tTemperature (C): ");
    Serial.print((float)temp, 2);
    Serial.print("\tDewPoint (C): ");
    Serial.println((float)dew, 2);
}








void setup()
{
  Serial.begin(115200);
  Serial.println("booting up...");
  pinMode(FAN, OUTPUT);

#if ETHERNET
  Serial.println("init ethernet...");
  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0)
    Serial.println(F("Failed to access Ethernet controller"));
  if (!ether.dhcpSetup())
    Serial.println(F("DHCP failed"));

  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);
  ether.printIp("DNS: ", ether.dnsip);

  if (!ether.dnsLookup(destination))
    Serial.println("DNS failed");

  ether.printIp("SRV: ", ether.hisip);

#endif

}






void loop()
{

#if ETHERNET
  ether.packetLoop(ether.packetReceive());
#endif

  if ( millis() > timerdht ) {    // read DHT not more than once every DHTREADFREQUENCY
    Serial.println("reading DHT...");
    timerdht = millis() + DHTREADFREQUENCY;

    // reading the DHTs
    if ( DHT.read(DOORDHT) == DHTLIB_OK ) {        // known return states:   DHTLIB_ERROR_CHECKSUM   DHTLIB_ERROR_TIMEOUT    DHTLIB_OK
      doorhumidity    = DHT.humidity;
      doortemperature = DHT.temperature;
      doordewpoint    = dewPoint(doortemperature, doorhumidity);
    }
    if ( DHT.read(FRONTDHT) == DHTLIB_OK ) {        // known return states:   DHTLIB_ERROR_CHECKSUM   DHTLIB_ERROR_TIMEOUT    DHTLIB_OK
      fronthumidity    = DHT.humidity;
      fronttemperature = DHT.temperature;
      frontdewpoint    = dewPoint(fronttemperature, fronthumidity);
    }
    if ( DHT.read(REARDHT) == DHTLIB_OK ) {        // known return states:   DHTLIB_ERROR_CHECKSUM   DHTLIB_ERROR_TIMEOUT    DHTLIB_OK
      rearhumidity    = DHT.humidity;
      reartemperature = DHT.temperature;
      reardewpoint    = dewPoint(reartemperature, rearhumidity);
    }

    printline("door", doorhumidity, doortemperature, doordewpoint);
    printline("front", fronthumidity, fronttemperature, frontdewpoint);
    printline("rear", rearhumidity, reartemperature, reardewpoint);


#if ETHERNET
    char msg[50];
    sprintf(msg, "Humidity Front %.1f", rearhumidity);
    Serial.println(msg);
    ether.sendUdp(msg, sizeof(msg), srcport, ether.hisip, dstport);
#endif


  }

  if ( millis() > timerfan ) {    // read DHT not more than once every FANSWITCHFREQUENCY
    Serial.print("FAN ...");
    timerfan = millis() + FANSWITCHFREQUENCY;
    if ( doordewpoint < (reardewpoint - DEWPOINTDELTA - FANTOGGLEDELTA )) {
       // turn on fan if dewpoint outside is < dewpoint inside - the delta 
       digitalWrite(FAN, ON);
       Serial.println(" ON");
    } else if ( doordewpoint > (reardewpoint - DEWPOINTDELTA + FANTOGGLEDELTA ) ) {
       // turn off fan
       digitalWrite(FAN, OFF);
       Serial.println(" OFF");
    }
  }

}
