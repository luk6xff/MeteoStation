/*
  time.c - low level time and date functions
  Copyright (c) Michael Margolis 2009-2014

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  
  1.0  6  Jan 2010 - initial release
  1.1  12 Feb 2010 - fixed leap year calculation error
  1.2  1  Nov 2010 - fixed setTime bug (thanks to Korman for this)
  1.3  24 Mar 2012 - many edits by Paul Stoffregen: fixed timeStatus() to update
                     status, updated examples for Arduino 1.0, fixed ARM
                     compatibility issues, added TimeArduinoDue and TimeTeensy3
                     examples, add error checking and messages to RTC examples,
                     add examples to DS1307RTC library.
  1.4  5  Sep 2014 - compatibility with Arduino 1.5.7
*/


#include "time_lib.h"

//HW Dependent
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
//HW Dependent - END


static timeDataModel_t tm;          		// a cache of time elements
static timeData_t cacheTime;   				// the time the cache was updated
static uint32_t syncIntervalSeconds = 600;  // time sync will be attempted after this many seconds, default = 600[s] = 10[min]
static uint32_t sysTime = 0;
static uint32_t nextSyncTime = 0;
static timeStatus_t Status = timeNotSet;
static getExternalTime getTimePtr;  		// pointer to external [NTP] sync function
static updateUiTime updateUiTimePtr;  		// pointer to external [NTP] sync function

static void timeTimerInit();

static void timeAdjustSystemTime(uint64_t adjustment)
{
	sysTime += adjustment;
}


void refreshCache(timeData_t t)
{
	if (t != cacheTime)
	{
		breakTime(t, &tm);
		cacheTime = t;
	}
}

int hourNow()
{
	// the hour timeNow
	return hour(timeNow());
}

int hour(timeData_t t)
{
	// the hour for the given time
	refreshCache(t);
	return tm.Hour;
}

int hourFormat12Now()
{
	// the hour timeNow in 12 hour format
	return hourFormat12(timeNow());
}

int hourFormat12(timeData_t t)
{
	// the hour for the given time in 12 hour format
	refreshCache(t);
	if (tm.Hour == 0)
	{
		return 12; // 12 midnight
	}
	else if (tm.Hour  > 12)
	{
		return tm.Hour - 12;
	}
	else
	{
		return tm.Hour;
	}
}

uint8_t isAMNow()
{
	// returns true if time timeNow is AM
	return !isPM(timeNow());
}

uint8_t isAM(timeData_t t)
{
	// returns true if given time is AM
	return !isPM(t);
}

uint8_t isPMNow()
{
	// returns true if PM
	return isPM(timeNow());
}

uint8_t isPM(timeData_t t)
{
	// returns true if PM
	return (hour(t) >= 12);
}

int minuteNow() {
	return minute(timeNow());
}

int minute(timeData_t t)
{
	// the minute for the given time
	refreshCache(t);
	return tm.Minute;
}

int secondNow()
{
	return second(timeNow());
}

int second(timeData_t t)
{
	// the second for the given time
	refreshCache(t);
	return tm.Second;
}

int dayNow()
{
	return(day(timeNow()));
}

int day(timeData_t t)
{
	// the day for the given time (0-6)
	refreshCache(t);
	return tm.Day;
}

int weekdayNow()
{
	// Sunday is day 1
	return  weekday(timeNow());
}

int weekday(timeData_t t)
{
	refreshCache(t);
	return tm.Wday;
}
   
int monthNow()
{
	return month(timeNow());
}

int month(timeData_t t)
{
	// the month for the given time
	refreshCache(t);
	return tm.Month;
}

int yearNow()
{
	// as in Processing, the full four digit year: (2009, 2010 etc)
	return year(timeNow());
}

int year(timeData_t t)
{
	// the year for the given time
	refreshCache(t);
	return tmYearToCalendar(tm.Year);
}

/*============================================================================*/	
/* functions to convert to and from system time */
/* These are for interfacing with time serivces and are not normally needed in a sketch */

// leap year calulator expects year argument as years offset from 1970
#define LEAP_YEAR(Y)     ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )

static  const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31}; // API starts months from 1, this array starts from 0
 
