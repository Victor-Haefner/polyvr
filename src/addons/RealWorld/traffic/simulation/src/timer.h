#ifndef TIMER_H
#define TIMER_H

#include "boost/date_time/posix_time/posix_time.hpp"

class Timer;

/// A global variable to access the timer.
/// @bug This is a global variable so there is only one timer for all Simulator-instances.
extern Timer timer;

using namespace boost::posix_time;

/**
 A class to replace the time() function.

 Using the time() function has to drawbacks: It only offers a resolution of seconds
 && it can not be stopped. If it would be used, then the vehicles would "jump" after
 a pause to make up for the lost time.

 To avoid this behavior the timer class offers the time since the last tick && a
 "current" time that does not keep running if the timer is stopped.
 */
class Timer {

    private:

        /// The time of the last tick.
        ptime lastTick;

        /// The time that is returned by getTime().
        ptime now;

        /// The time since the last tick.
        time_duration delta;

        /// Whether the timer is running at the moment.
        bool running;

        /// How many intern seconds happen in one real second.
        double timescale;

    public:
        /**
         Default constructor.
         The clock starts in running mode.
         */
        Timer();

        /**
         Starts the timer.
         */
        void start();

        /**
         Returns whether the timer is running at the moment.
         If not running, getTime() will always return the same.
         @return \c True if the timer is running at the moment.
         */
        bool isRunning();

        /**
         Stops the timer.
         */
        void stop();

        /**
         Calculates a new \c now && \c delta time.
         If the timer is stopped, the \c now value will not be changed.
         */
        void tick();

        /**
         Sets the time scale.
         This scale is a factor how many internal seconds happen in one
         real-time second.
         @param scale The new timescale. Has to be positive.
         */
        void setTimeScale(const double scale);

        /**
         Returns the current timescale setting.
         @return The current timescale.
         */
        double getTimeScale() const;

        /**
         Returns the time since the last tick.
         @return The time since the last tick.
         */
        time_duration getDelta();

        /**
         Returns the current time.
         @return The time.
         */
        ptime getTime();

};

#endif // TIMER_H
