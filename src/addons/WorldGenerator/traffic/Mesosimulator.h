#ifndef MESORIMULATOR_H
#define MESORIMULATOR_H

#include "RoadSystem.h"

/// A simulator that uses abstract vehicles to represent traffic flow.
class Mesosimulator {

    private:

        /// The RoadSystem to work on.
        RoadSystem *roadSystem;

        /**
         Checks for vehicles at the end of the street && moves them to another street.
         The given street has to be a meso street.
         @param street A pointer to the meso street to handle.
         */
        void handleStreet(Street *street);

        /**
         Moves a meso-vehicle to an adjacent street.
         @param node The node the vehicle is at.
         @param street The street the vehicle is on.
         @param direction The direction the vehicle has been driving into.
         @return \c True if the vehicle has been moved, \c false if it could not be moved at the moment.
         */
        bool moveVehicle(const ID node, Street *street, const int direction);

    public:
        /**
         Constructs an object of this class.
         @param roadSystem The RoadSystem to work on.
         */
        Mesosimulator(RoadSystem *roadSystem);

        /**
         Runs one step of the simulation.
         */
        void tick();
};

#endif // MESORIMULATOR_H
