
// (c) Daniel Llorens - 2005

// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 3 of the License, or (at your option) any
// later version.

#ifndef RA_TIMER_H
#define RA_TIMER_H

/// @file timer.H
/// @brief Utility for measuring time.

#include <ctime>
#include <sys/time.h>
#include <sys/resource.h>
#include <string>
#include <iosfwd>
#include <sstream>
#include <limits>

class Timer
{
// run time.
    clock_t last_run;
    double gone_run;
// run time, otherwise.
    timeval last_usr;
    timeval last_sys;
    double gone_usr;
    double gone_sys;
// wall time.
    timeval last_wall;
    double gone_wall;

public:
// fills a string with the current time and date.
    static std::string const timeAndDate()
    {
        std::time_t t;
        time(&t);
        std::string tad(std::ctime(&t));
        return tad;
    }
// convert number of seconds into formatted string.
    static std::string stime(double const & d)
    {
        double dd = d;
        std::ostringstream os;
        if (d<1.e-3) {
            os << int(dd*1.e6) << "us ";
        } else if (d<1.) {
            os << int(dd*1.e3) << "ms ";
        } else {
            if (dd>86400.) {
                os << int(dd/86400) << "d ";
                dd = dd-int(dd/86400.)*86400.;
            }
            if (dd>3600.) {
                os << int(dd/3600.) << "h ";
                dd = dd-int(dd/3600.)*3600.;
            }
            if (d>60.) {
                os << int(dd/60.) << "m ";
                dd = dd-int(dd/60.)*60.;
            }
            os << dd << "s";
        }
        return os.str();
    }

// constructor does not need any parameters. The count must still be started.
    Timer(bool const do_start = false)
    {
        if (do_start) {
            start();
        }
    }

// reset and start the count.
    void start()
    {
        last_run = clock();
        gettimeofday(&last_wall, 0);
        {
            rusage tmp;
            getrusage(RUSAGE_SELF, &tmp);
            last_sys = tmp.ru_stime;
            last_usr = tmp.ru_utime;
        }
    }

// record the count.
    void stop()
    {
        gone_run = double(clock()-last_run)/CLOCKS_PER_SEC;
        {
            timeval t;
            gettimeofday(&t, 0);
            gone_wall = double(t.tv_sec-last_wall.tv_sec); // @CONVERSION, etc
            gone_wall += double(t.tv_usec-last_wall.tv_usec)*1e-6;
        }
        {
            rusage t;
            getrusage(RUSAGE_SELF, &t);
            gone_sys = double(t.ru_stime.tv_sec-last_sys.tv_sec); // @CONVERSION, etc.
            gone_sys += double(t.ru_stime.tv_usec-last_sys.tv_usec)*1e-6;
            gone_usr = double(t.ru_utime.tv_sec-last_usr.tv_sec);
            gone_usr += double(t.ru_utime.tv_usec-last_usr.tv_usec)*1e-6;
        }
    }

    double run() const { return gone_run; }
    double wall() const { return gone_wall; }
    double cpu() const { return gone_sys+gone_usr; }
    double sys() const { return gone_sys; }
    double usr() const { return gone_usr; }

    std::string get_string() const
    {
        return stime(wall());
    }
};

inline std::ostream&
operator<<(std::ostream& o, Timer& timer)
{
    timer.stop();
    return o << timer.get_string();
}

#endif // RA_TIMER_H
