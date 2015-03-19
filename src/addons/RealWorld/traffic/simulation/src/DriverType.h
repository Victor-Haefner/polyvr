#ifndef DRIVERTYPE_H
#define DRIVERTYPE_H

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
class DriverType
{
    private:
        /// An id for the driver, used to reference the type in the output.
        const ID id;

        /// The probability of this driver. The sum of all probabilities is used as 100%.
        double probability;

        /// The probability in [0-1] that the driver will break the law, e.g. by speeding.
        double lawlessness;

        /// The probability in [0-1] for the driver to behave responsible, e.g. with a big safety margin.
        double cautiousness;

    public:

        /**
         A static method to create an object of this class.
         This method is not really needed, but allows to set all attributes in one call.
         @param id An id for the driver, used to reference the type in the output.
         @param probability The probability of this driver. The sum of all probabilities is used as 100%.
         @param lawlessness The probability in [0-1] that the driver will break the law, e.g. by speeding.
         @param cautiousness The probability in [0-1] for the driver to behave responsible, e.g. with a big safety margin.
         @return A pointer to the newly created object || \c null if a parameter is invalid.
        */
        static DriverType* createDriverType(const ID id, const double probability, const double lawlessness, const double cautiousness);

        /**
         Default constructor.
         @param id The id for the new driver.
         */
        DriverType(const ID id);

        /**
         Access id.
         @return The id of this driver type.
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
         Set lawlessness.
         This is the probability that the driver will break the law, e.g. by speeding.
         @param val The new value to set. Has to be in [0-1].
         @return \c True if the new value has been assigned, \c false if the new value has been invalid.
         */
        bool setLawlessness(double val);

        /**
         Access lawlessness.
         @return The current lawlessness.
         */
        double getLawlessness() const;

        /**
         Set cautiousness
         That is the probability for the driver to behave responsible, e.g. using a big safety margin.
         @param caution New value to set, has to be in [0-1].
         @return \c True if the new value has been assigned, \c false if the new value has been invalid.
         */
        bool setCautiousness(double caution);

        /**
         Access cautiousness
         @return The current cautiousness of this driver.
         */
        double getCautiousness() const;

        /**
         Writes some information about this object into a string.
         @param extendedOutput If \c true, more information will be returned as a multi-line string.
         @return A string describing this object.
         */
        string toString(const bool extendedOutput = true) const;
};

#endif // DRIVERTYPE_H
