#include <EEPROM.h>
#include <UIPEthernet.h>
#include <SoftwareSerial.h>
#include <Dns.h>

// Modem constants
#define MAX_COMMAND_LENGTH 64
#define MAX_S_REGISTERS 10
#define DEFAULT_S_REGISTER_VALUE 0
#define MAX_HOST_LENGTH 100
#define MAX_MODEM_INIT_STRING 100
#define MAX_BANNER_LENGTH 100
#define EEPROM_MODEM_CONFIG_ADDR 0
#define MODEM_CONNECTION_ID 0
#define TELNET_CONNECTION_ID 1
#define TELNET_PROTOCOL 0
#define SERIAL_SPEED 9600

// SIO constants
#define SIO_ACK 0x41
#define SIO_NAK 0x4E
#define SIO_ERROR 0x45
#define SIO_GET_STATUS 0x53
#define SIO_OPEN 0x4F
#define SIO_READ 0x52
#define SIO_WRITE 0x50
#define SIO_CLOSE 0x43

struct ModemConfig {
  char modemInitString[MAX_MODEM_INIT_STRING];
  uint16_t telnetListenPort;
  char busyBanner[MAX_BANNER_LENGTH];
  char connectBanner[MAX_BANNER_LENGTH];
  uint8_t sRegisters[MAX_S_REGISTERS];
};

SoftwareSerial SWSer(2, 3); // RX, TX
EthernetClient modemClient;
EthernetClient telnetClient;
ModemConfig modemConfig;
bool modemCommandMode = true;
char commandBuffer[MAX_COMMAND_LENGTH];
int commandIndex = 0;
DNSClient myDns;
uint8_t incomingByte;

enum class TelnetState {
    Data,
    IAC,      // Interpret As Command
    DO, DONT, // options for what the other end of the connection will do
    WILL, WONT // options for what the Telnet server will do
};

// Custom function declarations
void handleSIORequest();
void handleSIOGetStatus();
void handleSIOOpen(uint8_t aux1, uint8_t aux2);
void handleSIORead(uint8_t aux1, uint8_t aux2);
void handleSIOWrite(uint8_t aux1, uint8_t aux2);
void handleSIOClose(uint8_t aux1);
void loadModemConfigFromEEPROM();
void saveModemConfigToEEPROM();
void initModemConfig();
void handleIncomingConnections();
void handleOutgoingConnections();
void handleSerialData();
void handleNetworkData();
void relayModemData();
void relayTelnetData();
uint8_t getDeviceStatus();
uint8_t getConnectionStatus(uint8_t connectionIndex);
void readData(char* buffer, uint8_t length);
bool openConnection(uint8_t protocol, uint8_t connectionID, const char* hostAndPort);
uint8_t readDataFromConnection(uint8_t connectionID, char* buffer, uint8_t length);
bool writeDataToConnection(uint8_t connectionID, const char* buffer, uint8_t length);
bool closeConnection(uint8_t connectionID);
void handleDialCommand(const char* command);
void handleHangUpCommand();
void handleSRegisterCommand(const char* command);
void handleResetCommand();
void handleRingCommand();

void setup() {
  // Start the software serial port
  SWSer.begin(SERIAL_SPEED);
  // Load modem configuration from EEPROM
  loadModemConfigFromEEPROM();
  // Initialise the Ethernet connection
  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
  Ethernet.begin(mac);
  // Initialise the DNS client
  myDns.begin(Ethernet.dnsServerIP());
}

void loop() {
  // Handle incoming modem commands
  handleModemCommand();
  // Handle Atari SIO data
  handleSIORequest();
  // Handle modem and telnet data
  relayModemData();
  relayTelnetData();
}

void handleSIORequest()
{
    // Check if SIO data is available
    if (SWSer.available() > 0)
    {
        // Read the command type byte
        uint8_t command = SWSer.read();

        // Read the auxiliary bytes
        uint8_t aux1 = SWSer.read();
        uint8_t aux2 = SWSer.read();

        switch (command)
        {
        case SIO_GET_STATUS:
            handleSIOGetStatus();
            break;

        case SIO_OPEN:
            handleSIOOpen(aux1, aux2);
            break;

        case SIO_READ:
            handleSIORead(aux1, aux2);
            break;

        case SIO_WRITE:
            handleSIOWrite(aux1, aux2);
            break;

        case SIO_CLOSE:
            handleSIOClose(aux1);
            break;

        default:
            // Unknown command, send NAK
            SWSer.write(SIO_NAK);
            break;
        }
    }
}

