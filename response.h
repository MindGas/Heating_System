void zbTxStatusResp(ZBTxStatusResponse& resp, uintptr_t) {
  if (resp.isSuccess()) {
    if (DEBUG) {
      if (DEBUGlv2) {
        nss.println(F("!! TX OK"));
      }
    }
  }
  else {
    if (DEBUG) {
      if (DEBUGlv2) {
        nss.print(F("!! TX FAIL: "));
        nss.println(resp.getDeliveryStatus(), HEX);
      }
    }

    if (resp.getFrameId() == cmd_frame_id) {
      last_command();
    }
  }
}


void modemResp(ModemStatusResponse& resp, uintptr_t) {
  nss.print(F("!! Modem Status: "));
  
  if (resp.getStatus() == 0) {
    nss.println("Hardware Reset");
  }
  else if (resp.getStatus() == 2) {
    nss.println("Joined network");
  }
  else {
    nss.println(resp.getStatus(), HEX);
  }
}


void otherResp(XBeeResponse& resp, uintptr_t) {
  nss.println(F("-> Other Network Response"));
}


void atCmdResp(AtCommandResponse& resp, uintptr_t) {
  if (DEBUG) {
    nss.print(F("  AT command: "));
  }
  
  if (resp.getStatus() == AT_OK) {
    if (resp.getCommand()[0] == assocCmd[0] &&
        resp.getCommand()[1] == assocCmd[1]) {
      //Association Status
      associated = resp.getValue()[0];
      assc_pending = 0;
      
      if (DEBUG) {
        nss.print(char(resp.getCommand()[0]));
        nss.print(char(resp.getCommand()[1]));
        nss.print(" ; Response: ");
        switch (associated) {
          case 0:
            nss.println("Successful");
            break;
          // Arduino Uno running out of memory, slimming debugging code
          /*case 33:
            nss.println("Scan found no PANs");
            break;
          case 34:
            nss.println("Scan found no valid PANs based on current SC and ID settings");
            break;
          case 35:
            nss.println("Valid Coordinator or Routers found, but they are not allowing joining (NJ expired)");
            break;
          case 39:
            nss.println("Node Joining attempt failed (typically due to incompatible security settings)");
            break;
          case 255:
            nss.println("Initialization time - FF");
            break;*/
          default:
            nss.println(associated, HEX);
            break;
        }
      }
    }
    else if (resp.getCommand()[0] == netCmd[0] &&
             resp.getCommand()[1] == netCmd[1]) {
      //NWK
      netAddr[0] = resp.getValue()[0];
      netAddr[1] = resp.getValue()[1];
      nwk_pending = 0;
      
      if (DEBUG) {
        nss.print(char(resp.getCommand()[0]));
        nss.print(char(resp.getCommand()[1]));
        nss.print(" ; Response: ");
        nss.print(netAddr[0], HEX);
        nss.println(netAddr[1], HEX);
      }
    }
    else if (resp.getCommand()[0] == shCmd[0] &&
             resp.getCommand()[1] == shCmd[1]) {
      //SH
      if (DEBUG) {
        nss.print(char(resp.getCommand()[0]));
        nss.print(char(resp.getCommand()[1]));
        nss.print(" ; Response: ");
      }
      if (resp.getValueLength() == 4) {
        for (int i = 0; i < resp.getValueLength(); i++) {
            msb[i] = resp.getValue()[i];
            if (DEBUG) {
              nss.print(resp.getValue()[i], HEX);
            }
        }
        shdone=1;
        if (DEBUG) {
          nss.println();
        }
      }
    }
    else if (resp.getCommand()[0] == slCmd[0] &&
             resp.getCommand()[1] == slCmd[1]) {
      //SL
      if (DEBUG) {
        nss.print(char(resp.getCommand()[0]));
        nss.print(char(resp.getCommand()[1]));
        nss.print(" ; Response: ");
      }
      if (resp.getValueLength() == 4) {
        for (int i = 0; i < resp.getValueLength(); i++) {
            lsb[i] = resp.getValue()[i];
            if (DEBUG) {
              nss.print(resp.getValue()[i], HEX);
            }
        }
        sldone=1;
        if (DEBUG) {
          nss.println();
        }
      }     
    }
    else if (resp.getCommand()[0] == keyCmd[0] &&
             resp.getCommand()[1] == keyCmd[1]) {
      //Link Key
      if (DEBUG) {
        nss.print(char(resp.getCommand()[0]));
        nss.print(char(resp.getCommand()[1]));
        nss.print(" ; Response: ");
        if (resp.getValue()[0] == 0) {
          nss.println("No link key defined");
        }
        else {
          nss.println("Link Key Set");
        }
      }  
    }
    else {
      if (DEBUG) {
        nss.println(F("   Ukn Cmd"));
      }
    }
    if (shdone == 1 && sldone ==1) {
      macAddr.Set(XBeeAddress64(packArray(msb), packArray(lsb)));
      shdone=0;
      sldone=0;
    }
  }
  else {
    if (DEBUG) {
      nss.println(F("AT Fail"));
    }
  }
}


