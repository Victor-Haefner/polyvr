#ifndef VRSPACEMISSION_H_INCLUDED
#define VRSPACEMISSION_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "../VREngineeringFwd.h"

#include <map>
#include <string>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRSpaceMission : public std::enable_shared_from_this<VRSpaceMission> {
	private:
	    string name;
	    double start = 0;
	    double stop = 0;

	    map<double, string> waypoints;

	public:
		VRSpaceMission();
		~VRSpaceMission();

		static VRSpaceMissionPtr create();
		VRSpaceMissionPtr ptr();

		void setParameters(string name, double start, double stop);
		void addWayPoint(string name, double time);

        string getName();
		map<double, string> getWayPoints();
};

OSG_END_NAMESPACE;

#endif //VRSPACEMISSION_H_INCLUDED
