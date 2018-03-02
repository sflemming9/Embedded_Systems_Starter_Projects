/*
    File:       nmea.cpp
    Version:    0.1.0
    Date:       Feb. 23, 2013
	License:	GPL v2

	NMEA GPS content parser

    ****************************************************************************
    Copyright (C) 2013 Radu Motisan  <radu.motisan@gmail.com>

	http://www.pocketmagic.net

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
    ****************************************************************************
 */

#include "nmea.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


void NMEAInit(NMEA *nmea) {
	nmea->flagRead = false;
	nmea->flagDataReady = false;
}


// the parser, currently handling GPRMC and GPGGA, but easy to add any new sentences
static		void			parsedata(NMEA *nmea);
// aux functions
static		int				digit2dec(char hexdigit);
static		float			string2float(char* s);
static		int				mstrcmp(const char *s1, const char *s2);


/*
 * The serial data is assembled on the fly, without using any redundant buffers.
 * When a sentence is complete (one that starts with $, ending in EOL), all processing is done on
 * this temporary buffer that we've built: checksum computation, extracting sentence "words" (the CSV values),
 * and so on.
 * When a new sentence is fully assembled using the fusedata function, the code calls parsedata.
 * This function in turn, splits the sentences and interprets the data. Here is part of the parser function,
 * handling both the $GPRMC NMEA sentence:
 */
int NMEAFuseData(NMEA *nmea, char c) {

	if (c == '$') {
		nmea->flagRead = true;
		// init parser vars
		nmea->flagComputedChecksum = false;
		nmea->checksum = 0;
		// after getting  * we start cuttings the received nmea->checksum
		nmea->flagReceivedChecksum = false;
		nmea->indexReceivedChecksum = 0;
		// word cutting variables
		nmea->wordIndex = 0; nmea->prevIndex = 0; nmea->nowIndex = 0;
	}

	if (nmea->flagRead) {
		// check ending
		if (c == '\r' || c== '\n') {
			// catch last ending item too
			nmea->tmpWords[nmea->wordIndex][nmea->nowIndex - nmea->prevIndex] = 0;
			nmea->wordIndex++;
			// cut received nmea->checksum
			nmea->tmpSzChecksum[nmea->indexReceivedChecksum] = 0;
			// sentence complete, read done
			nmea->flagRead = false;
			// parse
			parsedata(nmea);
		} else {
			// computed nmea->checksum logic: count all chars between $ and * exclusively
			if (nmea->flagComputedChecksum && c == '*') nmea->flagComputedChecksum = false;
			if (nmea->flagComputedChecksum) nmea->checksum ^= c;
			if (c == '$') nmea->flagComputedChecksum = true;
			// received nmea->checksum
			if (nmea->flagReceivedChecksum)  {
				nmea->tmpSzChecksum[nmea->indexReceivedChecksum] = c;
				nmea->indexReceivedChecksum++;
			}
			if (c == '*') nmea->flagReceivedChecksum = true;
			// build a word
			nmea->tmpWords[nmea->wordIndex][nmea->nowIndex - nmea->prevIndex] = c;
			if (c == ',') {
				nmea->tmpWords[nmea->wordIndex][nmea->nowIndex - nmea->prevIndex] = 0;
				nmea->wordIndex++;
				nmea->prevIndex = nmea->nowIndex;
			}
			else nmea->nowIndex++;
		}
	}
	return nmea->wordIndex;
}


/*
 * parse internal tmp_ structures, fused by pushdata, and set the data flag when done
 */
