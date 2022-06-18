// library for Wi-Fi
#include <WiFi.h>

// library for DHT11 sensor
#include <dht_nonblocking.h>

// define sensor type as DHT11
#define DHT_SENSOR_TYPE DHT_TYPE_11

// define max values for temperature and humidity
#define maxTemp 25
#define maxHum 80

// network credentials to connect any device to the AirQualityBox
const char* ssid     = "AirQualityBox";
const char* password = "PQRE2022";

// set web server port number to 80
WiFiServer server(80);

// initialize DHT sensor
static const int DHT_SENSOR_PIN = 14;
DHT_nonblocking dht_sensor( DHT_SENSOR_PIN, DHT_SENSOR_TYPE );

// variable to store the HTTP request
String header;

// assign output variables to GPIO pins
const int greenLED = 25;
const int blueLED = 26;
const int redLED = 27;

// variables for temperature and humidity
float temperature;
float humidity;
int intTemperature;
int intHumidity;

// method when ESP32 starts up
void setup( )
{
  // initialize the serial port
  Serial.begin( 9600);
  
  // initialize the output variables as outputs
  pinMode(greenLED, OUTPUT);
  pinMode(blueLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  
  // set outputs to LOW
  digitalWrite(greenLED, LOW);
  digitalWrite(blueLED, LOW);
  digitalWrite(redLED, LOW);

  // connect to Wi-Fi network with SSID and password
  Serial.println("Setting AP (Access Point)…");
  // remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  // web server access normally 192.168.4.1
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  server.begin();
}

// poll for a measurement, keeping the state machine alive. returns true if a measurement is available
static bool measure_environment( float *temperature, float *humidity )
{
  static unsigned long measurement_timestamp = millis( );

  // measure once every ten seconds
  if( millis( ) - measurement_timestamp > 9000ul )
  {
    if( dht_sensor.measure( temperature, humidity ) == true )
    {
      measurement_timestamp = millis( );
      return( true );
    }
  }

  return( false );
}

// Main program loop
void loop( )
{
  bool connection;

  // Measure temperature and humidity.  If the functions returns true, then a measurement is available
  if( measure_environment( &temperature, &humidity ) == true )
  {    
    intTemperature = (int)temperature;
    intHumidity = (int)humidity;
    
    Serial.print("Humidity: ");
    Serial.print(intHumidity);
    Serial.print("%");
    Serial.print("  |  ");
    Serial.print("Temperature: ");
    Serial.print(intTemperature);
    Serial.println("°C");

    if(connection == false)
    {
      if(intTemperature > maxTemp or intHumidity > maxHum)
      {
        digitalWrite(greenLED, LOW);
        digitalWrite(blueLED, LOW);
        digitalWrite(redLED, HIGH);         // set RGB-LED to red
      }
      else{
        digitalWrite(greenLED, HIGH);       // if not, set RGB-LED to green
        digitalWrite(blueLED, LOW);
        digitalWrite(redLED, LOW);
      }
      Serial.println(intHumidity);
      Serial.println(intTemperature);
    }
  }

  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if a new client connects
    connection = true;
    digitalWrite(greenLED, LOW);
    digitalWrite(blueLED, HIGH);            // set RGB-LED to blue
    digitalWrite(redLED, LOW);
    
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected      
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
                                            // if the current line is blank, you got two newline characters in a row
                                            // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
                                            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                                            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            Serial.println(intHumidity);
            Serial.println(intTemperature);
            
            // display the HTML web page
            client.println("<!DOCTYPE HTML><html>");
            client.println("<head>");
            client.println("<title>Air Quality Box</title>");
            client.println("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<style>");
            client.println("html {font-family: Arial; display: inline-block; text-align: center;}");
            client.println("p {  font-size: 1.2rem;}");
            client.println("body {  margin: 0;}");
            client.println(".topnav { overflow: hidden; background-color: #444444; color: white; font-size: 1.7rem; }");
            client.println(".content { padding: 20px; }");
            client.println(".card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }");
            client.println(".cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }");
            client.println(".reading { font-size: 2.8rem; }");
            if(intTemperature > maxTemp){
              client.println(".card.temperature { color: #770000; }");
            }
            else{
              client.println(".card.temperature { color: #007700; }");
            }
            if(intHumidity > maxHum){
              client.println(".card.humidity { color: #770000; }");
            }
            else{
              client.println(".card.humidity { color: #007700; }");
            }
            client.println("</style>");
            client.println("</head>");
            
            // temperature and humidity
            client.println("<body>");
            client.println("<div class=\"topnav\">");
            client.println("<h3>Air Quality Box</h3>");
            client.println("</div>");
            client.println("<div class=\"content\">");
            client.println("<div class=\"cards\">");
            client.println("<div class=\"card temperature\">");
            client.print("<h4>TEMPERATURE</h4><p><span class=\"reading\"><span id=\"temp\">");
            client.print(intTemperature);
            client.println("</span> &deg;C</span></p>");
            client.println("</div>");
            client.println("<div class=\"card humidity\">");
            client.print("<h4>HUMIDITY</h4><p><span class=\"reading\"><span id=\"hum\">");
            client.print(intHumidity);
            client.println("</span> &percnt;</span></p>");
            client.println("</div>");
            client.println("</div>");
            client.println("</div>");
            client.println("</body>");
            client.println("</html>");
            
            // the HTTP response ends with another blank line
            client.println();
            // break out of the while loop
            break;
          } else {
            // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {
          // if you got anything else but a carriage return character, add it to the end of the currentLine
          currentLine += c;
        }
      }
    }
    // clear the header variable
    header = "";
    // close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");

    connection = false;
  }
}
