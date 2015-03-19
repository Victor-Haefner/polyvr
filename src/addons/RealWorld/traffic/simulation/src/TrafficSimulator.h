#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "RoadSystem.h"
#include "Microsimulator.h"
#include "Mesosimulator.h"
#include "NetworkInterface.h"

/**
 Main class of the traffic simulator.
 This class is responsible for the data structures, the network interface and
 running the simulators on the data.

 To run it, create an object of this class && execute the \c mainLoop() method.
 It will block && run the simulation until the state is set to \c STOP with the
 \c setState() method.
 */
class TrafficSimulator {

    public:
        /// The possible states of the simulator.
        enum STATE {
            /// The simulator is running its mainloop.
            RUN,
            /// The simulation is paused but the mainloop still handles the network.
            PAUSE,
            /// The simulator is shutting down && the \c mainLoop() method will return shortly.
            STOP,
            /// The simulator restarts, same as deleting this object && creating a new one.
            RESTART
        };

    private:
        /// The road system to operate on.
        RoadSystem roadSystem;

        /// The simulator for micro-vehicle.
        Microsimulator microsimulator;

        /// The simulator for meso-streets.
        Mesosimulator mesosimulator;

        /// The interface to the network.
        /// Uses JSON/HTTP on port 5550.
        NetworkInterface network;

        /// The current state of the simulator.
        STATE state;

    public:
        /**
         Creates an object of this class.
         The initial state is \c RUN.
         */
        TrafficSimulator();

        /**
         Runs the main loop of the simulator.
         This includes running the simulation && controlling the interfaces.

         This call will block until \c setState(STOP) is called.
         */
        void mainLoop();

        /**
         Sets the state of the simulator.
         @param newState The new state to change to.
         */
        void setState(const STATE newState);

        /**
         Returns the state.
         @return The current state of the simulator.
         */
        STATE getState() const;

        /**
         Returns a pointer to the used road system.
         @return A pointer pointing to the road system.
         */
        RoadSystem* getRoadSystem();
};

#endif // SIMULATOR_H
