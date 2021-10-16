void sendAT(uint8_t* cmd) {
  AtCommandRequest atRequest = AtCommandRequest();
  atRequest.setCommand((uint8_t*)cmd);
  xbee.send(atRequest);
}


void sendDevAnnounce() {
  /*
     12 byte data payload
     byte 0: Sequence Number
     byte 1-2: Net Addr in Little Endian
     byte 3-10: Mac Addr in Little Endian
     byte 11: Mac Capability Flag, 0x8C = Mains powered device; receiver on when idle; address not self-assigned.
  */
  
  uint64_t mac = SWAP_UINT64(macAddr.Get());
  uint8_t payload[] = {seqID++,
                       netAddr[1],
                       netAddr[0],
                       static_cast<uint8_t>((mac & 0xFF00000000000000) >> 56),
                       static_cast<uint8_t>((mac & 0x00FF000000000000) >> 48),
                       static_cast<uint8_t>((mac & 0x0000FF0000000000) >> 40),
                       static_cast<uint8_t>((mac & 0x000000FF00000000) >> 32),
                       static_cast<uint8_t>((mac & 0x00000000FF000000) >> 24),
                       static_cast<uint8_t>((mac & 0x0000000000FF0000) >> 16),
                       static_cast<uint8_t>((mac & 0x000000000000FF00) >> 8),
                       static_cast<uint8_t>((mac & 0x00000000000000FF) >> 0),
                       0x8C
                      };

  cmd_frame_id = xbee.getNextFrameId();
  last_command = &sendDevAnnounce;

  exp_tx = ZBExplicitTxRequest(COORDINATOR64, //address64
                               UKN_NET_ADDR, //0xFFFE, //address16
                               0x00,    //broadcast radius
                               0x00,    //option
                               payload, //payload
                               sizeof(payload),    //payload length
                               cmd_frame_id, // frame ID
                               0x00,    //src endpoint
                               0x00,    //dest endpoint
                               0x0013,    //cluster ID
                               0x0000 //profile ID
                              );

  xbee.send(exp_tx);

  if (DEBUG) {
    nss.print(F("<- Sent Device Announcment Frame: "));
    print_hex(payload, sizeof(payload));
    nss.println();
  }
}


void sendActiveEpResp() {
  /*
     byte 0 sequence number
     byte 1 status 00 success
     byte 2-3 NWK little endian
     byte 4 Number of active endpoints
     List of active endpoints
  */
  cmd_frame_id = xbee.getNextFrameId();
  last_command = &sendActiveEpResp;
  uint8_t len_payload = 5 + NUM_ENDPOINTS;
  uint8_t payload[len_payload] = {cmd_seq_id, //Has to match requesting packet
                                  0x00,
                                  netAddr[1],
                                  netAddr[0],
                                  NUM_ENDPOINTS,
                                 };

  uint8_t i = 5;
  uint8_t cl_i = 0;
  for (i; i < len_payload; i++) {
    payload[i] = ENDPOINTS[cl_i].id;
    cl_i++;
  }

  exp_tx = ZBExplicitTxRequest(COORDINATOR64,
                               UKN_NET_ADDR,
                               0x00,    //broadcast radius
                               0x00,    //option
                               payload, //payload
                               len_payload,    //payload length
                               cmd_frame_id, // frame ID
                               0x00,    //src endpoint
                               0x00,    //dest endpoint
                               ACTIVE_EP_RSP,    //cluster ID
                               0x0000 //profile ID
                              );

  xbee.send(exp_tx);

  if (DEBUG) {
    nss.print(F("<- Sent Active Endpoint Responce: "));
    print_hex(payload, sizeof(payload));
    nss.println();
  }

}


