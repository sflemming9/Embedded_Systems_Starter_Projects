#pragma once

/*
    File:       nmea.h
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
#include <stdbool.h>


typedef struct {
	bool flagRead;
	bool flagDataReady;
	char tmpWords[20][15];
	char tmpSzChecksum[15];
	bool flagComputedChecksum;
	int checksum;
	bool flagReceivedChecksum;
	int indexReceivedChecksum;
	int wordIndex;
	int prevIndex;
	int nowIndex;
	float longitude;
	float latitude;
	unsigned char utcHour, utcMinute, utcSecond, utcDay, utcMonth, utcYear;
	int numSatellites;
	float altitude;
	float speed;
	float bearing;
} NMEA;


/*
 * The serial data is assembled on the fly, without using any redundant buffers.
 * When a sentence is complete (one that starts with $, ending in EOL), all processing is done on
 * this temporary buffer that we've built: checksum computation, extracting sentence "words" (the CSV values),
 * and so on.
 * When a new sentence is fully assembled using the fusedata function, the code calls parsedata.
 * This function in turn, splits the sentences and interprets the data. Here is part of the parser function,
 * handling both the $GPRMC NMEA sentence:
 */

void NMEAInit(NMEA *nmea);

int NMEAFuseData(NMEA *nmea, char c);

// READER functions: retrieving results, call isdataready() first
bool NMEAIsDataReady(NMEA *nmea);
int	NMEAGetHour(NMEA *nmea);
int	NMEAGetMinute(NMEA *nmea);
int	NMEAGetSecond(NMEA *nmea);
int	NMEAGetDay(NMEA *nmea);
int	NMEAGetMonth(NMEA *nmea);
int	NMEAGetYear(NMEA *nmea);
float NMEAGetLatitude(NMEA *nmea);
float NMEAGetLongitude(NMEA *nmea);
int	NMEAGetSatellites(NMEA *nmea);
float NMEAGetAltitude(NMEA *nmea);
float NMEAGetSpeed(NMEA *nmea);
float NMEAGetBearing(NMEA *nmea);
