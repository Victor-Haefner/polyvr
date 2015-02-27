#ifndef VEHICLE_H
#define VEHICLE_H

#include <deque>
#include <string>

#include <set>

#include "DriverType.h"
#include "VehicleType.h"
#include "timer.h"

using namespace std;

/**
 A class that represents a vehicle consisting of driver && type.
 */
class Vehicle {

    public:

        /// A type to describe the state of a vehicle.
        typedef uint8_t STATE;
        /// The vehicle is turning left.
        static const STATE TURN_LEFT    = 1 << 0;
        /// The vehicle is turning right.
        static const STATE TURN_RIGHT   = 1 << 1;
        /// The vehicle is accelerating.
        static const STATE ACCELERATING = 1 << 2;
        /// The vehicle is waiting for something, e.g. for a red traffic light.
        static const STATE WAITING      = 1 << 3;
        /// The vehicles path is blocked by another vehicle.
        static const STATE BLOCKED      = 1 << 4;
        /// The vehicle is braking (note: This is active breaking as opposed to only decelerating, e.g. the stoplight is on).
        static const STATE BRAKING      = 1 << 5;
        /// The vehicle is inside the radius of an other vehicle.
        static const STATE COLLIDING    = 1 << 6;
        /// The vehicle has collided with an other vehicle.
        static const STATE CRASHED      = 1 << 7;

#ifdef WITH_GUI
        /// A string with information about the calculation of the current speed.
        string speedInfluences;

        /// A set of vehicles that are near this vehicles && are regarded for collision avoidance.
        set<pair<double, ID> > nearVehicles;
#endif // WITH_GUI

    private:

        /// The id of this vehicle.
        ID id;

        /// The id of the driver type.
        ID driverType;

        /// The id of the vehicle type.
        ID vehicleType;

        /// Whether the simulation || the client is controlling this car.
        /// Works as a timer, too:
        /// \li If positive, the value will be decreased && no extensive calculations will be done for the vehicle.
        /// \li If \c 0, the simulation will control it.
        /// \li If negativ, the client will control it.
        int controller;

        /// The current position of this car.
        Vec3f position;

        /// The presumably position of this vehicle after the next tick.
        Vec3f futurePosition;

        /// The current orientation of this car.
        Quaternion orientation;

        /// The current speed the vehicle is driving with.
        double currentSpeed;

        /// The speed the vehicle wants to drive with.
        double desiredSpeed;

        /// The current amount of rotation to apply.
        /// This can be interpreted as the steering wheel angle.
        Quaternion currentRotation;

        /// The route this vehicle is driving on.
        /// This list contains the IDs of all nodes the vehicle is driving through.
        deque<ID> route;

        /// The id of the street the vehicle is driving on.
        ID streetId;

        /// The number of the lane.
        int lane;

        /// The current destination.
        /// This position is near the next node but on the current lane.
        Vec2f currentDestination;

        /// The current state of the vehicle.
        STATE state;

        /// The number of concurrent ticks this vehicle is blocked by another vehicle.
        int blockedForTicks;

        /// A set of vehicles this vehicle is waiting for.
        set<ID> waitingFor;

    public:

        /**
         Creates a vehicle that is controlled by a client.
         @param id The id of the new vehicle.
         @param position The position of the vehicle.
         @param orientation The orientation of the vehicle.
         */
        Vehicle(const ID id, const Vec3f& position, const Quaternion& orientation);

        /**
         Creates a vehicle that is controlled by the simulator.
         @param id The id of the new vehicle.
         @param position The position of the vehicle.
         @param orientation The orientation of the vehicle.
         @param vehicleType The type of the vehicle.
         @param driverType The type of the driver.
         */
        Vehicle(const ID id, const Vec3f& position, const Quaternion& orientation, const ID vehicleType, const ID driverType);

        /**
         Returns the id of this vehicle.
         @return The id.
         */
        ID getId() const;

        /**
         Sets the vehicle type of the vehicle.
         @param type The id of the vehicle type.
         */
        void setVehicleType(const ID type);

        /**
         Returns the vehicle type.
         @return The id of the vehicle type.
         */
        ID getVehicleType() const;

        /**
         Sets the driver of the vehicle.
         @param type The id of the driver type.
         */
        void setDriverType(const ID type);

