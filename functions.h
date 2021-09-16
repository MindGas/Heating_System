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