void handleSIOGetStatus()
{
    // Get the status of the networking device and all connections
    uint8_t deviceStatus = getDeviceStatus();
    uint8_t connectionStatus[4] = {getConnectionStatus(0), getConnectionStatus(1), getConnectionStatus(2), getConnectionStatus(3)};

    // Send the status data back to the Atari
    SWSer.write(SIO_ACK);             // Acknowledge receipt of command
    SWSer.write(connectionStatus, 4); // Send the connection statuses
    SWSer.write(deviceStatus);        // Send the device status
    SWSer.write(SIO_CLOSE);        // Indicate the end of the command
}

void handleSIOOpen(uint8_t aux1, uint8_t aux2)
{
    // Extract the protocol and connection ID from aux1
    uint8_t protocol = aux1 >> 2;
    uint8_t connectionID = aux1 & 0x03;

    // The rest of the data contains the host and port
    uint8_t dataLength = aux2 == 0 ? 256 : aux2;
    char hostAndPort[dataLength + 1];
    readData(hostAndPort, dataLength);

    // Open the connection
    if (openConnection(protocol, connectionID, hostAndPort))
    {
        SWSer.write(SIO_ACK); // Acknowledge successful completion
    }
    else
    {
        SWSer.write(SIO_NAK); // Indicate an error occurred
    }

    SWSer.write(SIO_CLOSE); // Indicate the end of the command
}

void handleSIORead(uint8_t aux1, uint8_t aux2)
{
    // Extract the connection ID from aux1 and read aux2 bytes from it
    uint8_t connectionID = aux1 & 0x03;
    uint8_t dataLength = aux2;

    char buffer[dataLength + 1];
    uint8_t bytesRead = readDataFromConnection(connectionID, buffer, dataLength);

    // Send the data back to the Atari
    SWSer.write(SIO_ACK);           // Acknowledge receipt of command
    SWSer.write(buffer, bytesRead); // Send the read data
    SWSer.write(SIO_CLOSE);      // Indicate the end of the command
}

void handleSIOWrite(uint8_t aux1, uint8_t aux2)
{
    // Extract the connection ID from aux1 and write aux2 bytes to it
    uint8_t connectionID = aux1 & 0x03;
    uint8_t dataLength = aux2;

    char buffer[dataLength];
    readData(buffer, dataLength); // Read the data to be written from the command

    if (writeDataToConnection(connectionID, buffer, dataLength))
    {
        SWSer.write(SIO_ACK); // Acknowledge successful completion
    }
    else
    {
        SWSer.write(SIO_NAK); // Indicate an error occurred
    }

    SWSer.write(SIO_CLOSE); // Indicate the end of the command
}

void handleSIOClose(uint8_t aux1)
{
    // Extract the connection ID from aux1 and close it
    uint8_t connectionID = aux1 & 0x03;

    if (closeConnection(connectionID))
    {
        SWSer.write(SIO_ACK); // Acknowledge successful completion
    }
    else
    {
        SWSer.write(SIO_NAK); // Indicate an error occurred
    }

    SWSer.write(SIO_CLOSE); // Indicate the end of the command
}

void handleModemCommand()
{
    // Check if data is available
    if (SWSer.available() > 0)
    {
        // Read the incoming byte
        char incomingByte = SWSer.read();

        // Check if the byte is a newline or carriage return
        if (incomingByte == '\n' || incomingByte == '\r')
        {
            // Null-terminate the command
            commandBuffer[commandIndex] = 0;

            // Process the command
            if (strncmp(commandBuffer, "ATD", 3) == 0)
            {
                // Handle dial command
                handleDialCommand(commandBuffer + 3);
            }
            else if (strncmp(commandBuffer, "ATH", 3) == 0)
            {
                // Handle hang up command
                handleHangUpCommand();
            }
            else if (strncmp(commandBuffer, "ATS", 3) == 0)
            {
                // Handle S register command
                handleSRegisterCommand(commandBuffer + 3);
            }
            else if (strncmp(commandBuffer, "ATZ", 3) == 0)
            {
                // Handle reset command
                handleResetCommand();
            }
            else if (strncmp(commandBuffer, "RING", 4) == 0)
            {
                handleRingCommand();
            }
        }
        else
        {
            // Handle unknown command
            SWSer.println("ERROR");
        }

        // Clear the command buffer
        commandIndex = 0;
    }
    else
    {
        // Add the byte to the command buffer, if it will fit
        if (commandIndex < MAX_COMMAND_LENGTH - 1)
        {
            commandBuffer[commandIndex++] = incomingByte;
        }
    }
}


