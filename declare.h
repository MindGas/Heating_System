// Enable debug prints to serial monitor
#define DEBUG true
#define DEBUGlv2 false // extra debugging output over softserial.

#define WATER_TEMP_BUS 7  // Tempetature sensor pin
#define SSR_PIN LED_BUILTIN // LED for Relay On/Off state
#define HR_PIN 5 // Central Heating Relay On/Off pin
#define WR_PIN 6 // Water Heating Relay On/Off pin
#define NUM_ENDPOINTS 4   // Number of EndPoints (2x Temperature, 2x Switch)

#define RELAY_ON 1  // GPIO value to write to turn on attached relay
#define RELAY_OFF 0 // GPIO value to write to turn off attached relay

// Define TX/RX pins for SoftSerial 
#define ssRX 10           // Connect Arduino pin 10 to TX of usb-serial device
#define ssTX 11           // Connect Arduino pin 11 to RX of usb-serial device

SoftwareSerial nss(ssRX, ssTX);

//One wire temp sensors
OneWire oneWire(WATER_TEMP_BUS);
DallasTemperature sensors(&oneWire);
//Water top Temp
DeviceAddress highThermometer; //= {0x28, 0xAA, 0x18, 0xDB, 0x19, 0x13, 0x02, 0x9B}; // populated with correct address with sensors.getAddress(highThermometer, 0)
//Water bottom Temp
DeviceAddress lowThermometer; //= {0x28, 0xAA, 0x3C, 0xB1, 0x3C, 0x14, 0x01, 0xDA}; // populated with correct address with sensors.getAddress(lowThermometer, 1)

XBeeWithCallbacks  xbee;
ZBExplicitTxRequest exp_tx = ZBExplicitTxRequest();

#define UKN_NET_ADDR 0xFFFE
#define HA_PROFILE_ID 0x0104

#define MATCH_DESC_RQST 0x0006
#define MATCH_DESC_RSP 0x8006
#define SIMPLE_DESC_RSP 0x8004
#define ACTIVE_EP_RSP 0x8005
#define ACTIVE_EP_RQST 0x0005
#define SIMPLE_DESC_RQST 0x0004
#define READ_ATTRIBUTES 0x0000

//Input
#define BASIC_CLUSTER_ID 0x0000
//#define IDENTIFY_CLUSTER_ID 0x0003
//#define GROUPS_CLUSTER_ID 0x0004
//#define SCENES_CLUSTER_ID 0x0005
#define ON_OFF_CLUSTER_ID 0x0006
//#define LEVEL_CONTROL_CLUSTER_ID 0x0008
//#define LIGHT_LINK_CLUSTER_ID 0x1000
#define TEMP_CLUSTER_ID 0x0402
//#define HUMIDITY_CLUSTER_ID 0x405
//#define BINARY_INPUT_CLUSTER_ID 0x000f
//#define IAS_ZONE_CLUSTER_ID 0x0500
#define METERING_CLUSTER_ID 0x0702

//Attr id
#define INSTANTANEOUS_DEMAND 0x0400

//Output
//#define OTA_CLUSTER_ID 0x0019 //Upgrade

//Data Type
#define ZCL_CHAR_STR 0x42
//#define ZCL_UINT8_T 0x20
#define ZCL_UINT16_T 0x21
#define ZCL_BOOL 0x10

//Device
//#define ON_OFF_LIGHT 0x0100
//#define DIMMABLE_LIGHT 0x0101
#define TEMPERATURE_SENSOR 0x0302
#define ON_OFF_OUTPUT 0x0002
//#define IAS_ZONE 0x0402

// ieee high
static const uint8_t shCmd[] = {'S', 'H'};
// ieee low
static const uint8_t slCmd[] = {'S', 'L'};
// association status
static const uint8_t assocCmd[] = {'A', 'I'};
// panID
static const uint8_t netCmd[] = {'M', 'Y'};
// Link Key
static const uint8_t keyCmd[] = {'K', 'Y'};

uint8_t netAddr[2];
uint8_t seqID;
uint8_t t_payload[25] = {};
/*
  typedef struct {
  uint16_t id;
  uint8_t* value;
  uint8_t val_len;
  uint8_t type;
  } attribute;
*/

auto timer = timer_create_default(); // create a timer with default settings

LocalMac macAddr = LocalMac(0);

static uint8_t* manuf = (uint8_t*)"xBee-Arduino";

static attribute dev_basic_attr[] {{0x0004, manuf, 12, ZCL_CHAR_STR}, {0x0005, (uint8_t*)"Heating System", 14, ZCL_CHAR_STR}};

static attribute hr_attr[] {{0x0000, 0x00, 1, ZCL_BOOL}};
static attribute wr_attr[] {{0x0000, 0x00, 1, ZCL_BOOL}};
static attribute in_temp_attr[] = {{0x0000, 0x00, 1, ZCL_UINT16_T}};
static attribute out_temp_attr[] = {{0x0000, 0x00, 1, ZCL_UINT16_T}};

//dev_basic_attr
static Cluster hr_in_clusters[] = {Cluster(BASIC_CLUSTER_ID, dev_basic_attr, 2), Cluster(ON_OFF_CLUSTER_ID, hr_attr, 1)};
static Cluster wr_in_clusters[] = {Cluster(ON_OFF_CLUSTER_ID, wr_attr, 1)};
static Cluster i_in_clusters[] = {Cluster(TEMP_CLUSTER_ID, in_temp_attr, 1)};
static Cluster o_in_clusters[] = {Cluster(TEMP_CLUSTER_ID, out_temp_attr, 1)};

static Cluster out_clusters[] = {};

static Endpoint ENDPOINTS[NUM_ENDPOINTS] = {
  Endpoint(1, ON_OFF_OUTPUT, hr_in_clusters, out_clusters, 2, 0),
  Endpoint(2, ON_OFF_OUTPUT, wr_in_clusters, out_clusters, 1, 0),
  Endpoint(3, TEMPERATURE_SENSOR, i_in_clusters, out_clusters, 1, 0),
  Endpoint(4, TEMPERATURE_SENSOR, o_in_clusters, out_clusters, 1, 0),
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

int HRstatus = 0; //Central Heating Relay status updated in time? 0 no / 1 yes
int WRstatus = 0; //Water Heating Relay status updated in time? 0 no / 1 yes

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
