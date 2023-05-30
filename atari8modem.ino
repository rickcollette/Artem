#include <EEPROM.h>
#include <UIPEthernet.h>
#include <SoftwareSerial.h>
#include <Dns.h>

// SIO constants
#define SIO_ACK 0x41
#define SIO_NAK 0x4E
#define SIO_COMPLETE 0x43
#define SIO_ERROR 0x45

// Pins for SoftwareSerial
#define RX_PIN 2
#define TX_PIN 3

// Ethernet setup parameters
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

// SoftwareSerial instance
SoftwareSerial SWSer(RX_PIN, TX_PIN);

// Ethernet Server instance
EthernetServer server(23);

// Ethernet Client instances
EthernetClient modemClient;
EthernetClient telnetClient;

// Global variables
unsigned long prevCharTime = 0; 
bool modemCommandMode = false; 
// .. Other necessary variables ..

// Define data structure for Modem and Telnet state
struct ModemData {
  // Variables...
};

struct TelnetState {
  // Variables...
};

ModemData modemData;
TelnetState modemTelnetState;
TelnetState clientTelnetState;

void setup() {
  // Initialize the Ethernet connection
  Ethernet.begin(mac);
  while (!Ethernet.begin(mac)) {
    delay(1000); // Wait a second if initialization failed
  }

  // Initialize the SoftwareSerial
  SWSer.begin(9600);  // replace 9600 with the desired baud rate

  // Load configuration from EEPROM
  loadModemConfigFromEEPROM();

  // Initialize the server on the loaded port
  server = EthernetServer(modemConfig.telnetListenPort);
  server.begin();
}

void loop() {
  // Check for incoming connections and handle data
  // Switch between command and data modes
  // Handle SIO requests
  // Handle modem commands
}

void handleSIORequest() {
  // Handle the SIO request from the Atari
}

void handleModemCommand() {
  // Handle commands coming from the modem
}

void relayModemData() {
  // Relay the data from the modem to the Atari
}

void relayTelnetData() {
  // Relay the data from the telnet client to the Atari
}

bool handleTelnetProtocol(uint8_t b, EthernetClient &client, TelnetState &telnetState) {
  // Handle the telnet protocol
  // Return whether the byte was a telnet protocol command or not
}

// Additional functions as necessary

