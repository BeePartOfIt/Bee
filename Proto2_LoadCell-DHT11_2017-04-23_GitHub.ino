// Created by ME April 2017
// Based on code from Adafruit, Sparkfun and Acrobotics

//  This code takes readings from a DHT11 and Load Cell sensors connected to a HX711 amplifier
//  and posts them to Ubidots.com


/* https://github.com/esp8266/Arduino/issues/793
 *  this is know issue

check gpio0 and 2 not be used or must be hight before reset ,

other things when you flash , if you want test reboot must be cut power and reboot manually , after board reboot normal .

Probably there are a problem when board nodemcu 1.0 is flashed , something not put correct .

*/

#include <DHT.h>              // Library for DHT11 and DHT22
#include <ESP8266WiFi.h>      // Library for ESP to enable network connections
#include <HX711.h>            // Library for HX711 amplifier chip to read weights

#define errorPin 16           // This is the onboard LED for easy debugging
#define DHTPIN D3             // Use ESP pin D3 for temperature and humidity readings
#define DHTTYPE DHT11         // We use here a DHT11 sensor. Change this if you use DHT22 sensor 
#define calibration_factor 30200.0 //This value is obtained using the SparkFun_HX711_Calibration sketch
#define zero_factor -560041   //This large value is obtained using the SparkFun_HX711_Calibration sketch
#define DOUT  D2              // Use ESP pin D2 for weight data readings
#define CLK  D1               // ESP pin D3 is problematic since it also enables Flash mode ... I assume

// Instantiates and initializes 
DHT dht(DHTPIN, DHTTYPE);     // DHT object
HX711 scale(DOUT, CLK);       // HX711 object

//////////////////////////////////////////////////////////////////////////////////////////7
// Define and initialize constants and variables that we'll use later in the code
const int sleep_time = 300;  // Time to sleep (in seconds) between posts to Ubidots
WiFiClient client;

// After creating an account on Ubidots, you'll be able to setup variables where you 
// will store the data. In order to post the measurements to the Ubidots variables,
// we need their "IDs", which are given on the website
String variable_id1 = "57d700d9762542685e8aefd3"; //This is the temperature ID from Ubidots
String variable_id2 = "58e0b7b576254260433a6fd1"; //This is the humidity ID from Ubidots
String variable_id3 = "58e0b7d5762542604e6753e4"; //This is the weight ID from Ubidots

// In addition, we'll need the API token, which is what prevents other users
// Ubidots to publish their data to one of your variables
String token = "X2sHgZ64csB4TjR5w8sCbwb4G9coA9";

// We'll also initialize the values for our Wi-Fi network
const char* ssid = "MExxxxxxx";
const char* password = "6909896264xxxxxxxxxxx";

//////////////////////////////////////////////////////////////////////////////////
// Function Prototypes
void ubiSave_value(String, String);


/////////////SETUP////////////SETUP///////////////SETUP////////////////SETUP////////////////////SETUP////////////////////
// The setup function is executed once by the ESP8266 when it's powered up or reset
void setup()
{
  // Initialize Serial (USB) communication, which will be used for sending debugging messages
  // to the computer
  Serial.begin(9600); delay(200);
  Serial.println();
  Serial.println();
  Serial.println("Inside setup");
  Serial.println("Version DHT_LoadCell2Ubidots_20170305 is running");
  
 pinMode(errorPin, OUTPUT); // sets pin as an output to drive an LED for status indication
  // The following loop flashes the LED four times to indicate we're inside the setup function
  for (int i=0;i<4; i++)
  {
    digitalWrite(errorPin ,HIGH);
    delay(200);
    digitalWrite(errorPin ,LOW);
    delay(200);
  }
    
  
  // Start the communication with the DHT sensor by calling the begin method of the dht object
  dht.begin();
  // Manual delay while the communication with the sensor starts
  delay(200);
  Serial.println("DHT demo started");
  delay(100);
  
  // This value is obtained by using the SparkFun_HX711_Calibration sketch
  scale.set_scale(calibration_factor); 
  scale.set_offset(zero_factor); //Zero out the scale using a previously known zero_factor
  Serial.println("HX711 scale demo stated");
  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);

 // Debug messsages to indicate we're about to connect to the network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Use the scanNetworks method inside the Wi-Fi class to scan for any available Wi-Fi networks
  // nearby. If none are found, go to sleep
  int n = WiFi.scanNetworks();

  Serial.println("scan done"); 
  if (n == 0)
  {
    Serial.println("no networks found");
    Serial.println("Going into sleep");
// ESP.deepSleep(sleep_time * 1000000);
  }

  // If networks are found, attempt to connect to our Wi-Fi network
  WiFi.begin(ssid, password);

  // While the connection is not established, IDLE inside this while loop
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  // Once the connection to our Wi-Fi netowrk is successful, print some debug messages
  Serial.println("");
  Serial.println("Wi-Fi connected");

}

