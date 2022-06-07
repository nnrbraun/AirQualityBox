#include <WiFi.h>
#include <dht_nonblocking.h>

#define DHT_SENSOR_TYPE DHT_TYPE_11

// Replace with your network credentials
const char* ssid     = "AirQualityBox";
const char* password = "PQRE2022";

// Set web server port number to 80
WiFiServer server(80);

// Initialize DHT sensor
static const int DHT_SENSOR_PIN = 14;
DHT_nonblocking dht_sensor( DHT_SENSOR_PIN, DHT_SENSOR_TYPE );

// Variable to store the HTTP request
String header;

// Assign output variables to GPIO pins
const int greenLED = 25;
const int blueLED = 26;
const int redLED = 27;

/*
 * Initialize the serial port.
 */
void setup( )
{
  Serial.begin( 9600);
  
  // Initialize the output variables as outputs
  pinMode(greenLED, OUTPUT);
  pinMode(blueLED, OUTPUT);
  pinMode(redLED, OUTPUT);
  
  // Set outputs to LOW
  digitalWrite(greenLED, LOW);
  digitalWrite(blueLED, LOW);
  digitalWrite(redLED, LOW);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  server.begin();
}

/*
 * Poll for a measurement, keeping the state machine alive.  Returns
 * true if a measurement is available.
 */
static bool measure_environment( float *temperature, float *humidity )
{
  static unsigned long measurement_timestamp = millis( );

  /* Measure once every ten seconds. */
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

/*
 * Main program loop.
 */
void loop( )
{
  float temperature;
  float humidity;
  int intTemperature;
  int intHumidity;
  bool connection;

  /* Measure temperature and humidity.  If the functions returns
     true, then a measurement is available. */
  if( measure_environment( &temperature, &humidity ) == true )
  {
    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.print("%");
    Serial.print("  |  ");
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println("°C");

    intHumidity = (int)humidity;
    intTemperature = (int)temperature;

    if(connection == false)
    {
      if(intTemperature>27)
      {
        digitalWrite(greenLED, LOW);
        digitalWrite(blueLED, LOW);
        digitalWrite(redLED, HIGH);
      }
      else{
        digitalWrite(greenLED, HIGH);
        digitalWrite(blueLED, LOW);
        digitalWrite(redLED, LOW);
      }
    }
  }

  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    connection = true;
    digitalWrite(greenLED, LOW);
    digitalWrite(blueLED, HIGH);
    digitalWrite(redLED, LOW);
    
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected      
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
            
            // Display the HTML web page
            client.println("<!DOCTYPE html> <html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">");
            client.println("<title>ESP32 Air Quality Box</title>");
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println("body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}");
            client.println("p {font-size: 24px;color: #444444;margin-bottom: 10px;}");
            client.println("</style>");
            client.println("</head>");
            
            // Web Page Heading
            client.println("<body>");
            client.println("<div id=\"webpage\">");
            client.println("<h1>Air Quality Box</h1>");
            
            // temperature and humidity
            client.print("<p>Temperature: ");
            client.print(intTemperature);
            client.print(" \260C</p>");
            client.print("<p>Humidity: ");
            client.print(intHumidity);
            client.print(" %</p>");
            client.println("</div>");
            client.println("</body>");
            client.println("</html>");
            
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
    Serial.println("Client disconnected.");
    Serial.println("");

    connection = false;
  }
}
