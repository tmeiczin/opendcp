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

  /*! \file    KM_tai.h
    \version $Id: KM_tai.h,v 1.5 2012/02/21 02:09:30 jhurst Exp $
    \brief   portable time functions
  */

#ifndef _KUMU_TAI_H_
#define _KUMU_TAI_H_

#include <KM_platform.h>

//
namespace Kumu
{
  namespace TAI
  {
    class caltime;

    //
    struct tai
    {
      ui64_t x;
      inline void add_seconds(i32_t s)  { x += s; }
      inline void add_minutes(i32_t m) { x += m * 60; }
      inline void add_hours(i32_t h) { x += h * 3600; }
      inline void add_days(i32_t d) { x += d * 86400; }
      void now();

      const tai& operator=(const caltime& rhs);
    };
    
    //
    struct caldate
    {
      i32_t year;
      i32_t month;
      i32_t day;
    };

    //
    class caltime
    {
    public:
      caldate date;
      i32_t hour;
      i32_t minute;
      i32_t second;
      i32_t offset;

      const caltime& operator=(const tai& rhs);
    };


  } // namespace TAI

} // namespace Kumu


#endif // _KUMU_TAI_H_

//
// end KM_tai.h
//
