#ifndef MICRORIMULATOR_H
#define MICRORIMULATOR_H

#include "RoadSystem.h"

/// A simulator that uses moves individual vehicles along their roads.
class Microsimulator {

    private:

        /// Some flags to inform about a change in speed.
        /// At most one of the ACTION_*s && REASON_s will occur respectively,
        /// but no flag has to be set if no real change happens.
        typedef uint8_t SPEEDCHANGE;
        /// The vehicle is accelerating.
        static const SPEEDCHANGE ACTION_ACCELERATING  = 1 << 0;
        /// The vehicle is decelerating but not braking.
        static const SPEEDCHANGE ACTION_DECELERATING  = 1 << 1;
        /// The vehicle is braking (and consequently decelerating).
        static const SPEEDCHANGE ACTION_BRAKING       = 1 << 2;
        /// The car can not drive any faster.
        static const SPEEDCHANGE REASON_VEHICLELIMIT  = 1 << 4;
        /// It is not allowed to drive faster on this street.
        static const SPEEDCHANGE REASON_STREETLIMIT   = 1 << 5;
        /// Another vehicle is too near.
        static const SPEEDCHANGE REASON_OTHERVEHICLES = 1 << 6;
        /// The destination is reached.
        static const SPEEDCHANGE REASON_DESTINATION   = 1 << 7;

        /// The number of ticks until a deadlock is forced open.
        /// One tick in this case is a half second.
        static const int ticksTillIgnoreOthers = 120;

        /// The number of ticks the vehicle will ignore others.
        /// One tick in this case is a half second.
        static const int ticksWhileIgnoreOthers = 40;

        /// A factor to transfer km/h to internal units.
        /// Stored as attribute to avoid recalculation for each vehicle
        double kmhToUnits;

        /// The milliseconds since the last calculation-tick.
        int lastTickMilliseconds;

        /// The RoadSystem to work on.
        RoadSystem *roadSystem;

        /**
         Handles the movement of one vehicle.
         @param vehicle The vehicle to move.
         */
        void handleVehicle(Vehicle *vehicle, bool moveOnly);

        /**
         Extends the route-list of a vehicle.
         Selects a new, random vertex on the map && calculates a route to it.
         @param vehicle The vehicle to create a route for.
         */
        void extendRoute(Vehicle *vehicle);

        /**
         Selects a lane number for the next street.
         Measurements like physical properties of the vehicle && other vehicles are taken into account.
         @param vehicle The vehicle that wants to change.
         @param otherStreet The street that should be entered.
         @param otherDirection The direction on the new street.
         @return A lane number on the new street which is best for the vehicle.
         */
        int selectNextLaneNumber(const Vehicle *vehicle, const Street *otherStreet, const int otherDirection) const;

        /**
         Selects the optimal lane number on the current street.
         Takes into account the desired turning at the end of the street and
         the amount of vehicles on the different lanes.
         @param vehicle The vehicle to find a lane for.
         @param street A pointer to the street the vehicle is on.
         @return A new lane number for the vehicle. Might be the same as the current one.
         */
        int findOptimalLaneNumber(Vehicle *vehicle, const Street *street) const;

        /**
         Calculates the optimal speed for a vehicle.
         Checks the distance to other vehicles on the street, the maximal possible speed of this vehicle,
         the preference of the driver, the speed-limit of the street && some random variation.
         @param vehicle The vehicle to calculate the speed for.
         @param street A pointer to the street the vehicle is on.
         @param canEnterDestination Should be \c 0 if the destination node can be entered. In that case,
            the distance to the destination will not be regarded when calculating the speed. Otherwise,
            the given value will be interpreted as the distance to keep to the node.
         @param nextStreet The next street to drive on. Is allowed to be the same as the \c street parameter.
         @param nextLane The next lane to drive on.
         @return The optimal speed for this vehicles in units per time of this frame
            && flags describing the action && its reasons.
         */
        pair<double, SPEEDCHANGE> calculateOptimalSpeed(Vehicle *vehicle) const;

        /**
         Follows the route of the given vehicle && returns the street-distance to the first node that can not be entered.
         @param vehicle The vehicle to follow the route of.
         @param minDistance Nodes inside this distance are ignored. Since the vehicle is not able to brake in time
            for a node immediately in front of it, the node can be ignored.
         @param maxDistance The maximal distance to check nodes in. If not set, all nodes in the route will be checked.
         @return The distance to the first blocking node when following the street. If no nodes blocks,
            \c numeric_limits<double>::max() will be returned.
         */
        double findFirstBlockingNode(Vehicle *vehicle, const double minDistance = 0, const double maxDistance = numeric_limits<double>::max()) const;

        /**
         Find vehicles in front of the given vehicle.
         @param vehicle The vehicle to search in front of.
         @param maxDistance The maximum distance to search in.
         @return A set containing pairs of distances && the vehicle at it.
         */
        set<pair<double, Vehicle*> > findNearVehicles(Vehicle *vehicle, const double maxDistance) const;

    public:
        /**
         Constructs an object of this class.
         @param roadSystem The RoadSystem to work on.
         */
        Microsimulator(RoadSystem *roadSystem);

        /**
         Runs one step of the simulation.
         */
        void tick();
};

#endif // MICRORIMULATOR_H