static void parsedata(NMEA *nmea) {
	int received_cks = 16*digit2dec(nmea->tmpSzChecksum[0]) + digit2dec(nmea->tmpSzChecksum[1]);
	// check checksum, and return if invalid!
	if (nmea->checksum != received_cks) {
		return;
	}
	/* $GPGGA
	 * $GPGGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh
	 * ex: $GPGGA,230600.501,4543.8895,N,02112.7238,E,1,03,3.3,96.7,M,39.0,M,,0000*6A,
	 *
	 * WORDS:
	 *  1    = UTC of Position
	 *  2    = Latitude
	 *  3    = N or S
	 *  4    = Longitude
	 *  5    = E or W
	 *  6    = GPS quality indicator (0=invalid; 1=GPS fix; 2=Diff. GPS fix)
	 *  7    = Number of satellites in use [not those in view]
	 *  8    = Horizontal dilution of position
	 *  9    = Antenna altitude above/below mean sea level (geoid)
	 *  10   = Meters  (Antenna height unit)
	 *  11   = Geoidal separation (Diff. between WGS-84 earth ellipsoid and mean sea level.
	 *      -geoid is below WGS-84 ellipsoid)
	 *  12   = Meters  (Units of geoidal separation)
	 *  13   = Age in seconds since last update from diff. reference station
	 *  14   = Diff. reference station ID#
	 *  15   = Checksum
	 */
	if (mstrcmp(nmea->tmpWords[0], "$GPGGA") == 0 ||
            mstrcmp(nmea->tmpWords[0], "$GNGGA") == 0) {
		// Check GPS Fix: 0=no fix, 1=GPS fix, 2=Dif. GPS fix
		if (nmea->tmpWords[6][0] == '0') {
			// clear data
			nmea->latitude = 0;
			nmea->longitude = 0;
			nmea->flagDataReady = false;
			return;
		}
		// parse time
		nmea->utcHour = digit2dec(nmea->tmpWords[1][0]) * 10 + digit2dec(nmea->tmpWords[1][1]);
		nmea->utcMinute = digit2dec(nmea->tmpWords[1][2]) * 10 + digit2dec(nmea->tmpWords[1][3]);
		nmea->utcSecond = digit2dec(nmea->tmpWords[1][4]) * 10 + digit2dec(nmea->tmpWords[1][5]);
		// parse latitude and longitude in NMEA format
		nmea->latitude = string2float(nmea->tmpWords[2]);
		nmea->longitude = string2float(nmea->tmpWords[4]);
		// get decimal format
		if (nmea->tmpWords[3][0] == 'S') nmea->latitude  *= -1.0;
		if (nmea->tmpWords[5][0] == 'W') nmea->longitude *= -1.0;
		float degrees = trunc(nmea->latitude / 100.0f);
		float minutes = nmea->latitude - (degrees * 100.0f);
		nmea->latitude = degrees + minutes / 60.0f;
		degrees = trunc(nmea->longitude / 100.0f);
		minutes = nmea->longitude - (degrees * 100.0f);
		nmea->longitude = degrees + minutes / 60.0f;

		// parse number of satellites
		nmea->numSatellites = (int)string2float(nmea->tmpWords[7]);

		// parse altitude
		nmea->altitude = string2float(nmea->tmpWords[9]);

		// data ready
		nmea->flagDataReady = true;
	}

	/* $GPRMC
	 * note: a siRF chipset will not support magnetic headers.
	 * $GPRMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,ddmmyy,x.x,a*hh
	 * ex: $GPRMC,230558.501,A,4543.8901,N,02112.7219,E,1.50,181.47,230213,,,A*66,
	 *
	 * WORDS:
	 *  1	 = UTC of position fix
	 *  2    = Data status (V=navigation receiver warning)
	 *  3    = Latitude of fix
	 *  4    = N or S
	 *  5    = Longitude of fix
	 *  6    = E or W
	 *  7    = Speed over ground in knots
	 *  8    = Track made good in degrees True, Bearing This indicates the direction that the device is currently moving in,
	 *       from 0 to 360, measured in “azimuth”.
	 *  9    = UT date
	 *  10   = Magnetic variation degrees (Easterly var. subtracts from true course)
	 *  11   = E or W
	 *  12   = Checksum
	 */
	if (mstrcmp(nmea->tmpWords[0], "$GPRMC") == 0 || 
          mstrcmp(nmea->tmpWords[0], "$GNRMC") == 0) {
		// Check data status: A-ok, V-invalid
		if (nmea->tmpWords[2][0] == 'V') {
			// clear data
			nmea->latitude = 0;
			nmea->longitude = 0;
			nmea->flagDataReady = false;
			return;
		}
		// parse time
		nmea->utcHour = digit2dec(nmea->tmpWords[1][0]) * 10 + digit2dec(nmea->tmpWords[1][1]);
		nmea->utcMinute = digit2dec(nmea->tmpWords[1][2]) * 10 + digit2dec(nmea->tmpWords[1][3]);
		nmea->utcSecond = digit2dec(nmea->tmpWords[1][4]) * 10 + digit2dec(nmea->tmpWords[1][5]);
		// parse latitude and longitude in NMEA format
		nmea->latitude = string2float(nmea->tmpWords[3]);
		nmea->longitude = string2float(nmea->tmpWords[5]);
		// get decimal format
		if (nmea->tmpWords[4][0] == 'S') nmea->latitude  *= -1.0;
		if (nmea->tmpWords[6][0] == 'W') nmea->longitude *= -1.0;
		float degrees = trunc(nmea->latitude / 100.0f);
		float minutes = nmea->latitude - (degrees * 100.0f);
		nmea->latitude = degrees + minutes / 60.0f;
		degrees = trunc(nmea->longitude / 100.0f);
		minutes = nmea->longitude - (degrees * 100.0f);
		nmea->longitude = degrees + minutes / 60.0f;
                
		//parse speed
		// The knot (pronounced not) is a unit of speed equal to one nautical mile (1.852 km) per hour
        nmea->speed = string2float(nmea->tmpWords[7]);
		// parse bearing
		nmea->bearing = string2float(nmea->tmpWords[8]);
		// parse UTC date
		nmea->utcDay = digit2dec(nmea->tmpWords[9][0]) * 10 + digit2dec(nmea->tmpWords[9][1]);
		nmea->utcMonth = digit2dec(nmea->tmpWords[9][2]) * 10 + digit2dec(nmea->tmpWords[9][3]);
		nmea->utcYear = digit2dec(nmea->tmpWords[9][4]) * 10 + digit2dec(nmea->tmpWords[9][5]);

		// data ready
		nmea->flagDataReady = true;
	}
}
/*
 * returns base-16 value of chars '0'-'9' and 'A'-'F';
 * does not trap invalid chars!
 */
