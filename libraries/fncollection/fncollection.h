#ifndef __FNCOLLECTION_H_
#define __FNCOLLECTION_H_

#include <stdint.h>

class FNCOLLECTIONClass {
public:
	FNCOLLECTIONClass();
	void read_eeprom(char *);
	void write_eeprom(char *);
	void write_eeprom(char *, bool);
	void eeprom_init(void);
	void eeprom_factory_reset(char *unused);
	void ewb(uint8_t p, uint8_t v);
	void ewb(uint8_t p, uint8_t v, bool commit);
	void ewc(bool commit);
  void ews(uint8_t p, String data, bool commit = true);
  void ewch(uint8_t p, char* data, bool commit = true);
	uint8_t erb(uint8_t p);
  uint16_t erw(uint8_t p);
  String ers(uint8_t p);
  char* erch(uint8_t p);
	void ledfunc(char *);
	void prepare_boot(char *);
	void version(char *);
	void do_wdt_enable(uint8_t t);
private:
	void dumpmem(uint8_t *addr, uint16_t len);
  void display_string(uint8_t a, uint8_t cnt);
  void display_ee_bytes(uint8_t a, uint8_t cnt);
	void display_ee_mac(uint8_t);
#ifdef HAS_ETHERNET
      void display_ee_ip4(uint8_t a);
#endif
	};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_FNCOLLECTION)
extern FNCOLLECTIONClass FNcol;
#endif

// Already used magics: c1,c2

#define EE_MAGIC_OFFSET      (uint8_t)0                       // 2 bytes

#define EE_CC1100_CFG        (EE_MAGIC_OFFSET+2)                // Offset:  2
#define EE_CC1100_CFG_SIZE   0x29                               // 41
#define EE_CC1100_PA         (EE_CC1100_CFG+EE_CC1100_CFG_SIZE) // Offset 43/2B
#define EE_CC1100_PA_SIZE    8

#define EE_REQBL             (EE_CC1100_PA+EE_CC1100_PA_SIZE)
#define EE_LED               (EE_REQBL+1)
#define EE_FHTID             (EE_LED+1)

#define EE_FASTRF_CFG        (EE_FHTID+2)                       // Offset: 55:37
#define EE_RF_ROUTER_ID      (EE_FASTRF_CFG+EE_CC1100_CFG_SIZE)
#define EE_RF_ROUTER_ROUTER  (EE_RF_ROUTER_ID+1) 

#ifdef HAS_DUDETTE
// if device is equipped with Dudette bootloader (i.e. newer CUNO/COC/CCD/CSM) 
// there is some unique stuff programmed during manufacture:
# define EE_DUDETTE_MAC       (uint8_t)(E2END-5)		// UNIQUE MAC ADDRESS
# define EE_DUDETTE_PRIV      (EE_DUDETTE_MAC-16)		// 128bit RSA private key 
# define EE_DUDETTE_PUBL      (EE_DUDETTE_PRIV-16)		// 128bit RSA public key
#endif

#ifdef HAS_ETHERNET
# define EE_MAC_ADDR         (EE_RF_ROUTER_ROUTER+1)
# define EE_USE_DHCP         (EE_MAC_ADDR+6)                    // Offset x62
# define EE_IP4_ADDR         (EE_USE_DHCP+1)
# define EE_IP4_NETMASK      (EE_IP4_ADDR+4)
# define EE_IP4_GATEWAY      (EE_IP4_NETMASK+4)                 // Offset x71
# define EE_IP4_NTPSERVER    (EE_IP4_GATEWAY+4)                      
# define EE_IP4_TCPLINK_PORT (EE_IP4_NTPSERVER+4)               // Offset x79
# define EE_IP4_NTPOFFSET    (EE_IP4_TCPLINK_PORT+2)
# define EE_STR_LEN        40
# define EE_STR_MQTT       21
# define EE_WPA_SSID_MAX   33
# define EE_WPA_KEY_MAX    64
# ifdef ESP8266
#   define EE_MAX_ENTRY_LEN  64
#   define EE_WPA_SSID       (EE_IP4_NTPOFFSET+1)
#   define EE_WPA_KEY        (EE_WPA_SSID+EE_WPA_SSID_MAX)
#   define EE_NAME           (EE_WPA_KEY+EE_WPA_KEY_MAX)
#   define EE_OTA_SERVER     (EE_NAME+EE_STR_LEN)
#   define EE_ETH_LAST       (EE_OTA_SERVER+4)
# else
#   define EE_MAX_ENTRY_LEN   EE_STR_LEN
#   define EE_ETH_LAST       (EE_IP4_NTPOFFSET+1)
# endif
#else
# define EE_ETH_LAST         (EE_RF_ROUTER_ROUTER+1)
#endif

#ifdef HAS_LCD
# ifdef HAS_ETHERNET
#   define EE_CONTRAST        EE_ETH_LAST
# else
#   define EE_CONTRAST        (EE_FASTRF_CFG+EE_CC1100_CFG_SIZE)
# endif
# define EE_BRIGHTNESS        (EE_CONTRAST+1)
# define EE_SLEEPTIME         (EE_BRIGHTNESS+1)
# define EE_LCD_LAST          (EE_SLEEPTIME+1)
#else
# define EE_LCD_LAST          EE_ETH_LAST
#endif

#ifdef HAS_FS
# define EE_LOGENABLED        EE_LCD_LAST
# define EE_FS_LAST           (EE_LOGENABLED+1)
#else
#	define EE_FS_LAST           EE_LCD_LAST
#endif

#ifdef HAS_MQTT
# define EE_MQTT_SERVER       EE_FS_LAST
# define EE_MQTT_PORT         (EE_MQTT_SERVER+EE_STR_MQTT)
# define EE_MQTT_USER         (EE_MQTT_PORT+4)
# define EE_MQTT_PASS         (EE_MQTT_USER+EE_STR_MQTT)
# define EE_MQTT_CLIENT       (EE_MQTT_PASS+EE_STR_MQTT)
# define EE_MQTT_PRE          (EE_MQTT_CLIENT+EE_STR_MQTT)
# define EE_MQTT_SUB          (EE_MQTT_PRE+EE_STR_MQTT)
# define EE_MQTT_LWT          (EE_MQTT_SUB+EE_STR_MQTT)
# define EE_MQTT_LAST         (EE_MQTT_LWT+EE_STR_MQTT)
#else
# define EE_MQTT_LAST         EE_FS_LAST
#endif

extern uint8_t led_mode;

#endif
