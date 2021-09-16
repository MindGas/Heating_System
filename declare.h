#define DEBUG true
#define DEBUGlv2 true // extra debugging output over softserial. DEBUG must be true before enabling this one

#define WATER_TEMP_BUS 7  // Tempetature sensor pin
#define SSR_PIN LED_BUILTIN // LED for Relay On/Off state
//#define AMP_PIN 16
#define NUM_ENDPOINTS 3   // Number of EndPoints (2x Temperature, 1x Switch)

// Define TX/RX pins for SoftSerial 
#define ssRX 10           // Connect Arduino pin 10 to TX of usb-serial device
#define ssTX 11           // Connect Arduino pin 11 to RX of usb-serial device

SoftwareSerial nss(ssRX, ssTX);

//One wire temp sensors
OneWire oneWire(WATER_TEMP_BUS);
DallasTemperature sensors(&oneWire);
//Water Input Temp
DeviceAddress inputThermometer = {0x28, 0xAA, 0x18, 0xDB, 0x19, 0x13, 0x02, 0x9B}; // populated with correct address with sensors.getAddress(inputThermometer, 0)
//Water Output Temp
DeviceAddress outputThermometer = {0x28, 0xAA, 0x3C, 0xB1, 0x3C, 0x14, 0x01, 0xDA}; // populated with correct address with sensors.getAddress(outputThermometer, 1)

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
