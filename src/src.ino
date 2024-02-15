// Load Wi-Fi library
#include <ESP8266WiFi.h>

// Replace with your network credentials
const char* ssid     = "<ssid>";
const char* password = "<password>";

// Set your Static IP address
IPAddress localIP(192, 168, 1, 184);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variable to store the current output state
String enableState = "off";

const int enableOutput = 0;
const int enableInput = 4;
const int PIRSensorInput = 2;    //PIR Sensor OUT Pin
const int sprayerToggle = 5;

const int SPRAY_DURATION = 3000;
const int POST_SPRAY_WAIT = 10000;

bool enable = false;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;


void initializeServer() {
  if (!WiFi.config(localIP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void serverClientCommunication(WiFiClient client) {
  Serial.println("New Client.");          // print a message out in the serial port
  String currentLine = "";                // make a String to hold incoming data from the client
  currentTime = millis();
  previousTime = currentTime;
  while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
    currentTime = millis();         
    if (client.available()) {             // if there's bytes to read from the client,
      char c = client.read();             // read a byte, then
      Serial.write(c);                    // print it out the serial monitor
      header += c;
      if (c == '\n') {                    // if the byte is a newline character
        // if the current line is blank, you got two newline characters in a row.
        // that's the end of the client HTTP request, so send a response:
        if (currentLine.length() == 0) {
          // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
          // and a content-type so the client knows what's coming, then a blank line:
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println("Connection: close");
          client.println();
          
          // turns the GPIOs on and off
          if (header.indexOf("GET /enable/on") >= 0) {
            enableState = "on";
            digitalWrite(enableOutput, HIGH);
            Serial.println("Defences ON");
          } else if (header.indexOf("GET /enable/off") >= 0) {
            enableState = "off";
            digitalWrite(enableOutput, LOW);
            Serial.println("Defences OFF");
          }
          
          // Display the HTML web page
          client.println("<!DOCTYPE html><html>");
          client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
          client.println("<link rel=\"icon\" href=\"data:,\">");
          // CSS to style the on/off buttons 
          // Feel free to change the background-color and font-size attributes to fit your preferences
          client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
          client.println(".button { background-color: #FF0000; border: none; color: white; padding: 16px 40px;");
          client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
          client.println(".button2 {background-color: #77878A;}</style></head>");
          
          // Web Page Heading
          client.println("<body><h1>Anti-Meow Defences</h1>");
          
          // Display current state, and ON/OFF buttons for GPIO 5  
          client.println("<p>Defences " + enableState + "</p>");
          // If the enableState is off, it displays the ON button       
          if (enableState=="off") {
            client.println("<p><a href=\"/enable/on\"><button class=\"button\">ARM</button></a></p>");
          } else {
            client.println("<p><a href=\"/enable/off\"><button class=\"button button2\">DISARM</button></a></p>");
          } 
           
          client.println("</body></html>");
          
          // The HTTP response ends with another blank line
          client.println();
          // Break out of the while loop
          break;
        } else { // if you got a newline, then clear currentLine
          currentLine = "";
        }
      } else if (c != '\r') {  // if you got anything else but a carriage return character,
        currentLine += c;      // add it to the end of the currentLine
      }
    }
  }
  // Clear the header variable
  header = "";
  // Close the connection
  client.stop();
//  Serial.println("Client disconnected.");
  Serial.println("");
}

void toggleEnable() {
  Serial.println("INTERRUPT!!!!");
  enable = !enable;
}

// wait with waiting output
// only works with ms as a multiple of 1000
void wait(int ms, bool exclaim = false) {
  for (int i = 0; i < ms/1000; i++) {
    delay(1000);
    if (exclaim) Serial.print("!!!"); else Serial.print(".");
  }
  Serial.println("");
}

void setup() {
  Serial.begin(9600);
  // Initialize the output variable as outputs
  pinMode(enableOutput, OUTPUT);
  pinMode(enableInput, INPUT);
  pinMode(PIRSensorInput, INPUT);
  pinMode(sprayerToggle, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(enableInput), toggleEnable, CHANGE);
  // Set output to LOW
  digitalWrite(enableOutput, LOW);
  
   digitalWrite(sprayerToggle, HIGH);

  initializeServer();
}

void loop(){
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    serverClientCommunication(client);
  }

  if (enable && digitalRead(PIRSensorInput) == HIGH)
  {
     Serial.println("Cat detected!");
     Serial.print("Commencing sprayage");

     digitalWrite(sprayerToggle, LOW);
     delay(500);
     digitalWrite(sprayerToggle, HIGH);
     
     wait(SPRAY_DURATION, true);
     digitalWrite(sprayerToggle, LOW);
     delay(500);

     digitalWrite(sprayerToggle, HIGH);
     Serial.print("Reloading");

     wait(POST_SPRAY_WAIT);
     Serial.println("Reloaded");
  }
}
