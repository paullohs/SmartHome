#include <DHT.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WebServer.h>
#include <deque>
#include <sstream>

// Sensor-Setup
DHT dht(2, DHT11);
// Wifi-Setup
const char* ssid = "tbd";
const char* password = "tbd";
WiFiUDP ntpUDP;
// Time-Setup
const long utcOffsetInSeconds = 7200;
unsigned long startTime = 0;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);
// Webserver-Setup
WebServer server(80);
// Runtime-Variables
char buff[70000];
uint8_t currentSecond = 0;
uint8_t lastSecond = 0;
std::deque<uint16_t> temps;
std::deque<uint16_t> humids;
std::deque<uint16_t> timestamps;
std::stringstream outputHtml;
float currentTemp;
float currentHumid;
// Runtime-Setup
const uint16_t maxItems = 1500;
const uint8_t readInterval = 10;

void handle_root() {
  if (prep_html()) {
    snprintf(buff, sizeof(buff), outputHtml.str().c_str(), currentHumid, currentTemp);
    Serial.printf("Created site with %u entries, buffer usage %u out of %u.\n", temps.size(), strlen(buff), sizeof(buff));
    server.send(200, "text/html", buff);
  }
}

bool prep_html() {
  outputHtml.str(std::string());
  outputHtml.clear();
  outputHtml << "<!DOCTYPE html>\n\
  <html>\n\
  <head>\n\
  <script src=\"https://cdn.plot.ly/plotly-2.3.1.min.js\"></script>\n\
  </head>\n\
  <body>\n\
  <h1>Luftfeuchtigkeit: %.2f &#37;; Temperatur: %.2f &deg;C</h1><br/>\n\
  <div id=\'myDiv\'></div>\n\
  <script>var trace1 = {\nx: [";

  for (int i=0; i < timestamps.size(); i++) {
    outputHtml << timestamps[i];
    if (i != timestamps.size() - 1) {
      outputHtml << ",";
    }
   }
 outputHtml << "],\ny: [";
 
 for (int i=0; i < temps.size(); i++) {
  outputHtml << float(temps[i])/10;
  if (i != temps.size() - 1) {
    outputHtml << ",";
  }
 }
outputHtml << "],\nname: \'Temperatur\',\ntype:  \'scatter\'};\nvar trace2 = {\nx: [";

for (int i=0; i < timestamps.size(); i++) {
  outputHtml << timestamps[i];
  if (i != timestamps.size() - 1) {
    outputHtml << ",";
  }
 }
 outputHtml << "],\ny: [";
 
 for (int i=0; i < humids.size(); i++) {
  outputHtml << float(humids[i])/10;
  if (i != humids.size() - 1) {
    outputHtml << ",";
  }
 }
outputHtml << "],\nname: \'Feuchtigkeit\',\ntype: \'scatter\',\nyaxis: \'y2\'};\
  \nvar data = [trace1, trace2];\
  \nvar layout = {\nyaxis: {title: \'Celsius\'},\nyaxis2: {title: \'RH\',overlaying: \'y\', side: \'right\'}};\n\
  Plotly.newPlot(\'myDiv\', data, layout);</script>\n\
  </body>\n\
  </html>";
  return true;
}

void setup() {
  Serial.begin(115200);
  WiFi.scanNetworks();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println(WiFi.localIP());
  timeClient.begin();
  timeClient.update();
  startTime = timeClient.getEpochTime();
  Serial.printf("Synchronized time: %s\n", timeClient.getFormattedTime().c_str());
  server.on("/", handle_root);
  server.begin();
  Serial.println("Server started");
  dht.begin();
}

void loop() {
 server.handleClient();
 currentSecond = uint8_t(timeClient.getSeconds());
 if (((currentSecond % readInterval) == 0) && (lastSecond != currentSecond)) {
  lastSecond = uint8_t(timeClient.getSeconds());
  currentTemp = dht.readTemperature();
  currentHumid = dht.readHumidity();
  if (~(isnan(currentTemp) && isnan(currentHumid))) {
    if (timestamps.size() > maxItems) {
      temps.pop_front();
      humids.pop_front();
      timestamps.pop_front();
    }
    temps.push_back(uint16_t(currentTemp*10));
    humids.push_back(uint16_t(currentHumid*10));
    timestamps.push_back(uint16_t(timeClient.getEpochTime() - startTime));
    Serial.printf("Saved: %u, %u\n", temps.back(), humids.back());
  } else {
    Serial.printf("Not Saved - Reading was NaN!\n");
  }
 }
}