void breakTime(timeData_t timeInput, timeDataModel_t *tm)
{
	// break the given timeData_t into time components
	// this is a more compact version of the C library localtime function
	// note that year is offset from 1970
	uint8_t year;
	uint8_t month, monthLength;
	uint32_t time;
	unsigned long days;

	time = (uint32_t)timeInput;
	tm->Second = time % 60;
	time /= 60; // timeNow it is minutes
	tm->Minute = time % 60;
	time /= 60; // timeNow it is hours
	tm->Hour = time % 24;
	time /= 24; // timeNow it is days
	tm->Wday = ((time + 4) % 7) + 1;  // Sunday is day 1

	year = 0;
	days = 0;
	while((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= time)
	{
		year++;
	}
	tm->Year = year; // year is offset from 1970

	days -= LEAP_YEAR(year) ? 366 : 365;
	time  -= days; // timeNow it is days in this year, starting at 0

	days=0;
	month=0;
	monthLength=0;

	for (month=0; month<12; month++)
	{
		if (month==1)
		{ // february
		  if (LEAP_YEAR(year))
		  {
			  monthLength=29;
		  }
		  else
		  {
			  monthLength=28;
		  }
		}
		else
		{
			monthLength = monthDays[month];
		}

		if (time >= monthLength)
		{
			time -= monthLength;
		}
		else
		{
			break;
		}
	}
	tm->Month = month + 1;  // jan is month 1
	tm->Day = time + 1;     // day of month
}

timeData_t makeTime(timeDataModel_t *tm)
{
	// assemble time elements into timeData_t
	// note year argument is offset from 1970 (see macros in time.h to convert to other formats)
	// previous version used full four digit year (or digits since 2000),i.e. 2009 was 2009 or 9
    int i;
	uint32_t seconds;

	// seconds from 1970 till 1 jan 00:00:00 of the given year
	seconds= tm->Year*(SECS_PER_DAY * 365);
	for (i = 0; i < tm->Year; i++)
	{
		if (LEAP_YEAR(i))
		{
		seconds +=  SECS_PER_DAY;   // add extra days for leap years
		}
	}

	// add days for this year, months start from 1
	for (i = 1; i < tm->Month; i++)
	{
		if ( (i == 2) && LEAP_YEAR(tm->Year))
		{
		  seconds += SECS_PER_DAY * 29;
		}
		else
		{
			seconds += SECS_PER_DAY * monthDays[i-1];  //monthDay array starts from 0
		}
	}
	seconds+= (tm->Day-1) * SECS_PER_DAY;
	seconds+= tm->Hour * SECS_PER_HOUR;
	seconds+= tm->Minute * SECS_PER_MIN;
	seconds+= tm->Second;
	return (timeData_t)seconds;
}


/*=====================================================*/	
/* Low level system time functions  */
timeData_t timeNow()
{
	if (nextSyncTime <= sysTime)
	{
		if (getTimePtr != 0)
		{
			timeData_t t = getTimePtr();
			if (t != 0)
			{
				setTimeNow(t);
			}
			else
			{
				nextSyncTime = sysTime + syncIntervalSeconds;
				Status = (Status == timeNotSet) ?  timeNotSet : timeNeedsSync;
			}
		}
	}
	return (timeData_t)sysTime;
}

void setTimeNow(timeData_t t)
{
  sysTime = (uint32_t)t;  
  nextSyncTime = (uint32_t)t + syncIntervalSeconds;
  Status = timeSet;
} 

void setTime(int hr,int min,int sec,int dy, int mnth, int yr)
{
	// year can be given as full four digit year or two digts (2010 or 10 for 2010);
	//it is converted to years since 1970
	if( yr > 99)
	{
		yr = yr - 1970;
	}
	else
	{
		yr += 30;
	}
	tm.Year = yr;
	tm.Month = mnth;
	tm.Day = dy;
	tm.Hour = hr;
	tm.Minute = min;
	tm.Second = sec;
	setTimeNow(makeTime(&tm));
}


timeStatus_t timeInit(getExternalTime getTimeFunction, updateUiTime updateUiTimeFunction)
{
	timeSetSyncProvider(getTimeFunction);
	updateUiTimePtr = updateUiTimeFunction;
	timeTimerInit();
	return Status;
}

// indicates if time has been set and recently synchronized
timeStatus_t timeStatus()
{
	timeNow(); // required to actually update the status
	return Status;
}

void timeSetSyncProvider(getExternalTime getTimeFunction)
{
	getTimePtr = getTimeFunction;
	nextSyncTime = sysTime;
	timeNow(); // this will sync the clock
}

void timeSetsyncIntervalSeconds(timeData_t interval)
{ // set the number of seconds between re-sync
	syncIntervalSeconds = (uint32_t)interval;
	nextSyncTime = sysTime + syncIntervalSeconds;
}

timeDataModel_t timeCurrentData()
{
	return tm;
}

//Configures Timer3A as a 32-bit periodic timer
static void timeTimerInit()
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER3);

    TimerConfigure(TIMER3_BASE, TIMER_CFG_32_BIT_PER_UP);

    // Set the Timer3A load value to 1s.
    TimerLoadSet(TIMER3_BASE, TIMER_A, SysCtlClockGet() / 1); //1 [s]

    // Configure the Timer3A interrupt for timer timeout.
    TimerIntEnable(TIMER3_BASE, TIMER_TIMA_TIMEOUT);

    // Enable the Timer3A interrupt on the processor (NVIC).
    IntEnable(INT_TIMER3A);

    // Enable Timer3A.
	TimerEnable(TIMER3_BASE, TIMER_A);
}

//Timer3A interrupt handler
void TimeTimer3AIntHandler(void)
{
	static uint32_t seconds_cnt = 0;
	seconds_cnt++;
	timeAdjustSystemTime(1); //update about one second;
	if ((seconds_cnt % 60 == 0) && updateUiTimePtr)
	{
		updateUiTimePtr();
	}

	TimerIntClear(TIMER3_BASE, TIMER_TIMA_TIMEOUT);
}
