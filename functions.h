Endpoint GetEndpoint(uint8_t ep_id) {
  for (uint8_t i = 0; i < NUM_ENDPOINTS; i++) {
    if (ENDPOINTS[i].id == ep_id) {
      return ENDPOINTS[i];
    }
  }
}


uint32_t packArray(uint8_t *val) {
  uint32_t res = 0;
  for (int i = 0; i < 4; i++) {
    res = res << 8 | val[i];
  }
  return res;
}


void print_hex(uint8_t *data, uint8_t length) {  // prints 8-bit data in hex with leading zeroes
  for (int i=0; i<length; i++) { 
    if (data[i]<0x10) {nss.print("0");} 
    nss.print(data[i],HEX); 
    nss.print(" "); 
  }
}


void printAddr(uint64_t val) {
  uint32_t msb = val >> 32;
  uint32_t lsb = val;
  nss.print(msb, HEX);
  nss.println(lsb, HEX);
}


void build_payload_list(const uint16_t *values, const uint8_t v_size, uint8_t* res) {
  //Build byte Payload in little endian order
  *res = v_size / 2;
  uint8_t c = 0;
  for (uint8_t i = 1; i < (2 * v_size + 1); i += 2) {
    *(res + i) = static_cast<uint8_t>((values[c] & 0x00FF) >> 0);
    *(res + i + 1) = static_cast<uint8_t>((values[c] & 0xFF00) >> 8);
    c++;
  }
}


attribute* readTemp(uint8_t ep_id) {
  float TempC = 0;
  sensors.requestTemperatures();
  
  if (ep_id == 3) {
    TempC = sensors.getTempC(highThermometer);
  }
  else if (ep_id == 4) {
    TempC = sensors.getTempC(lowThermometer);
  }
  
  if (DEBUGlv2) {
    nss.print(F("Read Temp for EP"));
    nss.print(ep_id, HEX);
    nss.print(F(": "));
    nss.print(TempC);
    nss.println(F("°C"));
  }

  Endpoint end_point = GetEndpoint(ep_id);
    Cluster cluster = end_point.GetCluster(TEMP_CLUSTER_ID);
    attribute* Tattr = cluster.GetAttr(0x0000);
  if (TempC < 85 && TempC > 10){
    
    Tattr->val_len = 2;
    uint16_t cor_t = (uint16_t)(TempC * 100.0);
  
    uint8_t t_value[2] = {(uint8_t)cor_t,
                          (uint8_t)(cor_t >> 8)
                         };
    Tattr->value = t_value;
    delay(200);
    return Tattr;
  }
}
