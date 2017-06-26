/*
  time.h - low level time and date functions
*/

/*
  July 3 2011 - fixed elapsedSecsThisWeek macro (thanks Vincent Valdy for this)
              - fixed  daysTotimeData_t macro (thanks maniacbug)
*/     

#ifndef TIME_LIB_H_
#define TIME_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "stdint.h"
#include "stdbool.h"

typedef uint64_t timeData_t;

typedef enum
{
	timeNotSet, timeNeedsSync, timeSet
} timeStatus_t ;

typedef enum
{
    dowInvalid, dowSunday, dowMonday, dowTuesday, dowWednesday, dowThursday, dowFriday, dowSaturday
} timeDayOfWeek_t;

typedef enum
{
    tmSecond, tmMinute, tmHour, tmWday, tmDay,tmMonth, tmYear, tmNbrFields
} tmByteFields;	   

typedef struct
{
	uint8_t Second;
	uint8_t Minute;
	uint8_t Hour;
	uint8_t Wday;   // day of week, sunday is day 1
	uint8_t Day;
	uint8_t Month;
	uint8_t Year;   // offset from 1970;
} timeDataModel_t;

typedef enum
{
	timeZoneUTC,
	timeZoneCET,      //UTC + 1 - Poland winter
	timeZoneCEST, 	  //UTC + 2 - Poland summer
} timeZone_t;

//convenience macros to convert to and from tm years 
#define  tmYearToCalendar(Y) ((Y) + 1970)  // full four digit year 
#define  CalendarYrToTm(Y)   ((Y) - 1970)
#define  tmYearToY2k(Y)      ((Y) - 30)    // offset is from 2000
#define  y2kYearToTm(Y)      ((Y) + 30)   

typedef timeData_t(*getExternalTime)();

/*==============================================================================*/
/* Useful Constants */
#define SECS_PER_MIN  ((timeData_t)(60UL))
#define SECS_PER_HOUR ((timeData_t)(3600UL))
#define SECS_PER_DAY  ((timeData_t)(SECS_PER_HOUR * 24UL))
#define DAYS_PER_WEEK ((timeData_t)(7UL))
#define SECS_PER_WEEK ((timeData_t)(SECS_PER_DAY * DAYS_PER_WEEK))
#define SECS_PER_YEAR ((timeData_t)(SECS_PER_WEEK * 52UL))
#define SECS_YR_2000  ((timeData_t)(946684800UL)) // the time at the start of y2k
 
/* Useful Macros for getting elapsed time */
#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)  
#define numberOfMinutes(_time_) ((_time_ / SECS_PER_MIN) % SECS_PER_MIN) 
#define numberOfHours(_time_) (( _time_% SECS_PER_DAY) / SECS_PER_HOUR)
#define dayOfWeek(_time_)  ((( _time_ / SECS_PER_DAY + 4)  % DAYS_PER_WEEK)+1) // 1 = Sunday
#define elapsedDays(_time_) ( _time_ / SECS_PER_DAY)  // this is number of days since Jan 1 1970
#define elapsedSecsToday(_time_)  (_time_ % SECS_PER_DAY)   // the number of seconds since last midnight 
// The following macros are used in calculating alarms and assume the clock is set to a date later than Jan 1 1971
// Always set the correct time before settting alarms
#define previousMidnight(_time_) (( _time_ / SECS_PER_DAY) * SECS_PER_DAY)  // time at the start of the given day
#define nextMidnight(_time_) ( previousMidnight(_time_)  + SECS_PER_DAY )   // time at the end of the given day 
#define elapsedSecsThisWeek(_time_)  (elapsedSecsToday(_time_) +  ((dayOfWeek(_time_)-1) * SECS_PER_DAY) )   // note that week starts on day 1
#define previousSunday(_time_)  (_time_ - elapsedSecsThisWeek(_time_))      // time at the start of the week for the given time
#define nextSunday(_time_) ( previousSunday(_time_)+SECS_PER_WEEK)          // time at the end of the week for the given time


/* Useful Macros for converting elapsed time to a timeData_t */
#define minutesTotimeData_t ((M)) ( (M) * SECS_PER_MIN)
#define hoursTotimeData_t   ((H)) ( (H) * SECS_PER_HOUR)
#define daysTotimeData_t    ((D)) ( (D) * SECS_PER_DAY) // fixed on Jul 22 2011
#define weeksTotimeData_t   ((W)) ( (W) * SECS_PER_WEEK)

/*============================================================================*/
/*  time and date functions   */
int     hourNow();             // the hour now
int     hour(timeData_t t);    // the hour for the given time
int     hourFormat12();        // the hour now in 12 hour format
int     hourFormat12(timeData_t t); // the hour for the given time in 12 hour format
uint8_t isAMNow();             // returns true if time now is AM
uint8_t isAM(timeData_t t);    // returns true the given time is AM
uint8_t isPMNow();             // returns true if time now is PM
uint8_t isPM(timeData_t t);    // returns true the given time is PM
int     minuteNow();           // the minute now
int     minute(timeData_t t);  // the minute for the given time
int     secondNow();           // the second now
int     second(timeData_t t);  // the second for the given time
int     dayNow();              // the day now
int     day(timeData_t t);     // the day for the given time
int     weekdayNow();          // the weekday now (Sunday is day 1)
int     weekday(timeData_t t); // the weekday for the given time
int     monthNow();            // the month now  (Jan is month 1)
int     month(timeData_t t);   // the month for the given time
int     yearNow();             // the full four digit year: (2009, 2010 etc)
int     year(timeData_t t); // the year for the given time


void    setTime(int hr,int min,int sec,int dy, int mnth, int yr);
void    setTimeNow(timeData_t t);

/* low level functions to convert to and from system time                     */
void breakTime(timeData_t time, timeDataModel_t* tm);  // break timeData_t into elements
timeData_t makeTime(timeDataModel_t* tm);  // convert time elements into timeData_t

/* date strings */ 
#define dt_MAX_STRING_LEN 9 // length of longest date string (excluding terminating null)
char* monthStr(uint8_t month);
char* dayStr(uint8_t day);
char* monthShortStr(uint8_t month);
char* dayShortStr(uint8_t day);


/* public methods */
timeStatus_t timeInit(getExternalTime getTimeFunction);
void timeSetTimeZone(timeZone_t zone);
bool timeIsTimeChanged();
timeStatus_t timeStatus(); 		   // indicates if time has been set and recently synchronized
timeData_t timeNow();              // return the current time as seconds since Jan 1 1970
timeDataModel_t timeCurrentData(); // return whole time data;
/* time sync functions	*/
void timeSetSyncProvider(getExternalTime getTimeFunction); // identify the external time provider
void timeSetSyncInterval(timeData_t interval); // set the number of seconds between re-sync

#ifdef __cplusplus
}
#endif

#endif /* TIME_LIB_H_ */

