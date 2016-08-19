//This data should reflect your SIM card and your operator's settings. Refer to your operators website for the information.
#define PINNUMBER ""  // PIN Number

// APN data
#define GPRS_APN       "internet.tele2.ee" //GPRS APN
#define GPRS_LOGIN     "wap"    //GPRS login
#define GPRS_PASSWORD  "wap" //GPRS password

//How many updates to collect before uploading them to the server.
#define maxUpdates 20
#define WDTCOUNT 10
#define DELAYTIME 1800000 // 300000 //

#define xbeeRssiPin 47
#define MODEMSLEEPPIN 34
#define STATUSPIN 10
#define XBEE_RTS 46
#define XBEE_RESET 45
#define MODEM_POWER_PIN 24
#define MODEM_STATUS_PIN 35
#define BATTERY_VOLTAGE_PIN A14
#define MAIN_TEMPERATURE A1
#define LED 7 // Led on PIN 13, PB7

// Power saving mode defines
#define VOLTAGE_NORMAL 814      // 3.6V
#define VOLTAGE_POWER_SAVE 745  // 3.3V

enum power_states {
        MODE_NORMAL = 0,        // > 3.6V
        MODE_POWER_SAVE,        // 3.3V - 3.6
        MODE_SLEEP              // < 3.3V
};

char *power_state_names[] = {"MODE_NORMAL", "MODE_POWER_SAVE", "MODE_SLEEP"};
