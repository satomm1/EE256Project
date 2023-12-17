#include <WiFi.h>
#include <WebServer.h>
#include <SPI.h>
#include <ESP32SPISlave.h>
#include "SmartPotHTML.h"   // .h file that stores html code

#define ROOM058_WIFI

#ifdef ROOM058_WIFI
/* Network Details */
const char* ssid = "EE-IoT";  // Enter SSID here
const char* password = " 350Serra";  //Enter Password here
#endif



// the XML array size needs to be bigger that your maximum expected size
char XML[2048];

// just some buffer holder for char operations
char buf[32];

// variable for the IP reported once connected to LAN
IPAddress Actual_IP;

// Create the webserver
WebServer server(80);

// Create the SPI Slave
ESP32SPISlave spi_slave;

static constexpr uint32_t BUFFER_SIZE {6};
uint8_t spi_slave_tx_buf[BUFFER_SIZE];
uint8_t spi_slave_rx_buf[BUFFER_SIZE];

bool Unit_Select = false;
bool Unit_Val = false;
bool Threshold_Select = false;
bool Threshold_Val = false;
bool Water_Press = false;

uint8_t current_temp = 0;
uint8_t current_soil_moisture = 0;
bool unit_change = false;
uint8_t unit_change_status = 0;
bool threshold_change = false;
uint8_t threshold_change_status = 0;
uint8_t water_change_status = 0;
uint8_t received_data = 0;
/********************************************************************
setup()

The setup code for the ESP32, runs once at startup

Parameters: None

Returns: None

********************************************************************/
void setup() {

  // Get UART started to communicate with computer
  Serial.begin(115200);
  delay(1000);

  Serial.println("Connecting to ");
  Serial.println(ssid);

  //connect to your local wi-fi network
  WiFi.begin(ssid, password);

  //check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
  delay(1000);
  Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");  Serial.println(WiFi.localIP());

  // if  web page or XML are large, may not get a call back from the web page
  // and the ESP32 will think something has locked up and reboot the ESP
  // disable watch dog timer 0 so this doesn't happen
  disableCore0WDT();

  // disable watch dog timer 1 as well
  disableCore1WDT();

  // Serial Update
  Serial.println("starting server");

  // Connect to the LAN
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("IP address: "); Serial.println(WiFi.localIP());
  Actual_IP = WiFi.localIP(); // Save the assigned IP address

  printWifiStatus();

  // these calls will handle data coming back from your web page
  // this one is a page request, upon ESP getting / string the web page will be sent
  server.on("/", SendWebsite);

  // upon esp getting /XML string, ESP will build and send the XML, this is how we refresh
  // just parts of the web page
  server.on("/xml", SendXML);

  // upon ESP getting /BUTTON_0 string, ESP will execute the ProcessButton_0 function, etc.
  // Need some javascript in the web page to send these strings
  // this process is documented in SmartPotHTML.h
  server.on("/FAHRENHEIT_BUTTON", ProcessFahrenheitButton);
  server.on("/CELSIUS_BUTTON", ProcessCelsiusButton);
  server.on("/LOW_THRESHOLD_BUTTON", ProcessLowThresholdButton);
  server.on("/HIGH_THRESHOLD_BUTTON", ProcessHighThresholdButton);
  server.on("/WATER_BUTTON", ProcessWaterButton);

  // finally begin the server
  server.begin();


  // Set up the SPI
  spi_slave.setDataMode(SPI_MODE0);
  spi_slave.begin(VSPI); // Use VSPI, Default should be CS:  5, CLK: 18, MOSI: 23, MISO: 19

  // clear buffers
  memset(spi_slave_tx_buf, 0, BUFFER_SIZE);
  memset(spi_slave_rx_buf, 0, BUFFER_SIZE);

}