void sendSimpleDescRpt(uint8_t ep) {
  /*
     byte 0: Sequence number, match requesting packet
     byte 1: status 00= Success
     byte 2-3: NWK in little endian
     byte 4: Length of Simple Descriptor Report
     byte 5: End point report, which endpoint is being reported
     byte 6-7: profile id in little endian 0x0104
     byte 7-8: Device Type in little endian, 0x0007 is combined interface see page 51 of Zigbee HA profile
     byte 9: version number (App Dev)
     byte 10: Input Cluster Count
     byte [] List of output clusters in little endian format
     byte n+1: Output Cluster Count
     byte [] List of output clusters in little endian format
  */

  cmd_frame_id = xbee.getNextFrameId();
  uint8_t num_out = ENDPOINTS[(ep - 1)].GetNumOutClusters();  
  uint8_t out_len = 1; // Still needs to report number of clusters even if cluster is empty
  if (num_out > 0)
  {
     out_len = (2 * num_out) + 1;
  }
  uint8_t num_in = ENDPOINTS[(ep - 1)].GetNumInClusters();  
  uint8_t in_len = 1; // Still needs to report number of clusters even if cluster is empty
  if (num_in > 0)
  {
     in_len = (2 * num_in) + 1;
  }
  //uint8_t pre_len = 11;
  

  uint8_t pre[] = {cmd_seq_id,
                   0x00,
                   netAddr[1],
                   netAddr[0],
                   static_cast<uint8_t>(out_len + in_len + 6), //Length of simple descriptor
                   ep,
                   static_cast<uint8_t>((HA_PROFILE_ID & 0x00FF) >> 0),
                   static_cast<uint8_t>((HA_PROFILE_ID & 0xFF00) >> 8),
                   static_cast<uint8_t>((ENDPOINTS[(ep - 1)].GetDevType() & 0x00FF) >> 0), //Fix me
                   static_cast<uint8_t>((ENDPOINTS[(ep - 1)].GetDevType() & 0xFF00) >> 8),
                   0x01, //Don't Care (App Version)
                  };
  uint8_t pre_len = sizeof(pre);
  uint8_t payload_len = pre_len + out_len + in_len;
  uint8_t in_clusters[in_len];

  memcpy(t_payload, pre, pre_len);

  uint16_t in_cl[num_in];
  ENDPOINTS[(ep - 1)].GetInClusters(in_cl);
  build_payload_list(in_cl, 2 * num_in, in_clusters);

  memcpy(t_payload + pre_len , in_clusters, sizeof(in_clusters));

  uint8_t out_clusters[out_len];
  uint16_t out_cl[num_out];
  ENDPOINTS[(ep - 1)].GetOutClusters(out_cl);
  build_payload_list(out_cl, 2 * num_out, out_clusters);

  memcpy(t_payload + pre_len + sizeof(in_clusters) , out_clusters, sizeof(out_clusters));

  exp_tx = ZBExplicitTxRequest(COORDINATOR64,
                               UKN_NET_ADDR,
                               0x00,    //broadcast radius
                               0x00,    //option
                               t_payload, //payload
                               payload_len,    //payload length
                               cmd_frame_id, // frame ID
                               0x00,    //src endpoint
                               0x00,    //dest endpoint
                               SIMPLE_DESC_RSP,    //cluster ID
                               0x0000 //profile ID
                              );

  xbee.send(exp_tx);

  if (DEBUG) {
    nss.print(F("<- Sent Simple Descriptor Responce for EP "));
    nss.print(ep, HEX);
    nss.print(F(": "));
    print_hex(t_payload, payload_len);
    nss.println("");

    if (DEBUGlv2) {
      nss.print(F("     cmd_seq_id: "));
      nss.println(cmd_seq_id, HEX);
      nss.print(F("     cmd_frame_id: "));
      nss.println(cmd_frame_id, HEX); 
      nss.print(F("     num_out: "));
      nss.println(num_out, HEX);
      nss.print(F("     out_len: "));
      nss.println(out_len, HEX);
      nss.print(F("     num_in: "));
      nss.println(num_in, HEX);
      nss.print(F("     in_len: "));
      nss.println(in_len, HEX); 
      nss.print(F("     pre_len: "));
      nss.println(pre_len, HEX);
      nss.print(F("     payload_len: "));
      nss.println(payload_len, HEX);
      
      nss.print(F("     pre: "));
      print_hex(pre, sizeof(pre));
      nss.println();
      nss.print(F("     in_clusters: "));
      print_hex(in_clusters, sizeof(in_clusters));
      nss.println();
      nss.print(F("     out_clusters: "));
      print_hex(out_clusters, sizeof(out_clusters));
      nss.println();
    }
  }
}


void sendAttributeWriteRsp(uint16_t cluster_id, attribute* attr, uint8_t src_ep, uint8_t dst_ep,  uint8_t result) {
  /*
    payload
    byte 0: frame control
    byte 1 Seq
    byte 2 cmd id
    byte 3-4: Attr Id
    byte 5: type
    bytes6: Success 0x01
    -----------------------------
    CMDS: 0x0A Report Attr
          0x01 Read Attr Response
          0x0D Discover Attributes Response
          0x04 Write Attr Response

  */
  uint8_t payload_len = 6 + attr->val_len;

  uint8_t pre[payload_len] = {0x00,
                              cmd_seq_id,
                              0x0A,  //0x04, //Write attr resp
                              static_cast<uint8_t>((attr->id & 0x00FF) >> 0),
                              static_cast<uint8_t>((attr->id & 0xFF00) >> 8),
                              attr->type,
                              result
                             };


  memcpy(t_payload, pre, sizeof(pre));
  cmd_frame_id = xbee.getNextFrameId();

  exp_tx = ZBExplicitTxRequest(COORDINATOR64,
                               COORDINATOR_NWK,
                               0x00,    //broadcast radius
                               0x00,    //option
                               t_payload, //payload
                               payload_len,    //payload length
                               cmd_frame_id, // frame ID
                               src_ep,    //src endpoint
                               dst_ep,    //dest endpoint
                               cluster_id,    //cluster ID
                               HA_PROFILE_ID //profile ID
                              );


  if (attr->type != 0) {
    xbee.send(exp_tx);
    if (DEBUG) {
      nss.println(F("<- Sent Attr Write Rsp"));
    }
  }
}


