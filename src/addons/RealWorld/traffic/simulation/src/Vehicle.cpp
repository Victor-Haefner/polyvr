#include "Vehicle.h"

#include <boost/lexical_cast.hpp>
using boost::lexical_cast;

Vehicle::Vehicle(const ID id, const Vec3f& position, const Quaternion& orientation)
    : id(id), driverType(404), vehicleType(404), controller(-1), position(position),
      futurePosition(position + Vec3f(1, 0, 0)), orientation(orientation), currentSpeed(0),
      desiredSpeed(0), currentRotation(), route(), streetId(0), lane(0), currentDestination(),
      state(0), blockedForTicks(0), waitingFor() {

}

Vehicle::Vehicle(const ID id, const Vec3f& position, const Quaternion& orientation, const ID vehicleType, const ID driverType)
    : id(id), driverType(driverType), vehicleType(vehicleType), controller(0), position(position),
      futurePosition(position + Vec3f(1, 0, 0)), orientation(orientation), currentSpeed(0),
      desiredSpeed(0), currentRotation(), route(), streetId(0), lane(0), currentDestination(),
      state(0), blockedForTicks(0), waitingFor() {

}

ID Vehicle::getId() const {
    return id;
}

void Vehicle::setVehicleType(const ID type) {
    vehicleType = type;
}

ID Vehicle::getVehicleType() const {
    return vehicleType;
}

void Vehicle::setDriverType(const ID type) {
    driverType = type;
}

ID Vehicle::getDriverType() const {
    return driverType;
}


void Vehicle::setController(int value) {
    controller = value;
}

int Vehicle::getController() const {
    return controller;
}

void Vehicle::setPosition(const Vec3f& pos) {
    position = pos;
}

const Vec3f& Vehicle::getPosition() const {
    return position;
}

void Vehicle::setFuturePosition(const Vec3f& futurePos) {
    futurePosition = futurePos;
}

const Vec3f& Vehicle::getFuturePosition() const {
    return futurePosition;
}

void Vehicle::setOrientation(const Quaternion& orientation) {
    this->orientation = orientation;
}

const Quaternion& Vehicle::getOrientation() const {
    return orientation;
}

void Vehicle::setCurrentSpeed(const double speed) {
    currentSpeed = speed;
}

double Vehicle::getCurrentSpeed() const {
    return currentSpeed;
    // I am on speed!!!
    // Sorry. ;-)
}

void Vehicle::setDesiredSpeed(double dspeed) {
    desiredSpeed = dspeed;
}

double Vehicle::getDesiredSpeed() const {
    return desiredSpeed;
}

void Vehicle::setCurrentRotation(const Quaternion& rotation) {
    currentRotation = rotation;
}

const Quaternion& Vehicle::getCurrentRotation() const {
    return currentRotation;
}

deque<ID>* Vehicle::getRoute() {
    return &route;
}

void Vehicle::setStreet(ID street, int lane) {
    this->streetId = street;
    this->lane = lane;
}

ID Vehicle::getStreetId() const {
    return streetId;
}

int Vehicle::getLaneNumber() const {
    return lane;
}

void Vehicle::setCurrentDestination(const Vec2f& dest) {
    // Add a small random offset
    Vec2f offset((rand() % 10) / 10, (rand() % 10) / 10);
    currentDestination = dest + offset;
}

Vec2f Vehicle::getCurrentDestination() const {
    return currentDestination;
}

void Vehicle::setState(const STATE state) {
    this->state = state;
}

Vehicle::STATE Vehicle::getState() const {
    return state;
}

void Vehicle::setBlockedForTicks(int ticks) {
    blockedForTicks = ticks;
}

int Vehicle::getBlockedForTicks() const {
    return blockedForTicks;
}

set<ID>& Vehicle::getWaitingFor() {
    return waitingFor;
}

string Vehicle::toString(const bool extendedOutput) const {

    string str = string("Vehicle #") + lexical_cast<string>(id) + ((extendedOutput)?"\n  ":" [")
        + "driverType=" + lexical_cast<string>(driverType) + ((extendedOutput)?"\n  ":"; ")
        + "vehicleType=" + lexical_cast<string>(vehicleType) + ((extendedOutput)?"\n  ":"; ")
        + "controller=" + lexical_cast<string>(controller) + ((extendedOutput)?"\n  ":"; ")
        + "position=" + lexical_cast<string>(position[0]) + " / "  + lexical_cast<string>(position[1]) + " / " + lexical_cast<string>(position[2]) + ((extendedOutput)?"\n  ":"; ")
        + "fPosition=" + lexical_cast<string>(futurePosition[0]) + " / "  + lexical_cast<string>(futurePosition[1]) + " / " + lexical_cast<string>(futurePosition[2]) + ((extendedOutput)?"\n  ":"; ")
        + "currentSpeed=" + lexical_cast<string>(currentSpeed) + ((extendedOutput)?"\n  ":"; ")
        + "desiredSpeed=" + lexical_cast<string>(desiredSpeed) + ((extendedOutput)?"\n  ":"; ")
        + "orientation=" + lexical_cast<string>(orientation[0]) + " / "  + lexical_cast<string>(orientation[1]) + " / " + lexical_cast<string>(orientation[2]) + " / "  + lexical_cast<string>(orientation[3]) + ((extendedOutput)?"\n  ":"; ")
        + "currentRotation=" + lexical_cast<string>(currentRotation[0]) + " / "  + lexical_cast<string>(currentRotation[1]) + " / " + lexical_cast<string>(currentRotation[2]) + " / "  + lexical_cast<string>(currentRotation[3]) + ((extendedOutput)?"\n  ":"; ")
        + "streetId=" + lexical_cast<string>(streetId) + ((extendedOutput)?"\n  ":"; ")
        + "lane=" + lexical_cast<string>(lane) + ((extendedOutput)?"\n  ":"; ")
        + "currentDestination=" + lexical_cast<string>(currentDestination[0]) + " / "  + lexical_cast<string>(currentDestination[1]) + ((extendedOutput)?"\n  ":"; ")
        + "state=";

    if (state & TURN_LEFT) str += "left|";
    if (state & TURN_RIGHT) str += "right|";
    if (state & ACCELERATING) str += "acc|";
    if (state & BRAKING) str += "brake|";
    if (state & WAITING) str += "wait|";
    if (state & BLOCKED) str += "block|";
    if (state & COLLIDING) str += "coll|";
    if (state & CRASHED) str += "crash|";

    if (str[str.length() - 1] == '|')
        str.erase(str.length() - 1);
    else
        str += "none";
    str += ((extendedOutput)?"\n  ":"; ");

    str += "waiting for:";
    for (set<ID>::const_iterator waitingIter = waitingFor.begin(); waitingIter != waitingFor.end(); ++waitingIter)
        str += " " + lexical_cast<string>(*waitingIter);

    str += ((extendedOutput)?"\n  ":"; ");

    str += "blockedForTicks=" + lexical_cast<string>(blockedForTicks) + ((extendedOutput)?"\n  ":"; ");

#ifdef WITH_GUI
        if (extendedOutput)
            str += "speedInfluences=" + speedInfluences + "\n";
#endif

    str += ((extendedOutput)?"":"]");

    return str;
}
