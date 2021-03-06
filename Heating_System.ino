#include <SoftwareSerial.h>
#include <XBee.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <arduino-timer.h>
#include <EEPROM.h>
#include "classes.h"
#include "declare.h"
#include "functions.h"
#include "send.h"
#include "response.h"


void setup() {
  digitalWrite(PWR_PIN, HIGH);
  digitalWrite(HRL_PIN, LOW);
  digitalWrite(WRL_PIN, LOW);
  Serial.begin(9600);

  if (DEBUG) {
    nss.begin(9600);
    nss.println();
    nss.println(F("### Starting up XBee-Arduino ###"));
  }

  sensors.begin();

  xbee.setSerial(Serial);

  // Make sure relays are off when starting up
  digitalWrite(HR_PIN, RELAY_OFF);
  digitalWrite(WR_PIN, RELAY_OFF);
  // Then set relay pins in output mode
  pinMode(HR_PIN, OUTPUT);
  pinMode(WR_PIN, OUTPUT);
  
  if (DEBUG) {
    if (DEBUGlv2) {
      nss.println(F("  Starting temperature device search"));
    }
  }
  while(sensors.getDeviceCount() == 0) {
    delay(1000);
  }
  if (DEBUG) {
    nss.print(F("  Found "));
    nss.print(sensors.getDeviceCount(), DEC);
    nss.println(F(" temperature sensors."));
  }

  sensors.getAddress(highThermometer, 0);
  sensors.getAddress(lowThermometer, 1);
  sensors.setResolution(highThermometer, 9);
  sensors.setResolution(lowThermometer, 9);

  pinMode(PWR_PIN, OUTPUT);
  pinMode(HRL_PIN, OUTPUT);
  pinMode(WRL_PIN, OUTPUT);

  delay(2000);

  //Set up callbacks
  if (DEBUG) {
    if (DEBUGlv2) {
      xbee.onModemStatusResponse(modemResp);
      xbee.onOtherResponse(otherResp);
    }
  }
  xbee.onZBTxStatusResponse(zbTxStatusResp);
  xbee.onZBExplicitRxResponse(zdoReceive);
  xbee.onAtCommandResponse(atCmdResp);

  sendAT(shCmd);
  sendAT(slCmd);
  
  if (DEBUG) {
    nss.print(F("  MAC (SH+SL): "));
    printAddr(macAddr.Get());
  }
  
  sendAT(keyCmd);

  //Update temperature
  timer.every(30000, update_temp);

  //Check if relay status received
  timer.every(60000, check_relay_status);
}


void loop() {
  xbee.loop();
  
  if (associated != 0 && !assc_pending) {
    assc_pending = 1;
    sendAT(assocCmd);
  }
  
  if (netAddr[0] == 0 && netAddr[1] == 0 && !nwk_pending && !assc_pending) {
    nwk_pending = 1;
    sendAT(netCmd);
  }
  
  if (!nwk_pending && !assc_pending && !setup_complete) {
    setup_complete = 1;
    if (DEBUG) {
      nss.println(F("### Setup finished. Starting Main Loop ###"));
      nss.println();
    }
  }
  
  if (setup_complete && !start) {
    sendDevAnnounce();
    start = 1;
  }
  
  //if (start) {
    
  //}

  timer.tick();
}
