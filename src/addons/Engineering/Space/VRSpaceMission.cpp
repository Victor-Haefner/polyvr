#include "VRSpaceMission.h"

using namespace OSG;

VRSpaceMission::VRSpaceMission() {}
VRSpaceMission::~VRSpaceMission() {}

VRSpaceMissionPtr VRSpaceMission::create() { return VRSpaceMissionPtr( new VRSpaceMission() ); }
VRSpaceMissionPtr VRSpaceMission::ptr() { return static_pointer_cast<VRSpaceMission>(shared_from_this()); }

void VRSpaceMission::setParameters(string n, double a, double b) { name = n; start = a, stop = b; }
void VRSpaceMission::addWayPoint(string name, double time) { waypoints[time] = name; }

string VRSpaceMission::getName() { return name; }
map<double, string> VRSpaceMission::getWayPoints() { return waypoints; }
