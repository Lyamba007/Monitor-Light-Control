#include <ESP8266WiFi.h> 
#include <DNSServer.h>  
#include <ESP8266WebServer.h> 
#include <FastLED.h>	   //Special library for WS2812 RGB LED configuration

//----------------------SETTINGS-----------------------//
#define ANALOG_PIN A0	     //Analog pin for led strip brightness configuration 
#define DI_PIN 4             //Din Pin on led strip

#define OFF_TIME 10          //Time in sec for system connection waiting 
#define VOLTAGE	 5           //Voltage that led strip supports
#define CURRENT_LIMIT 2000   //2000 mA power supply limitation for brightness configuration

#define NUM_LEDS 50          //LEDS count in strip (16+9)*2
#define AUTO_BRIGHT 1        //Brightness configure with ambient light level
#define MAX_BRIGHT 255       //MAX brightness
#define MIN_BRIGHT 50        //MIN brightness
#define BRIGHT_CONSTANT 500  //Gain from ambient light (0-1023). The smaller value, the faster brightness will be added
#define COEF 0.9             //Brightness changing filter coefficient. The bigger value, the slower brightness changing

#define SERIAL_RATE 115200   //Baudrate
//----------------------END SETTINGS-----------------------//

uint8_t currentBright, newBright; 	   //variables for automatic brightness changing (adaptive mode)
unsigned long bright_timer, off_timer; //variables for keeping time for adaptive brightness mode and system connection 

uint8_t prefix[] = {'A', 'd', 'a'}, hi, lo, chk, i; //Set the passphrase for connection

bool led_state = true;  //LED strip state

uint8_t isEnabled = 0; //Activation button state signal (Connection with Mobile application)
uint8_t red   = 0;	   //Get the red   light value (R[0-255])
uint8_t green = 0; 	   //Get the green light value (G[0-255])
uint8_t blue  = 0;	   //Get the blue  light value (B[0-255])

const char *ssid    = "MonitorLight"; //The acces point (AP) name 
const byte DNS_PORT = 53;  			  //Select the DNS port value

IPAddress apIP(192, 168, 1, 1);		  //Set the static IP address	
DNSServer dnsServer; 				  //Create the DNS instance
ESP8266WebServer webServer(80);       //Set the port 80 for the web server

CRGB leds[NUM_LEDS];  				  //Create the LED strip instance (LEDS count)


void handleRoot() //Request handler
{
  isEnabled = webServer.arg(0).toInt();	//Read the Mobile client button value (0-1)
  red       = webServer.arg(1).toInt(); //Read the red   light value     (R[0-255])
  green     = webServer.arg(2).toInt(); //Read the green light value     (G[0-255])
  blue      = webServer.arg(3).toInt(); //Read the blue  light value     (B[0-255])
  
  webServer.send(200, "text/plain", "OK"); //Send the code 200(the request has been processed) to HTTP-Client 
}


void setup()
{  
  WiFi.mode(WIFI_AP); //Select the mode of AP
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0)); //Set the IP and subnet mask 
  WiFi.softAP(ssid);  //Create the AP by name

  webServer.on("/", handleRoot); //Initiate the server request-responce with created handler 
  webServer.begin();  //Start the server
   
  FastLED.addLeds<WS2812, DI_PIN, GRB>(leds, NUM_LEDS);  //LED strip initialization
  if (CURRENT_LIMIT > 0) 
	  FastLED.setMaxPowerInVoltsAndMilliamps(VOLTAGE, CURRENT_LIMIT); //Set the voltage and current limitations (power supply choosing due to LED strip Vcc and LEDs count)
}


void check_connection() //All system connection check function (LED strip to PC, PC application, WiFi connection)
{
  if (led_state) 
  {
	//if led_strip is not connected after 10 seconds it goes to be disconnected. 
	//The system should be restarted 
	//
    if ( (millis() - off_timer) > (OFF_TIME * 1000) )  
	{
      led_state = false;
      FastLED.clear();
      FastLED.show();
    }
  }
}


void SetLedColor() //Set the own color for each led in strip
{
  for(int i = 0; i < NUM_LEDS; i++)
    leds[i].setRGB(red, green, blue);
}


void loop() 
{
  dnsServer.processNextRequest(); //Ability to process a request when it is received
  webServer.handleClient(); 	  //Request processing
 
  //Adaptive bright mode
  //
  if (AUTO_BRIGHT) 
  {                      
    if (millis() - bright_timer > 100)  
    {
      bright_timer = millis();               //Reset the timer
      currentBright = map(analogRead(ANALOG_PIN), 0, BRIGHT_CONSTANT, MIN_BRIGHT, MAX_BRIGHT); //Get the value from photoresistor and convert it to ADC values
      currentBright = constrain(currentBright, MIN_BRIGHT, MAX_BRIGHT); //If the value smaller or bigger than defines, it should be restricted to appropriate range
      newBright = newBright * COEF + currentBright * (1 - COEF); //Set brightness formulae
      LEDS.setBrightness(newBright);        //Set the new brightness value for LED strip
    }
  }

  if (!led_state) 
  {
    led_state = true; 
  }
  off_timer = millis(); //start the disconnection timer if the led_state does not connect

  if(isEnabled) //Interrupt the automatic light changing via PC application and set the control to Mobile app
  {
    Serial.end();
    SetLedColor();
  }
  
  //Ambibox (PC application) work mode
  //
  else
  { 
    Serial.begin(serialRate);
    Serial.print("Ada\n"); //Show the code word to recognize it
    
	//Try to parse the code word
	//
    for (i = 0; i < sizeof prefix; ++i) 
    {
      waitLoop: while (!Serial.available()) check_connection();;
      if (prefix[i] == Serial.read()) 
      {
        continue;
      }
      i = 0;
      goto waitLoop;
    }
    while (!Serial.available()) check_connection();;
    hi = Serial.read();
    while (!Serial.available()) check_connection();;
    lo = Serial.read();
    while (!Serial.available()) check_connection();;
    chk = Serial.read();
    if (chk != (hi ^ lo ^ 0x55))
    {
      i = 0;
      goto waitLoop;
    }
	
	//Set the memory block for LED strip. Each LED is the structure of R, G and B colors 	
	//
    memset(leds, 0, NUM_LEDS * sizeof(struct CRGB));
  
    //LED strip read write processing block from PC application 
	//
    for (uint8_t i = 0; i < NUM_LEDS; i++) 
    {
      uint8_t r, g, b;
	  
      //Read the data for each color
	  //
      while (!Serial.available()) check_connection();
      r = Serial.read();
      while (!Serial.available()) check_connection();
      g = Serial.read();
      while (!Serial.available()) check_connection();
      b = Serial.read();
	  
	  //Set the colors to each LED in strip
	  //
      leds[i].r = r;
      leds[i].g = g;
      leds[i].b = b;
    }
  }
  
  FastLED.show(); //Flash the LED strip with current colors 
}
