#ifndef VEHICLETYPE_H
#define VEHICLETYPE_H

#include <string>

using namespace std;

#include "types.h"

/**
 A class that represents a driver type.
 The attributes of this class can be set to model a certain kind of driver,
 for example a quite aggressive driver who is always speeding tailgating others.
 @note Creating an object of this class manually is quite useless, you have to create it
       through the RoadSystem to have it registered.
 */
class VehicleType
{
    private:
        /// An id for the vehicle, used to reference the type in the output.
        const ID id;

        /// The probability of this vehicle. The sum of all probabilities is used as 100%.
        double probability;

        /// The radius of the vehicle.
        double radius;

        /// The maximal speed in km/h the vehicle can drive with.
        double maxSpeed;

        /// The maximal acceleration of the vehicle in m/s^2.
        double maxAcc;

        /// The maximum possible rotation per second in degree.
        double maxRot;


    public:

        /**
         A static method to create an object of this class.
         This method is not really needed, but allows to set all attributes in one call.
         @param id An id for the vehicle, used to reference the type in the output.
         @param probability The probability of this vehicle. The sum of all probabilities is used as 100%.
         @param radius The radius of the vehicle. If two vehicles are nearer to each other than their.
                combined radii, a collision-warning is sent to the client.
         @param maxSpeed The maximal speed in km/h the vehicle can drive with.
         @param maxAcc The maximal acceleration / deceleration of the vehicle in m/s^2.
         @param maxRot The maximum possible rotation per second in degree. This determines how good
                the vehicle can drive around corners.
         @return A pointer to the newly created object || \c null if a parameter is invalid.
        */
        static VehicleType* createVehicleType(const ID id, const double probability, const double radius, const double maxSpeed, const double maxAcc, const double maxRot);

        /**
         Default constructor.
         @param id The id for the new vehicle.
         */
        VehicleType(const ID id);

        /**
         Access id.
         @return The id of this vehicle type.
         */
        ID getId() const;

        /**
         Set probability that this driver will be choosen.
         The sum of the probabilities of all registered drivers is used as 100%.
         @param prob The new probability. Has to be non-negative.
         @return \c True if the new value has been assigned, \c false if the new value has been invalid.
         */
        bool setProbability(const double prob);

        /**
         Access probability.
         @return The current probability of choosing this driver type.
         */
        double getProbability() const;

        /**
         Set the radius of the vehicle.
         If two vehicles are nearer to each other than their.
         combined radii, a collision-warning is sent to the client.
         @param radius The new radius. Has to be non-negative.
         @return \c True if the new value has been assigned, \c false if the new value has been invalid.
         */
        bool setRadius(const double radius);

        /**
         Access the radius of this vehicle.
         @return The current radius.
         */
        double getRadius() const;

        /**
         Sets the maximum speed of this vehicle.
         The maximal speed in km/h the vehicle can drive with.
         @param speed The new speed. Has to be non-negative.
         @return \c True if the new value has been assigned, \c false if the new value has been invalid.
         */
        bool setMaxSpeed(const double speed);

        /**
         Access maximum speed.
         @return The maximum speed of this vehicle Type.
         */
        double getMaxSpeed() const;

        /**
         Sets the maximal acceleration / deceleration of the vehicle in m/s^2.
         @param acc The new acceleration. Has to be non-negative.
         @return \c True if the new value has been assigned, \c false if the new value has been invalid.
         */
        bool setMaxAcceleration(const double acc);

        /**
         Access maximum acceleration.
         @return The current accelerationl of this vehicle type.
         */
        double getMaxAcceleration() const;

        /**
         Set the maximum possible rotation per second in degree.
         This determines how good the vehicle can drive around corners.
         @param rot The new rotation. Has to be in [0-180].
         @return \c True if the new value has been assigned, \c false if the new value has been invalid.
         */
        bool setMaxRotation(const double rot);

        /**
         Access the maximum rotation.
         @return The maximum rotation.
         */
        double getMaxRotation() const;

        /**
         Writes some information about this object into a string.
         @param extendedOutput If \c true, more information will be returned as a multi-line string.
         @return A string describing this object.
         */
        string toString(const bool extendedOutput = true) const;
};

#endif // VEHICLETYPE_H
