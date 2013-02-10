/*

THIS IS A SUBSET OF THE FULL LIBTAI. CHANGES HAVE BEEN MADE TO SUIT
LIBKUMU STYLE AND TYPE CONVENTIONS. ALL BUGS BELONG TO JOHN HURST.
THE FOLLOWING IS FOR ATTRIBUTION, THANK YOU MR. BERNSTEIN FOR WRITING
AND DISTRIBUTING SUCH GREAT SOFTWARE:

libtai 0.60, alpha.
19981013
Copyright 1998
D. J. Bernstein, djb@pobox.com
http://pobox.com/~djb/libtai.html


libtai is a library for storing and manipulating dates and times.

libtai supports two time scales: (1) TAI64, covering a few hundred
billion years with 1-second precision; (2) TAI64NA, covering the same
period with 1-attosecond precision. Both scales are defined in terms of
TAI, the current international real time standard.

libtai provides an internal format for TAI64, struct tai, designed for
fast time manipulations. The tai_pack() and tai_unpack() routines
convert between struct tai and a portable 8-byte TAI64 storage format.
libtai provides similar internal and external formats for TAI64NA.

libtai provides struct caldate to store dates in year-month-day form. It
can convert struct caldate, under the Gregorian calendar, to a modified
Julian day number for easy date arithmetic.

libtai provides struct caltime to store calendar dates and times along
with UTC offsets. It can convert from struct tai to struct caltime in
UTC, accounting for leap seconds, for accurate date and time display. It
can also convert back from struct caltime to struct tai for user input.
Its overall UTC-to-TAI conversion speed is 100x better than the usual
UNIX mktime() implementation.

This version of libtai requires a UNIX system with gettimeofday(). It
will be easy to port to other operating systems with compilers
supporting 64-bit arithmetic.

The libtai source code is in the public domain.

*/

  /*! \file    KM_tai.cpp
    \version $Id: KM_tai.cpp,v 1.5 2012/03/07 17:30:52 mikey Exp $
    \brief   portable time functions
  */

#include <KM_tai.h>
#ifdef KM_WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif

//
void
caldate_frommjd(Kumu::TAI::caldate* cd, i32_t day)
{
  assert(cd);
  i32_t year, month, yday;

  year = day / 146097L;
  day %= 146097L;
  day += 678881L;
  while (day >= 146097L) { day -= 146097L; ++year; }

  /* year * 146097 + day - 678881 is MJD; 0 <= day < 146097 */
  /* 2000-03-01, MJD 51604, is year 5, day 0 */

  year *= 4;
  if (day == 146096L) { year += 3; day = 36524L; }
  else { year += day / 36524L; day %= 36524L; }
  year *= 25;
  year += day / 1461;
  day %= 1461;
  year *= 4;

  yday = (day < 306);
  if (day == 1460) { year += 3; day = 365; }
  else { year += day / 365; day %= 365; }
  yday += day;

  day *= 10;
  month = (day + 5) / 306;
  day = (day + 5) % 306;
  day /= 10;
  if (month >= 10) { yday -= 306; ++year; month -= 10; }
  else { yday += 59; month += 2; }

  cd->year = year;
  cd->month = month + 1;
  cd->day = day + 1;
}

//
static ui32_t times365[4] = { 0, 365, 730, 1095 } ;
static ui32_t times36524[4] = { 0, 36524UL, 73048UL, 109572UL } ;
static ui32_t montab[12] =
{ 0, 31, 61, 92, 122, 153, 184, 214, 245, 275, 306, 337 } ;
/* month length after february is (306 * m + 5) / 10 */

//
i32_t
caldate_mjd(const Kumu::TAI::caldate* cd)
{
  assert(cd);
  i32_t y, m, d;

  d = cd->day - 678882L;
  m = cd->month - 1;
  y = cd->year;

  d += 146097L * (y / 400);
  y %= 400;

  if (m >= 2) m -= 2; else { m += 10; --y; }

  y += (m / 12);
  m %= 12;
  if (m < 0) { m += 12; --y; }

  d += montab[m];

  d += 146097L * (y / 400);
  y %= 400;
  if (y < 0) { y += 400; d -= 146097L; }

  d += times365[y & 3];
  y >>= 2;

  d += 1461L * (y % 25);
  y /= 25;

  d += times36524[y & 3];

  return d;
}


//
void
caltime_utc(Kumu::TAI::caltime* ct, const Kumu::TAI::tai* t)
{
  assert(ct&&t);
  Kumu::TAI::tai t2 = *t;
  ui64_t u = t2.x + 58486;
  i32_t s = (i32_t)(u % ui64_C(86400));

  ct->second = (s % 60); s /= 60;
  ct->minute = s % 60; s /= 60;
  ct->hour = s;

  u /= ui64_C(86400);
  caldate_frommjd(&ct->date,/*XXX*/(i32_t) (u - ui64_C(53375995543064)));

  ct->offset = 0;
}

//
void
caltime_tai(const Kumu::TAI::caltime* ct, Kumu::TAI::tai* t)
{
  assert(ct&&t);
  i32_t day, s;

  /* XXX: check for overflow? */

  day = caldate_mjd(&ct->date);

  s = ct->hour * 60 + ct->minute;
  s = (s - ct->offset) * 60 + ct->second;

  t->x = day * ui64_C(86400) + ui64_C(4611686014920671114) + (i64_t)s;
}

//
void
Kumu::TAI::tai::now()
{
#ifdef KM_WIN32
  SYSTEMTIME st;
  ::GetSystemTime(&st);
  TAI::caltime ct;
  ct.date.year = st.wYear;
  ct.date.month = st.wMonth;
  ct.date.day = st.wDay;
  ct.hour = st.wHour;
  ct.minute = st.wMinute;
  ct.second = st.wSecond;
  caltime_tai(&ct, this);
#else
  struct timeval now;
  gettimeofday(&now, 0);
  x = ui64_C(4611686018427387914) + (ui64_t)now.tv_sec;
#endif
}


//
const Kumu::TAI::tai&
Kumu::TAI::tai::operator=(const Kumu::TAI::caltime& rhs)
{
  caltime_tai(&rhs, this);
  return *this;
}

//
const Kumu::TAI::caltime&
Kumu::TAI::caltime::operator=(const Kumu::TAI::tai& rhs)
{
  caltime_utc(this, &rhs);
  return *this;
}


//
// end KM_tai.cpp
//