void zdoReceive(ZBExplicitRxResponse& erx, uintptr_t) {
  // Create a reply packet containing the same data
  // This directly reuses the rx data array, which is ok since the tx
  // packet is sent before any new response is received
  uint8_t ep = erx.getDstEndpoint();
  uint8_t cmd_id = erx.getFrameData()[erx.getDataOffset() + 2];
  cmd_seq_id = erx.getFrameData()[erx.getDataOffset() + 1];

  if (erx.getRemoteAddress16() == 0 ) {
    if (DEBUG) {
      if (DEBUGlv2) {
        nss.print(F("-> Received ZDO: "));
        nss.println(erx.getClusterId(), HEX);
      }
    }
    if (erx.getClusterId() == ACTIVE_EP_RQST) {
      //Have to match sequence number in response
      cmd_seq_id = erx.getFrameData()[erx.getDataOffset()];
      if (DEBUG) {
        nss.print("-> Received Active Enpoint Request: ");
        print_hex(erx.getFrameData(), erx.getFrameDataLength());
        nss.println();
      }
      sendActiveEpResp();
    }
    else if (erx.getClusterId() == SIMPLE_DESC_RQST) {
      //Have to match sequence number in response
      cmd_seq_id = erx.getFrameData()[erx.getDataOffset()];
      //Payload is EndPoint
      ep = erx.getFrameData()[erx.getDataOffset() + 3];
      if (DEBUG) {
        nss.print("-> Received Simple Descriptor Request for EP");
        nss.print(ep, HEX);
        nss.print(F(": "));
        print_hex(erx.getFrameData(), erx.getFrameDataLength());
        nss.println("");
      }
      sendSimpleDescRpt(ep);
    }
    else if (erx.getClusterId() == ON_OFF_CLUSTER_ID) {
      if (DEBUG) {
        nss.print("-> Received On/Off Cluster Request for EP");
        nss.print(ep, HEX);
        nss.print(F(": "));
        print_hex(erx.getFrameData(), erx.getFrameDataLength());
        nss.println("");
      }
      Endpoint end_point = GetEndpoint(ep);
      if (cmd_id == 0x00) {
        SetAttr(ep, erx.getClusterId(), 0x0000, 0x00);
      }
      else if (cmd_id == 0x01) {
        SetAttr(ep, erx.getClusterId(), 0x0000, 0x01);
      }
      else {
        if (DEBUG) {
          if (DEBUGlv2) {
            // Command B looks like acknowledgment. No use for it now
            nss.print(F("     Received Unknown On/Off Cluster Command ("));
            nss.print(cmd_id, HEX);
            nss.print(F(") for EP"));
            nss.println(ep, HEX);
          }
        }
      }
    }
    else if (erx.getClusterId() == TEMP_CLUSTER_ID) {
      Endpoint end_point = GetEndpoint(ep);
      if (cmd_id == 0x00) {
        if (DEBUG) {
          nss.print("-> Received Temperature Attribute Request for EP");
          nss.print(ep, HEX);
          nss.print(F(": "));
          print_hex(erx.getFrameData(), erx.getFrameDataLength());
          nss.println("");
        }
        sendAttributeRsp(erx.getClusterId(), readTemp(ep), ep, erx.getSrcEndpoint(), 0x01);
      }
      else {
        if (DEBUG) {
          if (DEBUGlv2) {
            // Command B looks like acknowledgment. No use for it now
            nss.print("-> Received Unknown Temperature Cluster Command (");
            nss.print(cmd_id, HEX);
            nss.print(F(") for EP"));
            nss.println(ep, HEX);
          }
        }
      }
    }
    else if (erx.getClusterId() == BASIC_CLUSTER_ID) {
      if (cmd_id == 0x00) {
        Endpoint end_point = GetEndpoint(ep);
        uint8_t i = erx.getDataOffset() + 3;
        uint16_t attr_rqst = (erx.getFrameData()[i + 1] << 8) |
                             (erx.getFrameData()[i] & 0xff);
        attribute* attr = end_point.GetCluster(erx.getClusterId()).GetAttr(attr_rqst);

        if (DEBUG) {
          nss.print(F("-> Received Basic Cluster Attribute Request for EP"));
          nss.print(ep, HEX);
          nss.print(F(": "));
          print_hex(erx.getFrameData(), erx.getFrameDataLength());
          nss.println();

          if (DEBUGlv2) {
            nss.print("attr ID: ");
            nss.println(attr_rqst);
            nss.print("CMD ID: ");
            nss.println(cmd_id, HEX);
            nss.print("Data Offset: ");
            nss.println(erx.getDataOffset(), HEX);
          }
        }
        sendAttributeRsp(erx.getClusterId(), attr, ep, erx.getSrcEndpoint(), 0x01);
      }
      else {
        if (DEBUG) {
          if (DEBUGlv2) {
            // Command B looks like acknowledgment. No use for it now
            nss.print("-> Received Unknown Basic Cluster Command (");
            nss.print(cmd_id, HEX);
            nss.print(F(") for EP"));
            nss.println(ep, HEX);
          }
        }
      }
    }
  }
}
