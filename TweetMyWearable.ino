/*********************************************************************
  This is an example for our nRF52 based Bluefruit LE modules

  Pick one up today in the adafruit shop!

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  MIT license, check LICENSE for more information
  All text above, and the splash screen below must be included in
  any redistribution
*********************************************************************/

/*********************************************************************
Note from Julie Barrett:
I used the Adafruit blueart nrf52 example code as a base, but have made my
additions.

The same MIT license applies here.
*********************************************************************/

#include <bluefruit.h>
//#include <Adafruit_LittleFS.h>
//#include <InternalFileSystem.h>
#include <Adafruit_NeoPixel.h>
#include "SoftwareSerial.h"
#include "SPI.h"
#define PixelPin 15
#define PixelNum 88
//#define ANIMATION_PERIOD_MS  300  // The amount of time (in milliseconds) that each
                                  // animation frame lasts.  Decrease this to speed
                                  // up the animation, and increase it to slow it down.
                                  
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(PixelNum, PixelPin, NEO_GRB + NEO_KHZ800);
// BLE Service
BLEDfu  bledfu;  // OTA DFU service
BLEDis  bledis;  // device information
BLEUart bleuart; // uart over ble
BLEBas  blebas;  // battery

String strBlu = "";
String sColor = "";
char character;
//uint8_t  frame = 0;    // Frame count, results displayed every 256 frames
//uint8_t  i;
  
void setup()
{
  pixels.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  //pixels.fill((192,192,192),0,PixelNum);
  //pixels.fill((12,235,12),0,PixelNum);
  //sparkle();
  pixels.show();            // Turn OFF all pixels ASAP
  pixels.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
  randomSeed(analogRead(0));  //Set random number generator seed from analog pin 0
  Serial.begin(115200);
  //rainbow(100);             // Flowing rainbow cycle along the whole strip
  
#if CFG_DEBUG
  // Blocking wait for connection when debug mode is enabled via IDE
  while ( !Serial ) yield();
#endif

  Serial.println("Bluefruit52 BLEUART Example");
  Serial.println("---------------------------\n");

  // Setup the BLE LED to be enabled on CONNECT
  // Note: This is actually the default behavior, but provided
  // here in case you want to control this LED manually via PIN 19
  Bluefruit.autoConnLed(true);

  // Config the peripheral connection with maximum bandwidth
  // more SRAM required by SoftDevice
  // Note: All config***() function must be called before begin()
  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);

  Bluefruit.begin();
  Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values
  //Bluefruit.setName(getMcuUniqueID()); // useful testing with multiple central connections
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  // To be consistent OTA DFU should be added first if it exists
  bledfu.begin();

  // Configure and Start Device Information Service
  bledis.setManufacturer("Adafruit Industries");
  bledis.setModel("Bluefruit Feather52");
  bledis.begin();

  // Configure and Start BLE Uart Service
  bleuart.begin();

  // Start BLE Battery Service
  blebas.begin();
  blebas.write(100);

  // Set up and start advertising
  startAdv();

  Serial.println("Please use Adafruit's Bluefruit LE app to connect in UART mode");
  Serial.println("Once connected, enter character(s) that you wish to send");
}


void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  //Serial.println("advertising...");
  // Include bleuart 128-bit uuid
  Bluefruit.Advertising.addService(bleuart);

  // Secondary Scan Response packet (optional)
  // Since there is no room for 'Name' in Advertising packet
  Bluefruit.ScanResponse.addName();

  /* Start Advertising
     - Enable auto advertising if disconnected
     - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
     - Timeout for fast mode is 30 seconds
     - Start(timeout) with timeout = 0 will advertise forever (until connected)

     For recommended advertising interval
     https://developer.apple.com/library/content/qa/qa1931/_index.html
  */
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}

void loop()
{

  // If data is typed into the serial-monitor, treat it the same as input from BLEUART
  while (Serial.available())
  {
    // Delay to wait for enough input, since we have a limited transmission buffer
    delay(2);

    char buf[64];
    memset(buf, 0, sizeof(buf));
    int count = Serial.readBytes(buf, sizeof(buf));
    Serial.printf(" got %d chars from serial: %s\n", count, buf);
    strBlu = String(buf);
  }

  // Get data from BLEUART (if any) and do stuff
  while ( bleuart.available() )
  {
    character = bleuart.read();
    strBlu.concat(character);
  }
  
  if (strBlu != "") {
    Serial.print("What we got from the BLUEART: ");
    Serial.println(strBlu);
    sColor = strBlu;   
    strBlu = "";
  }

  if (random(100) > 25)
    sparks(sColor.c_str());
  else
    scan(sColor.c_str());
}


// callback invoked when central connects
void connect_callback(uint16_t conn_handle)
{
  // Get the reference to current connection
  BLEConnection* connection = Bluefruit.Connection(conn_handle);

  char central_name[32] = { 0 };
  connection->getPeerName(central_name, sizeof(central_name));

  Serial.print("Connected to ");
  Serial.println(central_name);
  Serial.println();
}

/**
   Callback invoked when a connection is dropped
   @param conn_handle connection where this event happens
   @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
*/
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;
  Serial.println();
  Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);
}

//Larsen scanner

void scan(const char* colorName)
{
  Serial.printf("scan(%s)\n", colorName);
  unsigned long color = 0;
  sscanf(colorName + 1, "%X", &color);

  for (int count = 0; count < 3; count++)
  {
    for (int i = 2; i < PixelNum-2; i++) {
      pixels.setPixelColor(i-2, 0);
      pixels.setPixelColor(i-1, color);
      pixels.setPixelColor(i,   color);
      pixels.setPixelColor(i+1, color);
      pixels.show();
      delay(20);
    }

    for (int i = PixelNum-2; i >= 2; i--) {
      pixels.setPixelColor(i-1, color);
      pixels.setPixelColor(i,   color);
      pixels.setPixelColor(i+1, color);
      pixels.setPixelColor(i+1, 0);
      pixels.show();
      delay(20);
    }
  }
    
  for (int i = 0; i < PixelNum; i++) {
    pixels.setPixelColor(i, 0);
  }
  pixels.show();
}
//Sparks animation
void sparks(const char* colorName){
  // Random sparks - just one LED on at a time!
  Serial.printf("sparks: %s\n", colorName);
  
  unsigned long color = 0;
  sscanf(colorName + 1, "%X", &color);
  
  for (int count = 0; count < 100; count++)
  {
    int i = random(PixelNum);
    pixels.setPixelColor(i, color);
    pixels.show();
    delay(50);
    pixels.setPixelColor(i, 0);
  }   
}
