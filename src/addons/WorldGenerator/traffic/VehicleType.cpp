#include "VehicleType.h"

#include <boost/lexical_cast.hpp>
using boost::lexical_cast;

VehicleType* VehicleType::createVehicleType(const ID id, const double probability, const double radius, const double maxSpeed, const double maxAcc, const double maxRot) {

    VehicleType *vehicle = new VehicleType(id);

    // Try to set all parameter. They check the values internally, so just chain them together
    if (vehicle->setProbability(probability) && vehicle->setRadius(radius) && vehicle->setMaxSpeed(maxSpeed) && vehicle->setMaxAcceleration(maxAcc) && vehicle->setMaxRotation(maxRot))
        return vehicle;

    // At least one parameter is invalid, return a null pointer
    delete vehicle;
    return NULL;
}

VehicleType::VehicleType(const ID id)
    : id(id), probability(1), radius(VEHICLE_LENGTH), maxSpeed(120), maxAcc(10), maxRot(10) {
}

ID VehicleType::getId() const {
    return id;
}

bool VehicleType::setProbability(const double prob) {
    if (prob < 0)
        return false;
    probability = prob;
    return true;
}

double VehicleType::getProbability() const {
    return probability;
}

bool VehicleType::setRadius(const double radius) {
    if (radius < 0)
        return false;
    this->radius = radius;
    return true;
}

double VehicleType::getRadius() const {
    return radius;
}

bool VehicleType::setMaxSpeed(const double speed) {
    if (speed < 0)
        return false;
    maxSpeed = speed;
    return true;
}

double VehicleType::getMaxSpeed() const {
    return maxSpeed;
}

bool VehicleType::setMaxAcceleration(const double acc) {
    if (acc < 0)
        return false;
    maxAcc = acc;
    return true;
}

double VehicleType::getMaxAcceleration() const {
    return maxAcc;
}

bool VehicleType::setMaxRotation(const double rot) {
    if (rot < 0 || rot > 180)
        return false;
    maxRot = rot;
    return true;
}

double VehicleType::getMaxRotation() const {
    return maxRot;
}

string VehicleType::toString(const bool extendedOutput) const {

    string str = string("VehicleType #") + lexical_cast<string>(id) + ((extendedOutput)?"\n  ":" [")
        + "probability=" + lexical_cast<string>(probability) + ((extendedOutput)?"\n  ":"; ")
        + "radius=" + lexical_cast<string>(radius) + ((extendedOutput)?"\n  ":"; ")
        + "maxSpeed=" + lexical_cast<string>(maxSpeed) + ((extendedOutput)?"\n  ":"; ")
        + "maxAcc=" + lexical_cast<string>(maxAcc) + ((extendedOutput)?"\n  ":"; ")
        + "maxRot=" + lexical_cast<string>(maxRot) + ((extendedOutput)?"":"]");

    return str;
}