        /**
         Returns the driver type.
         @return The id of the driver type.
         */
        ID getDriverType() const;


        /**
         Whether the simulation || the client is controlling this car.
         \li Works as a timer, too: If positive, it will be decreased.
         \li If \c 0, the simulation will control it.
         \li If negativ, the client will control it.
         @param value The value to set.
         */
        void setController(int value);

        /**
         Returns the current controller.
         @return The current controller.
         @see setController()
         */
        int getController() const;

        /**
         Sets the current position of the vehicle.
         @param pos The position to set.
         */
        void setPosition(const Vec3f& pos);

        /**
         Returns the current position of the vehicle.
         @return The position of the vehicle.
         */
        const Vec3f& getPosition() const;

        /**
         Sets the presumably position of this vehicle after the next tick.
         @param futurePos The future position.
         */
        void setFuturePosition(const Vec3f& futurePos);

        /**
         Returns the presumably position of this vehicle after the next tick.
         @return The future position.
         */
        const Vec3f& getFuturePosition() const;

        /**
         Sets the current orientation of the vehicle.
         @param orientation The orientation to set.
         */
        void setOrientation(const Quaternion& orientation);

        /**
         Returns the current orientation of the vehicle.
         Applying the quaternion to the (1, 0, 0) vector describes the
         direction the vehicle is looking at.
         @return The orientation of the vehicle.
         */
        const Quaternion& getOrientation() const;

        /**
         Sets the current speed of the vehicle.
         @param speed The new speed.
         */
        void setCurrentSpeed(const double speed);

        /**
         Retrieves the current speed.
         @return The current speed.
         */
        double getCurrentSpeed() const;

        /**
         Sets the desired speed this vehicle wants to drive with.
         @param dspeed The desired speed.
         */
        void setDesiredSpeed(double dspeed);

        /**
         Gets the desired speed this vehicle wants to drive with.
         @return The desired speed.
         */
        double getDesiredSpeed() const;

        /**
         The current amount of rotation to apply.
         This can be interpreted as the steering wheel angle.
         @param rotation The rotation to apply.
         */
        void setCurrentRotation(const Quaternion& rotation);

        /**
         Returns he current amount of rotation to apply.
         @return The rotation to apply.
         */
        const Quaternion& getCurrentRotation() const;

        /**
         Returns the route this vehicle is driving on.
         This list contains the IDs of all nodes the vehicle is driving through.
         The first entry is the node the vehicle is coming from, the second entry is
         the node the vehicle is driving to at the moment.
         @warning The list might be empty. In that case, feel free to add new destinations.
         @return A pointer to the route.
         */
        deque<ID>* getRoute();

        /**
         Sets the street && the lane of the vehicle.
         @param street The id of the street.
         @param lane The number of the lane.
         */
        void setStreet(ID street, int lane);

        /**
         Returns he id of the street the vehicle is driving on.
         @return The id.
         */
        ID getStreetId() const;

        /**
         Returns the number of the lane the vehicle is on.
         @return The number of the lane.
         */
        int getLaneNumber() const;

        /**
         Sets the current destination.
         This position is near the next node but on the current lane.
         @param dest The destination this vehicle should approach.
         */
        void setCurrentDestination(const Vec2f& dest);

        /**
         Returns the current destination.
         @return The destination.
         */
        Vec2f getCurrentDestination() const;

        /**
         Sets the current state of a vehicle.
         @param state The new state of the vehicle.
         */
        void setState(const STATE state);

        /**
         Returns the current state.
         This might be used for some animations.
         @return The current state.
         */
        STATE getState() const;

        /**
         Sets the number of concurrent ticks this vehicle has been blocked for.
         @param ticks The number of ticks.
         */
        void setBlockedForTicks(int ticks);

        /**
         Gets the number of concurrent ticks this vehicle has been blocked for.
         @return The number of ticks.
         */
        int getBlockedForTicks() const;

        /**
         Returns a reference of the set this with the vehicles this vehicle is waiting for.
         @return A reference to the set.
         */
        set<ID>& getWaitingFor();

        /**
         Writes some information about this object into a string.
         @param extendedOutput If \c true, more information will be returned as a multi-line string.
         @return A string describing this object.
         */
        string toString(const bool extendedOutput = true) const;

};

#endif // VEHICLE_H