void handleRingCommand()
{
    static int ringCount = 0;
    ringCount++;

    if (ringCount >= modemConfig.sRegisters[0])
    { // S0 is our auto-answer register
        SWSer.println("ATA");
        ringCount = 0; // reset ring count
    }
}

void handleDialCommand(const char *serverInfo)
{
    // Dial the specified serverInfo (in this case, it could be IP address or domain name)
    char serverIP[16];
    int serverPort = 23; // Default port
    sscanf(serverInfo, "%15[^:]:%d", serverIP, &serverPort);

    // Resolve domain name if serverIP is not an IP address
    if (!modemClient.connect(serverIP, serverPort))
    {
        IPAddress resolvedIP;
        myDns.getHostByName(serverIP, resolvedIP);
        if (!modemClient.connect(resolvedIP, serverPort))
        {
            SWSer.println("NO CARRIER");
            return;
        }
    }

    SWSer.println("CONNECT");
    modemCommandMode = false;
}

void handleHangUpCommand()
{
    // Hang up the current connection
    if (modemClient.connected())
    {
        modemClient.stop();
        SWSer.println("OK");
    }
    else
    {
        SWSer.println("ERROR");
    }
}

void handleSRegisterCommand(const char *registerInfo)
{
    // Handle setting/getting S register values
    int registerNumber = 0;
    int registerValue = 0;
    sscanf(registerInfo, "%d=%d", &registerNumber, &registerValue);

    // Validate register number
    if (registerNumber >= 0 && registerNumber < MAX_S_REGISTERS)
    {
        setSRegister(registerNumber, registerValue);
        SWSer.println("OK");
    }
    else
    {
        SWSer.println("ERROR");
    }
}

void handleResetCommand()
{
    // Reset the modem
    // Disconnect any current connection
    if (modemClient.connected())
    {
        modemClient.stop();
    }

    // Reset S registers to their default values
    for (int i = 0; i < MAX_S_REGISTERS; i++)
    {
        setSRegister(i, DEFAULT_S_REGISTER_VALUE);
    }

    SWSer.println("OK");
}

bool openConnection(uint8_t protocol, uint8_t connectionID, const char *hostAndPort)
{
    // Parse host and port from hostAndPort string
    char host[MAX_HOST_LENGTH];
    uint16_t port;
    sscanf(hostAndPort, "%[^:]:%u", host, &port); // Adjusted sscanf pattern to exclude ':' from the host string

    // Based on protocol and connectionID, open a connection using the appropriate EthernetClient instance
    if (protocol == TELNET_PROTOCOL)
    {
        if (connectionID == MODEM_CONNECTION_ID)
        {
            return modemClient.connect(host, port);
        }
        else if (connectionID == TELNET_CONNECTION_ID)
        {
            return telnetClient.connect(host, port);
        }
    }
    return false;
}

bool closeConnection(uint8_t connectionID)
{
    // Based on connectionID, close the connection using the appropriate EthernetClient instance
    if (connectionID == MODEM_CONNECTION_ID)
    {
        modemClient.stop();
        return !modemClient.connected();
    }
    else if (connectionID == TELNET_CONNECTION_ID)
    {
        telnetClient.stop();
        return !telnetClient.connected();
    }
    return false;
}

