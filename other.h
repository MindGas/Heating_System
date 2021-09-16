uint8_t netAddr[2];

//uint8_t frameID;
uint8_t seqID;

auto timer = timer_create_default(); // create a timer with default settings

uint8_t t_payload[25] = {};
/*
  typedef struct {
  uint16_t id;
  uint8_t* value;
  uint8_t val_len;
  uint8_t type;
  } attribute;
*/

LocalMac macAddr = LocalMac(0);

static uint8_t* manuf = (uint8_t*)"xBee-Arduino";

static attribute dev_basic_attr[] {{0x0004, manuf, 12, ZCL_CHAR_STR}, {0x0005, (uint8_t*)"Heating System", 14, ZCL_CHAR_STR}};
static attribute in_temp_basic_attr[] {{0x0004, manuf, 12, ZCL_CHAR_STR}, {0x0005, (uint8_t*)"High Temp", 9, ZCL_CHAR_STR}};
static attribute out_temp_basic_attr[] {{0x0004, manuf, 12, ZCL_CHAR_STR}, {0x0005, (uint8_t*)"Low Temp", 8, ZCL_CHAR_STR}};

static attribute ssr_attr[] {{0x0000, 0x00, 1, ZCL_BOOL}};
static attribute metering_attr[] = {{INSTANTANEOUS_DEMAND, 0x00, 1, ZCL_UINT16_T}};
static attribute in_temp_attr[] = {{0x0000, 0x00, 1, ZCL_UINT16_T}};
static attribute out_temp_attr[] = {{0x0000, 0x00, 1, ZCL_UINT16_T}};

//dev_basic_attr

static Cluster s_in_clusters[] = {Cluster(BASIC_CLUSTER_ID, dev_basic_attr, 2), Cluster(ON_OFF_CLUSTER_ID, ssr_attr, 1), Cluster(METERING_CLUSTER_ID, metering_attr, 1)};
static Cluster i_in_clusters[] = {Cluster(BASIC_CLUSTER_ID, in_temp_basic_attr, 2), Cluster(TEMP_CLUSTER_ID, in_temp_attr, 1)};
static Cluster o_in_clusters[] = {Cluster(BASIC_CLUSTER_ID, out_temp_basic_attr, 2), Cluster(TEMP_CLUSTER_ID, out_temp_attr, 1)};

static Cluster out_clusters[] = {};

static Endpoint ENDPOINTS[NUM_ENDPOINTS] = {
  Endpoint(1, ON_OFF_OUTPUT, s_in_clusters, out_clusters, 3, 0),
  Endpoint(2, TEMPERATURE_SENSOR, i_in_clusters, out_clusters, 2, 0),
  Endpoint(3, TEMPERATURE_SENSOR, o_in_clusters, out_clusters, 2, 0),
};

XBeeAddress64 COORDINATOR64 = XBeeAddress64(0, 0);
uint16_t COORDINATOR_NWK = 0x0000;

bool is_joined = 0;
bool start = 0;
uint8_t associated = 1;
bool setup_complete = 0;
bool nwk_pending = 0;
bool assc_pending = 0;

uint8_t msb[4];
uint8_t lsb[4];
uint8_t shdone = 0;
uint8_t sldone = 0;

uint8_t cmd_frame_id;
typedef void (*cmd_ptr)();
cmd_ptr last_command;
uint8_t cmd_seq_id;


uint64_t SWAP_UINT64(uint64_t num) {
  uint64_t byte0, byte1, byte2, byte3, byte4, byte5, byte6, byte7;
  byte0 = (num & 0x00000000000000FF) >> 0 ;
  byte1 = (num & 0x000000000000FF00) >> 8 ;
  byte2 = (num & 0x0000000000FF0000) >> 16 ;
  byte3 = (num & 0x00000000FF000000) >> 24 ;
  byte4 = (num & 0x000000FF00000000) >> 32 ;
  byte5 = (num & 0x0000FF0000000000) >> 40 ;
  byte6 = (num & 0x00FF000000000000) >> 48 ;
  byte7 = (num & 0xFF00000000000000) >> 56 ;
  return ((byte0 << 56) | (byte1 << 48) | (byte2 << 40) | (byte3 << 32) | (byte4 << 24) | (byte5 << 16) | (byte6 << 8) | (byte7 << 0));
}


Endpoint GetEndpoint(uint8_t ep_id) {
  for (uint8_t i = 0; i < NUM_ENDPOINTS; i++) {
    if (ENDPOINTS[i].id == ep_id) {
      return ENDPOINTS[i];
    }
  }
}