/********************************************************************
loop()

The main loop of the code, handles sending data via the web server

Parameters: None

Returns: None

********************************************************************/
void loop() {
 
  // Everytime we receive an update via SPI we process it and and send to server
  // block until the transaction comes from master
    spi_slave.wait(spi_slave_rx_buf, spi_slave_tx_buf, BUFFER_SIZE);

    // if transaction has completed from master,
    // available() returns the number of completed transactions,
    // and `spi_slave_rx_buf` is automatically updated
    while (spi_slave.available()) {
        // do something with `spi_slave_rx_buf`
        // Serial.print("Command Received: ");
        // Serial.println(spi_slave_rx_buf[0]);
        // Serial.println(spi_slave_rx_buf[1]);

        // received_data = spi_slave_rx_buf[0];
        // current_temp = received_data;

        uint8_t buf0 = spi_slave_rx_buf[0];
        uint8_t buf1 = spi_slave_rx_buf[1];
        uint8_t buf2 = spi_slave_rx_buf[2];
        uint8_t buf3 = spi_slave_rx_buf[3];
        uint8_t buf4 = spi_slave_rx_buf[4];
        uint8_t buf5 = spi_slave_rx_buf[5];

        if ((buf0 == buf3) && buf0 > 0) {
          current_temp = buf0;
          Serial.print("Updated Temp: ");
          Serial.println(buf0);
        }

        if ((buf1 == buf4) && buf1 > 0) {
          current_soil_moisture = buf1;
          Serial.print("Updated Soil Moisture: ");
          Serial.println(buf1);
        }

        if ((buf2 == buf5) && buf2) {
          if (0b00000001 & buf2) {
            Serial.println("Threshold Update");
            threshold_change = true;
            threshold_change_status = ((0b00000010 & buf2) > 0)+1;
          }

          if (0b10000000 & buf2) {
            Serial.println("Unit Update");
            unit_change = true;
            if (0b01000000 & buf2) {
              unit_change_status = 2;
            } else {
              unit_change_status = 1;
            }
          } else {
            unit_change_status = 0;
          }

          // if (0b00100000 & buf2) {
          //   if (0b00010000 & buf2) {
          //     water_change_status = 2;
          //   } else {
          //     water_change_status = 1;
          //   }
          // } else {
          //   water_change_status = 0;
          // }

        }

        // for (size_t i=0; i < BUFFER_SIZE; ++i) {
        //   printf("%d ", spi_slave_rx_buf[i]);
        // }
        // printf("\n");
        // if (received_data >= 128) {
        //   current_temp = received_data - 128;
        // } else {
        //   if (0b00000010 | received_data) {
        //     unit_change = true;
        //     unit_change_status = 0b00000001 | received_data + 1;
        //   } else if (0b00001000 | received_data) {
        //     threshold_change = true;
        //     threshold_change_status = (0b00000100 | received_data)/4 + 1;
        //   }
        // }
        
        spi_slave.pop();
    }
  
  // Must call handleClient to give webpage instructions to do something
  server.handleClient();

  // Update the data to transmit to the master
  spi_slave_tx_buf[0] = 0b10000000;

  if (Unit_Select) {
    spi_slave_tx_buf[0] = spi_slave_tx_buf[0] | 0b10000010; // Set bit 2 high to indicate unit update
    if (Unit_Val){
      spi_slave_tx_buf[0] = spi_slave_tx_buf[0] | 0b10000001; // Set bit 1 high to indicate Fahrenheit, otherwise celsius
    }
    Unit_Select = false; // Don't want to continuously send unit updates
  }

  if (Threshold_Select) {
    // spi_slave_tx_buf[0] = spi_slave_tx_buf[0] | 0b10000010; // Set bit 2 high to indicate High Threshold
    spi_slave_tx_buf[0] = spi_slave_tx_buf[0] | 0b10010000; // Set bit 5 high to indicate unit update
    if (Threshold_Val){
      spi_slave_tx_buf[0] = spi_slave_tx_buf[0] | 0b10011000; // Set bit 4 high to indicate High threshold, otherwise low threshold
    }
    Threshold_Select = false; // Don't want to continuously send threshold updates
    Serial.print("Threshold Signal: ");
    Serial.print(spi_slave_tx_buf[0]);
  }

  if (Water_Press) {
    spi_slave_tx_buf[0] = spi_slave_tx_buf[0] | 0b10000100; // Set bit 3 high to indicate we should water
    Water_Press = false; // Don't want to continuously send water presses
  }
  
  // TODO: Send buf[1] to provide reliability
}


// Sends the main webpage
// PAGE_MAIN is a large char defined in SmartPotHTML.h
void SendWebsite() {
  Serial.println("sending web page");
  
  server.send(200, "text/html", PAGE_MAIN); // May need larger than 200 if page is large
}

// Sends the XML
// I avoid string data types at all cost hence all the char mainipulation code
void SendXML() {

  // Serial.println("sending xml");

  strcpy(XML, "<?xml version = '1.0'?>\n<Data>\n");

  // send Temp info
  sprintf(buf, "<TEMP>%d</TEMP>\n", current_temp);
  strcat(XML, buf);

  // send Soil info
  sprintf(buf, "<SOIL>%d</SOIL>\n", current_soil_moisture);
  strcat(XML, buf);

  sprintf(buf, "<Unit>%d</Unit>\n", unit_change_status);
  strcat(XML, buf);
  unit_change_status = 0;

  // sprintf(buf, "<low_water>%d</low_water>\n", water_change_status);
  // strcat(XML, buf);
  // water_change_status = 0;
  
  if (threshold_change) {
    sprintf(buf, "<Water_Level>%d</Water_Level>\n", threshold_change_status);
    strcat(XML, buf);
    threshold_change = false;
  } else {
    strcat(XML, "<Water_Level>0</Water_Level>\n");
  }

  strcat(XML, "</Data>\n");

  // For debuggging, May need to increase size of XML[] if too large
  // Serial.println(XML);

  // Send the data
  server.send(200, "text/xml", XML); // May need larger than 200 if data is large
}

// Process what happens when the fahrenheit button is pressed
void ProcessFahrenheitButton() {

  // Edit SPI message to the PIC32
  Serial.println("Fahrenheit Button Pressed");
  Unit_Select = true;
  Unit_Val = true;

  // keep page live but dont send any thing
  server.send(200, "text/plain", ""); // Send web page
}

// Process what happens when celsius button is pressed
void ProcessCelsiusButton() {

  // Edit SPI message to the PIC32
  Serial.println("Celsius Button Pressed");
  Unit_Select = true;
  Unit_Val = false;

  // keep page live but dont send any thing
  server.send(200, "text/plain", ""); // Send web page
}

// Process what happens when the low threshold button is pressed
void ProcessLowThresholdButton() {

  // Edit SPI message to the PIC32
  Serial.println("Low Threshold Button Pressed");
  Threshold_Select = true;
  Threshold_Val = false;

  // keep page live but dont send any thing
  server.send(200, "text/plain", ""); // Send web page
}

// Process what happens when the high threshold button is pressed
void ProcessHighThresholdButton() {

  // Edit SPI message to the PIC32
  Serial.println("High Threshold Button Pressed");
  Threshold_Select = true;
  Threshold_Val = true;

  // keep page live but dont send any thing
  server.send(200, "text/plain", ""); // Send web page
}

// Process what happens when the water button is pressed
void ProcessWaterButton() {

  // Edit SPI message to the PIC32
  Serial.println("Water Button Pressed");
  Water_Press = true;

  // keep page live but dont send any thing
  server.send(200, "text/plain", ""); // Send web page
}

void printWifiStatus() {

  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("Open http://");
  Serial.println(ip);
}