////////////// LOOP ////////////// LOOP //////////// LOOP ///////////// LOOP //////////// LOOP //// LOOP ////
// Main code
void loop()
{
  Serial.println("Inside loop");
 // The following loop flashes the LED two times to indicate we're inside the loop function
  for (int i=0;i<2; i++)
  {
    digitalWrite(errorPin ,HIGH);
    delay(200);
    digitalWrite(errorPin ,LOW);
    delay(200);
  }
  
  // Read the current temperature and humidity measured by the sensor
  float temp = dht.readTemperature(false);
  float hum = dht.readHumidity();
  float weight = scale.get_units(5);    //scale.get_units() returns a float
  Serial.println();
 
  // Call our user-defined function ubiSave_value (defined below), and pass it both the 
  // measurements as well as the corresponding Ubidots variable IDs
  ubiSave_value(String(variable_id1), String(temp));
  ubiSave_value(String(variable_id2), String(hum));
  ubiSave_value(String(variable_id3), String(weight));

  // Send some debug messages over USB
  Serial.println("Ubidots data");
  Serial.println("temperature: "+String(temp));
  Serial.println("humidity: "+String(hum));
  Serial.println("weight: "+String(weight));
  Serial.println(" Going to Sleep for a while !" );
  Serial.println(" -------------------------------------------" );
  Serial.println("" );

  // deepSleep time is defined in microseconds. Multiply seconds by 1e6
  // ESP.deepSleep(sleep_time * 1000000);//one or other
  
  // Wait a few seconds before publishing additional data to avoid saturating the system
   delay(sleep_time*1000);  
}


////////////////////////////////////////////////////////////////////////////////
// User-defined functions
// We encapsulate the grunt work for publishing temperature and humidty values to Ubidots
// inside the function ubiSave_value
void ubiSave_value(String variable_id, String value)
{
  // Prepare the value that we're to send to Ubidots and get the length of the entire string
  // that's being sent
  String var = "{\"value\": " + value +"}"; // We'll pass the data in JSON format
  String length = String(var.length());

  // If we get a proper connection to the Ubidots API
  if (client.connect("things.ubidots.com", 80))
  {
    Serial.println("Connected to Ubidots...");
    delay(100);

    // Construct the POST request that we'd like to issue
    client.println("POST /api/v1.6/variables/"+variable_id+"/values HTTP/1.1");
    // We also use the Serial terminal to show how the POST request looks like
    Serial.println("POST /api/v1.6/variables/"+variable_id+"/values HTTP/1.1");
    // Specify the contect type so it matches the format of the data (JSON)
    client.println("Content-Type: application/json");
    Serial.println("Content-Type: application/json");
    // Specify the content length
    client.println("Content-Length: "+ length);
    Serial.println("Content-Length: "+ length);
    // Use our own API token so that we can actually publish the data
    client.println("X-Auth-Token: "+ token);
    Serial.println("X-Auth-Token: "+ token);
    // Specify the host
    client.println("Host: things.ubidots.com\n");
    Serial.println("Host: things.ubidots.com\n");
    // Send the actual data
    client.print(var);
    Serial.print(var+"\n");
  }
  else
  {
    // If we can't establish a connection to the server:
    Serial.println("Ubidots connection failed...");
  }
  
  // If our connection to Ubidots is healthy, read the response from Ubidots
  // and print it to our Serial Monitor for debugging!
  while (client.available())
  {
    char c = client.read();
    Serial.print(c);
  }
  
  // Done with this iteration, close the connection.
  if (client.connected())
  {
    Serial.println("Disconnecting from Ubidots...");
    client.stop();
  }
}