void sendAttributeRpt(uint16_t cluster_id, attribute* attr, uint8_t src_ep, uint8_t dst_ep) {
  /*
    payload
    byte 0: frame control
    byte 1 Seq
    byte 2 cmd id
    byte 3-4: Attr Id
    byte 5: type
    bytes[] value in little endian
    -----------------------------
    CMDS: 0x0A Report Attr
          0x01 Read Attr Response
          0x0D Discover Attributes Response
          0x04 Write Attr Response
  */

  uint8_t payload_len;
  if (attr->type == ZCL_CHAR_STR){
    payload_len = 7 + attr->val_len;
    uint8_t pre[] = {0x00, 
                     cmd_seq_id,
                     0x0A, //Report attr 
                     static_cast<uint8_t>((attr->id & 0x00FF) >> 0),
                     static_cast<uint8_t>((attr->id & 0xFF00) >> 8),
                     attr->type,
                     attr->val_len,
                    };
    memcpy(t_payload, pre, sizeof(pre));
    memcpy(t_payload + 7, attr->value, attr->val_len);
  }
  else {
    payload_len = 6 + attr->val_len;
    uint8_t pre[] = {0x00,
                     cmd_seq_id,
                     0x0A, //Report Attr
                     static_cast<uint8_t>((attr->id & 0x00FF) >> 0),
                     static_cast<uint8_t>((attr->id & 0xFF00) >> 8),
                     attr->type,
                     //attr->val_len,
                    };
    memcpy(t_payload, pre, sizeof(pre));
    memcpy(t_payload + 6, attr->value, attr->val_len);
  }

  cmd_frame_id = xbee.getNextFrameId();

  exp_tx = ZBExplicitTxRequest(COORDINATOR64,
                               UKN_NET_ADDR, //COORDINATOR_NWK,
                               0x00,    //broadcast radius
                               0x00,    //option
                               t_payload, //payload
                               payload_len,    //payload length
                               cmd_frame_id, // frame ID
                               src_ep,    //src endpoint
                               dst_ep,    //dest endpoint
                               cluster_id,    //cluster ID
                               HA_PROFILE_ID //profile ID
                              );

  if (attr->type != 0) {
    xbee.send(exp_tx);
    if (DEBUG) {
      nss.print(F("<- Sent Attribute Report: "));
      print_hex(t_payload, payload_len);
      nss.println();
    }
    if (DEBUGlv2) {
      nss.print(F("     cluster_id: "));
      nss.println(cluster_id, HEX);
      nss.print(F("     attr->id: "));
      nss.println(attr->id, HEX);
      nss.print(F("     attr->type: "));
      nss.println(attr->type, HEX);
      nss.print(F("     attr->val_len: "));
      nss.println(attr->val_len, HEX);
      nss.print(F("     attr->value: "));
      print_hex(attr->value, attr->val_len); // Fix. Sends correct value but prints wrong attr->value
      nss.println();
      nss.print(F("     src_ep: "));
      nss.println(src_ep, HEX);
      nss.print(F("     dst_ep: "));
      nss.println(dst_ep, HEX);
    }
  }
}


