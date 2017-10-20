#pragma once

#include "misc/models.h"

#define 			CONN_WLAN 					0x2
#define 			CONN_LORA 					0x4
/************************************************************************************************************************************************/
/* Unique ID configurator : DYNAMIC IDs                                                                                                         */
/************************************************************************************************************************************************/
#define				DEV_MODEL					MODEL_CITY
#define 			CONNECTIVITY				CONN_WLAN
#define				FORCE_DEV_NUMBER			0x9
// if 0, dynamic ID is used

#if CONNECTIVITY == CONN_LORA
#define 			DEFAULT_SEND_INTERVAL		600 				// LORA: 10 minutes
#else
#define 			DEFAULT_SEND_INTERVAL		60 					// REST: 1minute
#define				FORCE_AP_SSID			""
#define				FORCE_AP_PASSWORD		""
#endif

#define				FORCE_SEND_INTERVAL			0					// no default value, just set here if you want default!
#define 			READ_INTERVAL 				5					// read sensors every x seconds and average their values
#define				DEFAULT_WATCHDOG_INTERVAL	3660				// seconds to wait until a reboot IF the server doesn't answer
/************************************************************************************************************************************************/
/* Various constants                                                                                                         */
/************************************************************************************************************************************************/
#define 			VREF 						3.3					// 3.3 regulated Voltage ref thanks to AMS1117 3.3V
#define				VER_SW						102
#define				VER_HW						101
#define				DEV_CLASS					0x14				// CITY

#define 			WARMUP 						60					// time to wait before considering sensor data, default 60
#define				WATCHDOG_INTERVAL(x) 		(2*x + WARMUP + 120)// formula to compute the watchdog interval, where x is the sendInterval value

