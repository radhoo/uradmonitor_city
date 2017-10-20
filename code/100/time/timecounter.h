/**
 *	File:       	timecounter.h
 *	Version:  		1.0
 *	Date:       	2013
 *	License:		GPL v3
 *	Description:	time handling subsystem for AVR microcontrollers - implement a time counter
 *	Project:		uRADMonitor KIT1, a hackable digital sensor monitoring tool with network interface
 *  
 *	Copyright 2013 by Radu Motisan, radu.motisan@gmail.com
 *	Copyright 2016 by Magnasci SRL, www.magnasci.com
 *  
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 * 	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 * 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * 	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#pragma once
#include <stdint.h>

typedef void (*TimeCallback)();

#define MAX_COUNTERS 3

class TimeCounter {
	uint16_t		milis;		// time: counts the intervals of 2.048ms (on 8MHz F_CPU) elapsed
					
	uint32_t 		total_sec,
					total_min,
					counter[MAX_COUNTERS];
	TimeCallback	ptrFuncSec,	// called when a full second has elapsed
					ptrFuncMin;	// a full minute has elapsed
	public:
	enum {
			TIME_COUNTER_1,
			TIME_COUNTER_2,
			TIME_COUNTER_3
		};

	TimeCounter() {
		ptrFuncSec = 0; ptrFuncMin = 0;
	}
	void init(TimeCallback callSec, TimeCallback callMin);
	inline void TimerEvent();
	inline uint32_t getTotalSec() { return total_sec; };
	inline uint32_t getTotalMin() { return total_min; };
	uint8_t getSec();
	uint8_t getMin();
	uint8_t getHour();

	void startCount(uint8_t index);
	uint32_t getCount(uint8_t index);
};

// event to be executed every  2.048ms here with 8MHz crystal
// event to be executed every  1.024ms here with 16MHz crystal
void TimeCounter::TimerEvent() {
	milis ++;
	// small timing error here: 488.28125 exact value for 8MHz
	if (milis < 488) return; // 8MHZ
	//if (milis < 976) return; // 16MHZ

	total_sec++;

	if ((total_sec>0) && (total_sec % 60 == 0)) {
		total_min++;
		if (ptrFuncMin != 0) ptrFuncMin();
	}
	if (ptrFuncSec != 0) ptrFuncSec();

	// reset counter
	milis = 0; // 488 x 2.048ms ~= 1000 ms = 1sec for 8MHz , 976 x 1.024ms ~= 1000 ms = 1 sec for 16MHz
}
