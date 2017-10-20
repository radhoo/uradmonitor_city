/**
 *	File:       	timecounter.cpp
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

#include "timecounter.h"
#include <avr/interrupt.h>
#include <avr/io.h>

// init function used to set a callback to be called every new minute
void  TimeCounter::init(TimeCallback callSec, TimeCallback callMin) {
	ptrFuncSec = callSec;
	ptrFuncMin = callMin;
	
	TIMSK0 |= (1 << TOIE0); //atmega168
	sei(); // enable interrupts
	TCCR0B |= _BV(CS01) | _BV(CS00); // set prescaler to 64 and start the timer

}


uint8_t TimeCounter::getSec() { return total_sec % 60; }
uint8_t TimeCounter::getMin() { return (total_sec / 60) % 60; }
uint8_t TimeCounter::getHour() { return (total_sec / 3600) % 24; }


void TimeCounter::startCount(uint8_t index) {
	if (index > MAX_COUNTERS-1) index = 0;
	counter[index] = total_sec;
}
uint32_t TimeCounter::getCount(uint8_t index) {
	if (index > MAX_COUNTERS-1) index = 0;
	if (counter[index] < total_sec)
		return total_sec - counter[index];
	else
		return 0;
}
