#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_ImageReader.h>
#include <WiFi.h>
#include <SpotifyEsp32.h>
#include <SPI.h>

#define TFT_CS 5 //change or not?? db check
#define TFT_RST 4
#define TFT_DC 2
#define TFT_SCLK 6
#define TFT_MOSI 7

// SPI or QSPI flash filesystem (i.e. CIRCUITPY drive)
  #if defined(__SAMD51__) || defined(NRF52840_XXAA)
    Adafruit_FlashTransport_QSPI flashTransport(PIN_QSPI_SCK, PIN_QSPI_CS,
      PIN_QSPI_IO0, PIN_QSPI_IO1, PIN_QSPI_IO2, PIN_QSPI_IO3);
  #else
    #if (SPI_INTERFACES_COUNT == 1)
      Adafruit_FlashTransport_SPI flashTransport(SS, &SPI);
    #else
      Adafruit_FlashTransport_SPI flashTransport(SS1, &SPI1);
    #endif
  #endif
  Adafruit_SPIFlash    flash(&flashTransport);
  FatFileSystem        filesys;
  Adafruit_ImageReader reader(filesys); // Image-reader, pass in flash filesys


Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

char* ssid = "YOUR WIFI SSID";
const char* password = "YOUR WIFI PASSWORD";
const char* CLIENT_ID = "YOUR CLIENT ID FROM THE SPOTIFY DASHBOARD";
const char* CLIENT_SECRET = "YOUR CLIENT SECRET FROM THE SPOTIFY DASHBOARD";


unsigned long previousMillis = 0;
unsigned long interval = 30000;

//After logging in via the URL shown in the Serial Monitor, your ESP32 will print a refresh token.
// Copy this token and pass it as the third parameter to the constructor.

Spotify sp(CLIENT_ID, CLIENT_SECRET);

//format string to wite tft.write(variable.toString().c_str());

const int buttonPin1 = 8;  // (replace pin number 3x) done!!
int buttonState1 = 0;  // variable for reading the pushbutton status
const int buttonPin2 = 6;  // the number of the pushbutton
int buttonState2 = 0; 
const int buttonPin3 = 4;  // the number of the pushbutton 
int buttonState3 = 0; 

void setup() {
  Serial.begin(115200);


  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);


  Serial.println("Setup done");

  tft.initR(INITR_BLACKTAB);  // the type of screen
  tft.setRotation(1);         // this makes the screen landscape! remove this line for portrait
  Serial.println("TFT Initialized!");
  tft.fillScreen(ST77XX_BLACK);  // make sure there is nothing in the buffer

  initWiFi();


  tft.setCursor(0, 0);                           // make the cursor at the top left
  tft.write(WiFi.localIP().toString().c_str());  // print out IP on the screen


  // Uncomment following line if you want to enable debugging:
  // sp.set_log_level(SPOTIFY_LOG_DEBUG);

  sp.begin();
  while (!sp.is_auth()) {
    sp.handle_client();
  }
  Serial.printf("Authenticated! Refresh token: %s\n", sp.get_user_tokens().refresh_token);

  if(!flash.begin()) {
    Serial.println(F("flash begin() failed"));
    for(;;);
  }
  if(!filesys.begin(&flash)) {
    Serial.println(F("filesys begin() failed"));
    for(;;);
  }

  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);
  pinMode(buttonPin3, INPUT);

}



void loop() {

  while (WiFi.status() != WL_CONNECTED) {
    scan_wifi();
  }

  unsigned long currentMillis = millis();
  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= interval)) {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;


    static String lastArtist;
    static String lastTrackname;
    static String lastAlbumImage;

    String currentArtist = sp.current_artist_names();
    String currentTrackname = sp.current_track_name();
    String currentAlbumImage = sp.get_current_album_image_url(int image_size_idx);

    if (lastArtist != currentArtist && currentArtist != "Something went wrong" && !currentArtist.isEmpty()) {
      lastArtist = currentArtist;
      Serial.println("Artist: " + lastArtist);
      tft.setCursor(10, 10);
      tft.write(lastArtist.c_str());
    }

    if (lastTrackname != currentTrackname && currentTrackname != "Something went wrong" && currentTrackname != "null") {
      lastTrackname = currentTrackname;
      Serial.println("Track: " + lastTrackname);
      tft.setCursor(10, 40);
      tft.write(lastTrackname.c_str());
    }
    if (lastAlbumImage != currentAlbumImage && currentAlbumImage != "Something went wrong" && currentAlbumImage != "null") {
      lastAlbumImage = currentAlbumImage;

      ImageReturnCode stat;
      Adafruit_Image img;
      stat = reader.loadBMP(currentAlbumImage, img);
      reader.printStatus(stat);
      img.draw(tft, 0, 0);
    }

    buttonState1 = digitalRead(buttonPin1);
      // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
    if (buttonState1 == HIGH) {
      // turn LED on:
      sp.previous();
      } else {
        // turn LED off:
      return sp.is_playing(); 
      }
    }

    buttonState2 = digitalRead(buttonPin2);
      // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
    if (buttonState2 == HIGH) {
      // turn LED on:
      sp.start_resume_playback();
      } else {
        // turn LED off:
      sp.start_resume_playback();
      }
    }

    buttonState3 = digitalRead(buttonPin3);
      // check if the pushbutton is pressed. If it is, the buttonState is HIGH:
    if (buttonState3 == HIGH) {
      // turn LED on:
      sp.skip();
      } else {
        // turn LED off:
      return sp.is_playing(); 
      }
    }

  }

  void initWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi ..");
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print('.');
      delay(1000);
    }
    Serial.println(WiFi.localIP());
  }

  void scan_wifi() {


    Serial.println("scan start");

    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0) {
      Serial.println("no networks found");
    } else {
      Serial.print(n);
      Serial.println(" networks found");
      for (int i = 0; i < n; ++i) {
        // Print SSID and RSSI for each network found
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        Serial.print(" (");
        Serial.print(WiFi.RSSI(i));
        Serial.print(")");
        Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
        delay(10);
      }
    }
    Serial.println("");

    // Wait a bit before scanning again
    delay(5000);
  }

//String get_current_album_image_url(int image_size_idx);

//sp.current_artist_names();   // returns String of all artists separated by a ','
// sp.current_track_name();     // returns String of song name
// sp.start_resume_playback();  // Calling this method will pause/play the track
// sp.skip();                   // skips current track
// sp.previous();               // goes to previous track
// sp.is_playing();             // returns a boolean if something is playing currently





