#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#ifndef WIFI_SSID
    #define WIFI_SSID "SSID" // Put you SSID and Password here
    #define WIFI_PWD "ssidpwd
#endif

#define LOCATION "Lisbon"
#define METRICS  "metric"   
#define OWAPIKEY "ow key"   // Put your OpenWeather Key here.

// My Phant server stream for logging heap
#define PHANTSERVER  "192.168.1.17:8080"
#define PHANTPUBKEY  "VmwrzZDPKpFyAddzPX4oIbqXwp9"
#define PHANTPRIVKEY "7QLYlybXMgCAaVVdLp9zsaWdRw6"

#ifdef __cplusplus
extern "C" {
#endif

	// UART config
	#define SERIAL_BAUD_RATE 115200

	// ESP SDK config
	#define LWIP_OPEN_SRC
	#define USE_US_TIMER

	// Default types
	#define __CORRECT_ISO_CPP_STDLIB_H_PROTO
	#include <limits.h>
	#include <stdint.h>

	// Override c_types.h include and remove buggy espconn
	#define _C_TYPES_H_
	#define _NO_ESPCON_

	// Updated, compatible version of c_types.h
	// Just removed types declared in <stdint.h>
	#include <espinc/c_types_compatible.h>

	// System API declarations
	#include <esp_systemapi.h>

	// C++ Support
	#include <esp_cplusplus.h>
	// Extended string conversion for compatibility
	#include <stringconversion.h>
	// Network base API
	#include <espinc/lwip_includes.h>

	// Beta boards
	#define BOARD_ESP01

#ifdef __cplusplus
}
#endif

#endif