static int digit2dec(char digit) {
	if ((int)digit >= 65)
		return (int)digit - 55;
	else
		return (int)digit - 48;
}

/* returns base-10 value of zero-terminated string
 * that contains only chars '+','-','0'-'9','.';
 * does not trap invalid strings!
 */
static float string2float(char* s) {
	long  integer_part = 0;
	float decimal_part = 0.0;
	float decimal_pivot = 0.1;
	bool isdecimal = false, isnegative = false;

	char c;
	while ( ( c = *s++) )  {
		// skip special/sign chars
		if (c == '-') { isnegative = true; continue; }
		if (c == '+') continue;
		if (c == '.') { isdecimal = true; continue; }

		if (!isdecimal) {
			integer_part = (10 * integer_part) + (c - 48);
		}
		else {
			decimal_part += decimal_pivot * (float)(c - 48);
			decimal_pivot /= 10.0;
		}
	}
	// add integer part
	decimal_part += (float)integer_part;

	// check negative
	if (isnegative)  decimal_part = - decimal_part;

	return decimal_part;
}

static int mstrcmp(const char *s1, const char *s2)
{
	while((*s1 && *s2) && (*s1 == *s2))
	s1++,s2++;
	return *s1 - *s2;
}

bool NMEAIsDataReady(NMEA *nmea) {
	return nmea->flagDataReady;
}

int NMEAGetHour(NMEA *nmea) {
	return nmea->utcHour;
}

int NMEAGetMinute(NMEA *nmea) {
	return nmea->utcMinute;
}

int NMEAGetSecond(NMEA *nmea) {
	return nmea->utcSecond;
}

int NMEAGetDay(NMEA *nmea) {
	return nmea->utcDay;
}

int NMEAGetMonth(NMEA *nmea) {
	return nmea->utcMonth;
}

int NMEAGetYear(NMEA *nmea) {
	return nmea->utcYear;
}

float NMEAGetLatitude(NMEA *nmea) {
	return nmea->latitude;
}

float NMEAGetLongitude(NMEA *nmea) {
	return nmea->longitude;
}

int NMEAGetSatellites(NMEA *nmea) {
	return nmea->numSatellites;
}

float  NMEAGetAltitude(NMEA *nmea) {
	return nmea->altitude;
}

float NMEAGetSpeed(NMEA *nmea) {
	return nmea->speed;
}

float NMEAGetBearing(NMEA *nmea) {
	return nmea->bearing;
}
