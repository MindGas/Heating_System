#include <SoftwareSerial.h>
#include <XBee.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <arduino-timer.h>
#include <EEPROM.h>
#include "declare.h"
#include "classes.h"
#include "other.h"
#include "functions.h"
#include "send.h"
#include "response.h"


void setup() {
  digitalWrite(SSR_PIN, HIGH);
  Serial.begin(9600);
  
  if (DEBUG) {
    nss.begin(9600);
    nss.println(F("Startup"));
  }

  sensors.begin();

  xbee.setSerial(Serial);

  while(sensors.getDeviceCount() == 0) {
    delay(1000);
  }
  if (DEBUG) {
    nss.print(F("Found "));
    nss.print(sensors.getDeviceCount(), DEC);
    nss.println(F(" devices."));
  }

  sensors.getAddress(inputThermometer, 0);
  sensors.getAddress(outputThermometer, 1);
  sensors.setResolution(inputThermometer, 9);
  sensors.setResolution(outputThermometer, 9);

  pinMode(SSR_PIN, OUTPUT);

  delay(2000);

  //Set up callbacks
  if (DEBUGlv2) {
    xbee.onZBTxStatusResponse(zbTxStatusResp);
    xbee.onModemStatusResponse(modemResp);
    xbee.onOtherResponse(otherResp);
  }
  xbee.onZBExplicitRxResponse(zdoReceive);
  xbee.onAtCommandResponse(atCmdResp);

  sendAT(shCmd);
  sendAT(slCmd);
  
  if (DEBUG) {
    nss.print(F("MAC (SH+SL): "));
    printAddr(macAddr.Get());
  }
  
  sendAT(keyCmd);

  //Update temperature
  timer.every(30000, update_temp);

  //Update our current state
  uint8_t ep_id = 1;
  Endpoint end_point = GetEndpoint(ep_id);
  Cluster cluster = end_point.GetCluster(ON_OFF_CLUSTER_ID);
  attribute* attr = cluster.GetAttr(0x0000);
  *attr->value = 0x01;  
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
  if (!nwk_pending && !assc_pending) {
    setup_complete = 1;
  }
  if (setup_complete && !start) {
    sendDevAnnounce();

    start = 1;
  }
  if (start) {
    
    
  }

  timer.tick();
}