void sendAttributeRsp(uint16_t cluster_id, attribute* attr, uint8_t src_ep, uint8_t dst_ep, uint8_t cmd) {
  /*
    payload
    byte 0: frame control
    byte 1 Seq
    byte 2 cmd id
    byte 3-4: Attr Id
    byte 5: type
    bytes[] value in little endian
    -----------------------------
    CMDS: 0x0A Report Attr
          0x01 Read Attr Response
          0x0D Discover Attributes Response
          0x04 Write Attr Response
  */

  uint8_t payload_len;
  if (attr->type == ZCL_CHAR_STR){
    payload_len = 8 + attr->val_len;
    uint8_t pre[] = {0x00, 
                     cmd_seq_id,
                     cmd, //Read attr resp
                     static_cast<uint8_t>((attr->id & 0x00FF) >> 0),
                     static_cast<uint8_t>((attr->id & 0xFF00) >> 8),
                     0x00,//status
                     attr->type,
                     attr->val_len,
                    };
    memcpy(t_payload, pre, sizeof(pre));
    memcpy(t_payload + 8, attr->value, attr->val_len);
  }
  else {
   
    payload_len = 7 + attr->val_len;
    //payload_len = 8 + attr->val_len;
    uint8_t pre[] = {0x00,
                     cmd_seq_id,
                     cmd, //Read attr resp
                     static_cast<uint8_t>((attr->id & 0x00FF) >> 0),
                     static_cast<uint8_t>((attr->id & 0xFF00) >> 8),
                     0x00,
                     attr->type,
                     //attr->val_len, // added for testing
                    };
    memcpy(t_payload, pre, sizeof(pre));
    memcpy(t_payload + 7, attr->value, attr->val_len);
    //memcpy(t_payload + 8, attr->value, attr->val_len);
  }

  cmd_frame_id = xbee.getNextFrameId();

  exp_tx = ZBExplicitTxRequest(COORDINATOR64,
                               COORDINATOR_NWK,
                               0x00,    //broadcast radius
                               0x00,    //option
                               t_payload, //payload
                               payload_len,    //payload length
                               cmd_frame_id, // frame ID
                               src_ep,    //src endpoint
                               dst_ep,    //dest endpoint
                               cluster_id,    //cluster ID
                               HA_PROFILE_ID //profile ID
                              );

  if (attr->type != 0) {
    xbee.send(exp_tx);
    if (DEBUG) {
      nss.print(F("<- Sent Attribute Response: "));
      print_hex(t_payload, payload_len);
      nss.println();
    }
    if (DEBUGlv2) {
      nss.print(F("     cluster_id: "));
      nss.println(cluster_id, HEX);
      nss.print(F("     attr->id: "));
      nss.println(attr->id, HEX);
      nss.print(F("     attr->type: "));
      nss.println(attr->type, HEX);
      nss.print(F("     attr->val_len: "));
      nss.println(attr->val_len, HEX);
      nss.print(F("     attr->value: "));
      print_hex(attr->value, attr->val_len);
      nss.println();
      nss.print(F("     src_ep: "));
      nss.println(src_ep, HEX);
      nss.print(F("     dst_ep: "));
      nss.println(dst_ep, HEX);
      nss.print(F("     payload_len: "));
      nss.println(payload_len, HEX);
    }
  }
}


void SetAttr(uint8_t ep_id, uint16_t cluster_id, uint16_t attr_id, uint8_t value) {
  if (ep_id == 0x01) {
    HRstatus = 1;
  }
  if (ep_id == 0x02) {
    WRstatus = 1;
  }
  
  Endpoint end_point = GetEndpoint(ep_id);
  Cluster cluster = end_point.GetCluster(cluster_id);
  attribute* attr = cluster.GetAttr(attr_id);
  if (cluster_id == ON_OFF_CLUSTER_ID) {
    *attr->value = value; //breaking
    if (value == 0x00) {
      if (ep_id == 0x01) {
        digitalWrite(HR_PIN, RELAY_OFF);
      }
      else if (ep_id == 0x02) {
        digitalWrite(WR_PIN, RELAY_OFF);
      }
      if (DEBUG) {
        nss.print(F("     Turn Off EP"));
        nss.println(end_point.id);
      }
      digitalWrite(SSR_PIN, LOW);
    }
    else if (value == 0x01) {
      if (ep_id == 0x01) {
        digitalWrite(HR_PIN, RELAY_ON);
      }
      else if (ep_id == 0x02) {
        digitalWrite(WR_PIN, RELAY_ON);
      }
      if (DEBUG) {
        nss.print(F("     Turn On EP"));
        nss.println(end_point.id);
      }
      //if () {
        digitalWrite(SSR_PIN, HIGH);
      //}
    }
  }
  sendAttributeWriteRsp(cluster_id, attr, ep_id, 0x01, value);
    nss.print(F("     cluster_id: "));
    nss.println(cluster_id, HEX);
    nss.print(F("     attr_id: "));
    nss.println(attr_id, HEX);
    nss.print(F("     value: "));
    nss.println(value, HEX);
}


//Update temp, serial for now
bool  update_temp(void *) {
  uint8_t in_ep_id = 3;
  uint8_t out_ep_id = 4;

  sendAttributeRpt(TEMP_CLUSTER_ID, readTemp(in_ep_id), in_ep_id, 0x01);

  sendAttributeRpt(TEMP_CLUSTER_ID, readTemp(out_ep_id), out_ep_id, 0x01);
  return true; // repeat? true
}
