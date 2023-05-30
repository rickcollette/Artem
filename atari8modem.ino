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

// Global variables
unsigned long prevCharTime = 0;
bool modemCommandMode = false;
// .. Other necessary variables ..

#define MAX_S_REGISTERS 10
#define MAX_MODEM_INIT_STRING 100
#define MAX_BANNER_LENGTH 50

struct ModemConfig {
  uint8_t sRegisters[MAX_S_REGISTERS];
  char modemInitString[MAX_MODEM_INIT_STRING];
  uint16_t telnetListenPort;
  char busyBanner[MAX_BANNER_LENGTH];
  char connectBanner[MAX_BANNER_LENGTH];
};

ModemConfig modemConfig;

// Ethernet Server instance
EthernetServer server(modemConfig.telnetListenPort);

// Ethernet Client instances
EthernetClient modemClient;
EthernetClient telnetClient;

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

#define EEPROM_MODEM_CONFIG_ADDR 0

void setup() {
  // Initialize the Ethernet connection

  // Initialize the SoftwareSerial
  
  // Load configuration from EEPROM
  loadModemConfigFromEEPROM();
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

void loadModemConfigFromEEPROM() {
  EEPROM.get(EEPROM_MODEM_CONFIG_ADDR, modemConfig);
}

void saveModemConfigToEEPROM() {
  EEPROM.put(EEPROM_MODEM_CONFIG_ADDR, modemConfig);
}

void setModemInitString(const char* newInitString) {
  strncpy(modemConfig.modemInitString, newInitString, MAX_MODEM_INIT_STRING);
  saveModemConfigToEEPROM();
}

void setTelnetListenPort(uint16_t newPort) {
  modemConfig.telnetListenPort = newPort;
  saveModemConfigToEEPROM();
}

void setBusyBanner(const char* newBanner) {
  strncpy(modemConfig.busyBanner, newBanner, MAX_BANNER_LENGTH);
  saveModemConfigToEEPROM();
}

void setConnectBanner(const char* newBanner) {
  strncpy(modemConfig.connectBanner, newBanner, MAX_BANNER_LENGTH);
  saveModemConfigToEEPROM();
}

void setSRegister(uint8_t index, uint8_t value) {
  if (index < MAX_S_REGISTERS) {
    modemConfig.sRegisters[index] = value;
    saveModemConfigToEEPROM();
  }
  // else: error, index out of range
}

// ...and so on for other fields