uint8_t readDataFromConnection(uint8_t connectionID, char *buffer, uint8_t length)
{
    // Based on connectionID, read data from the connection using the appropriate EthernetClient instance
    EthernetClient *client = NULL;
    if (connectionID == MODEM_CONNECTION_ID)
    {
        client = &modemClient;
    }
    else if (connectionID == TELNET_CONNECTION_ID)
    {
        client = &telnetClient;
    }

    if (client && client->available())
    {
        return client->readBytes(buffer, length);
    }
    return 0;
}

bool writeDataToConnection(uint8_t connectionID, const char *buffer, uint8_t length)
{
    // Based on connectionID, write data to the connection using the appropriate EthernetClient instance
    EthernetClient *client = NULL;
    if (connectionID == MODEM_CONNECTION_ID)
    {
        client = &modemClient;
    }
    else if (connectionID == TELNET_CONNECTION_ID)
    {
        client = &telnetClient;
    }

    if (client && client->connected())
    {
        return client->write((uint8_t *)buffer, length) == length;
    }
    return false;
}

uint8_t getDeviceStatus()
{
    // Return the device status. Here we consider a simple status flag indicating if the device is connected to the network
    return (uint8_t)Ethernet.localIP() != 0; // You may want to consider more specific status like: is IP assigned, is gateway reachable, etc.
}

uint8_t getConnectionStatus(uint8_t connectionID)
{
    // Return the connection status
    if (connectionID == MODEM_CONNECTION_ID)
    {
        return modemClient.connected() ? 1 : 0;
    }
    else if (connectionID == TELNET_CONNECTION_ID)
    {
        return telnetClient.connected() ? 1 : 0;
    }
    return 0;
}

void readData(char *buffer, uint8_t length)
{
    // Read the data from the SoftwareSerial instance
    for (int i = 0; i < length && SWSer.available(); i++)
    {
        buffer[i] = SWSer.read();
    }
}

void relayModemData() {
  while (modemClient.available()) {
    SWSer.write(modemClient.read());
  }
}

void relayTelnetData() {
  while (telnetClient.available()) {
    SWSer.write(telnetClient.read());
  }
}

bool handleTelnetProtocol(uint8_t b, EthernetClient &client, TelnetState &telnetState)
{
    switch (telnetState) {
        case TelnetState::Data:
            if (b == 0xFF) { // IAC
                telnetState = TelnetState::IAC;
                return true;
            } else {
                return false; // Data byte
            }

        case TelnetState::IAC:
            switch (b) {
                case 0xFB: // DO
                    telnetState = TelnetState::DO;
                    return true;
                case 0xFC: // DONT
                    telnetState = TelnetState::DONT;
                    return true;
                case 0xFD: // WILL
                    telnetState = TelnetState::WILL;
                    return true;
                case 0xFE: // WONT
                    telnetState = TelnetState::WONT;
                    return true;
                default:
                    telnetState = TelnetState::Data;
                    return true; // Unrecognized IAC command
            }

        case TelnetState::DO:
        case TelnetState::DONT:
        case TelnetState::WILL:
        case TelnetState::WONT:
            telnetState = TelnetState::Data; // Just consume the option code
            return true;
    }
}


void loadModemConfigFromEEPROM()
{
    EEPROM.get(EEPROM_MODEM_CONFIG_ADDR, modemConfig);
}

void saveModemConfigToEEPROM()
{
    EEPROM.put(EEPROM_MODEM_CONFIG_ADDR, modemConfig);
}

void setModemInitString(const char *newInitString)
{
    strncpy(modemConfig.modemInitString, newInitString, MAX_MODEM_INIT_STRING);
    saveModemConfigToEEPROM();
}

void setTelnetListenPort(uint16_t newPort)
{
    modemConfig.telnetListenPort = newPort;
    saveModemConfigToEEPROM();
}

void setBusyBanner(const char *newBanner)
{
    strncpy(modemConfig.busyBanner, newBanner, MAX_BANNER_LENGTH);
    saveModemConfigToEEPROM();
}

void setConnectBanner(const char *newBanner)
{
    strncpy(modemConfig.connectBanner, newBanner, MAX_BANNER_LENGTH);
    saveModemConfigToEEPROM();
}

void setSRegister(uint8_t index, uint8_t value) {
  if (index < MAX_S_REGISTERS) {
    modemConfig.sRegisters[index] = value;
    saveModemConfigToEEPROM();
  }
}
