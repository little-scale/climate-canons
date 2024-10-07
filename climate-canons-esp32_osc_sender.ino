#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>

char ssid[] = "CompactRouter";  // your network SSID (name)
char pass[] = "jellybean3000";  // your network password

WiFiUDP Udp;                              // A UDP instance to let us send and receive packets over UDP
const IPAddress outIp(192, 168, 8, 221);  // remote IP of your computer
const unsigned outPort = 9999;        // remote port to receive OSC
const unsigned int localPort = 8888;      // local port to listen for OSC packets (actually not used for sending)

uint8_t amplitude;
int thresh = 700;
int rate_thresh = 20;
int data;
int previous_data;

const int gong_number = 2;

void setup() {
  Serial.begin(115200);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(localPort);
}

void loop() {

  previous_data = data;
  data = analogRead(A1);

  if (data > thresh) {

    data = data >> 4;
    OSCMessage msg("/gong");
    msg.add((float)gong_number); 
    msg.add((float)data);
    Udp.beginPacket(outIp, outPort);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();

    delay(250);
  }
  delay(5);
}