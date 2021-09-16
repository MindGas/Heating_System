void zbTxStatusResp(ZBTxStatusResponse& resp, uintptr_t) {
  if (resp.isSuccess()) {
    nss.println(F("TX OK"));

  }
  else {
    nss.print(F("TX FAIL: "));
    nss.println(resp.getDeliveryStatus(), HEX);

    //if (resp.getFrameId() == cmd_frame_id) {
      //last_command();
    //}
  }
}


void modemResp(ModemStatusResponse& resp, uintptr_t) {
  nss.print(F("Modem Status: "));
  
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
  nss.println(F("Other Response"));
}


void atCmdResp(AtCommandResponse& resp, uintptr_t) {
  nss.print(F("AT command: "));
  
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
          case 33:
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
            break;
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

  if (erx.getRemoteAddress16() == 0 ) {
    if (DEBUG) {
      nss.print(F("Received ZDO: "));
      nss.println(erx.getClusterId(), HEX);
    }
    if (erx.getClusterId() == ACTIVE_EP_RQST) {
      //Have to match sequence number in response
      cmd_seq_id = erx.getFrameData()[erx.getDataOffset()];
      if (DEBUG) {
        nss.print("Received Active Enpoint Request: ");
        print_hex(erx.getFrameData(), erx.getFrameDataLength());
        nss.println("");
      }
      sendActiveEpResp();
    }
    else if (erx.getClusterId() == SIMPLE_DESC_RQST) {
      //Have to match sequence number in response
      cmd_seq_id = erx.getFrameData()[erx.getDataOffset()];
      //Payload is EndPoint
      uint8_t ep = erx.getFrameData()[erx.getDataOffset() + 3];
      if (DEBUG) {
        nss.print("Received Simple Descriptor Request for EP ");
        nss.print(ep, HEX);
        nss.print(F(": "));
        print_hex(erx.getFrameData(), erx.getFrameDataLength());
        nss.println("");
      }
      sendSimpleDescRpt(ep);
    }
    else if (erx.getClusterId() == ON_OFF_CLUSTER_ID) {
      uint8_t len_data = erx.getDataLength() - 3;
      uint16_t attr_rqst[len_data / 2];
      if (DEBUG) {
        nss.print(F("ON/OFF Cl: "));
        for (uint8_t i = erx.getDataOffset(); i < (erx.getDataLength() + erx.getDataOffset() + 3); i ++) {
          nss.print(erx.getFrameData()[i]);
        }
        nss.println();
      }
      cmd_seq_id = erx.getFrameData()[erx.getDataOffset() + 1];
      uint8_t ep = erx.getDstEndpoint();
      uint8_t cmd_id = erx.getFrameData()[erx.getDataOffset() + 2];
      Endpoint end_point = GetEndpoint(ep);
      if (cmd_id == 0x00) {
        if (DEBUG) {
          nss.println(F("Cmd Off"));
        }
        SetAttr(ep, erx.getClusterId(), 0x0000, 0x00);
      }
      else if (cmd_id == 0x01) {
        if (DEBUG) {
          nss.println(F("Cmd On"));
        }
        SetAttr(ep, erx.getClusterId(), 0x0000, 0x01);
      }
      else {
        if (DEBUG) {
          nss.print(F("Cmd Id: "));
          nss.println(cmd_id, HEX);
        }
      }
    }
      else if (erx.getClusterId() == READ_ATTRIBUTES) { //SHould be basic cluster id
      cmd_seq_id = erx.getFrameData()[erx.getDataOffset() + 1];
      uint8_t ep = erx.getDstEndpoint();
      if (DEBUG) {
        nss.println(F("Clstr Rd Att:"));
        nss.print(F("Cmd Seq: "));
        nss.println(cmd_seq_id);
      }

      uint8_t len_data = erx.getDataLength() - 3;
      uint16_t attr_rqst[len_data / 2];

      Endpoint end_point = GetEndpoint(ep);
      for (uint8_t i = erx.getDataOffset() + 3; i < (len_data + erx.getDataOffset() + 3); i += 2) {
        attr_rqst[i / 2] = (erx.getFrameData()[i + 1] << 8) |
                           (erx.getFrameData()[i] & 0xff);
        attribute* attr = end_point.GetCluster(erx.getClusterId()).GetAttr(attr_rqst[i / 2]);
        sendAttributeRsp(erx.getClusterId(), attr, ep, ep, 0x01);
      }

    }
  }
}
