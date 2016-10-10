// receiver.pde
//
// Simple example of how to use VirtualWire to receive messages
// Implements a simplex (one-way) receiver with an Rx-B1 module
//
// See VirtualWire.h for detailed API docs
// Author: Mike McCauley (mikem@airspayce.com)
// Copyright (C) 2008 Mike McCauley
// $Id: receiver.pde,v 1.3 2009/03/30 00:07:24 mikem Exp $

#include <VirtualWire.h>
#include <EtherCard.h>

// ethernet interface mac address, must be unique on the LAN
static byte mymac[] = { 0x74,0x69,0x69,0x2D,0x30,0x31 };

// MAC address in the HEX format  
String macString = "";

byte Ethernet::buffer[700];
static uint32_t timer;

// Website IP 
const char website[] PROGMEM = "192.168.1.107";

void initEthernet()
{
  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0) 
    Serial.println(F("Failed to access Ethernet controller"));
  if (!ether.dhcpSetup())
    Serial.println(F("DHCP failed"));

  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);  
  ether.printIp("DNS: ", ether.dnsip);  

#if 0
  // use DNS to resolve the website's IP address
  if (!ether.dnsLookup(website))
    Serial.println("DNS failed");
#elif 2
  // if website is a string containing an IP address instead of a domain name,
  // then use it directly. Note: the string can not be in PROGMEM.
  // Copy website string from PROGMEM
  char* temp = (char*)malloc(strlen(website) * sizeof(char));
  strcpy_P(temp, website);
  ether.parseIp(ether.hisip, temp);
#endif
  ether.printIp("SRV: ", ether.hisip);

  // Convert MAC address from byty to HEX presentation
  macString = convertByteMacToHex();
}

void setup()
{
  Serial.begin(9600);  // Debugging only

  // Initialise the IO and ISR
  vw_set_ptt_inverted(true); // Required for DR3100
  vw_setup(2000);  // Bits per sec
  vw_set_rx_pin(8);
  vw_rx_start();       // Start the receiver PLL running
  initEthernet();
}

void loop()
{
  uint8_t buf[VW_MAX_MESSAGE_LEN];
  uint8_t buflen = VW_MAX_MESSAGE_LEN;
  ether.packetLoop(ether.packetReceive());

  if (vw_get_message(buf, &buflen)) // Non-blocking
  {
    int i;

    String data = "";
    for (i = 0; i < buflen; i++) {
      data += buf[i];
    }
    // If we get the 1 (ASCII code - 49)
    if (data == "49") {
      Serial.println("---------HEART BEAT------------");
      sendDataToServer(1);
    }
  }
}

/**
 * Method for pushing the heart beat value to the server
 * */
void sendDataToServer(int value)
{
  if (millis() > timer) {
    timer = millis() + 100;
    Stash stash;
    byte sd = stash.create();
    stash.print("mac_address=");
    stash.print(macString);
    stash.print("&value=");
    stash.print(value);
    stash.save();
    Stash::prepare(PSTR("POST http://$F/api/health/create HTTP/1.1" "\r\n"
      "Host: $F" "\r\n"
      "Content-Type: application/x-www-form-urlencoded" "\r\n"
      "Content-Length: $D" "\r\n"
      "\r\n"
      "$H"),
      website, website, stash.size(), sd);
    ether.tcpSend();
  }
}

/**
 * Method for converting of the byte MAC to hex format
 */
String convertByteMacToHex()
{
  String response = "";
  for (int i = 0; i < sizeof(mymac); i++) {
    if (i > 0) response += ":";
    response += String(mymac[i], HEX);
  }
  return response;
}
