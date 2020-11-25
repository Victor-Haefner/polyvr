#ifndef VRCARTESIANKINEMATICS_H_INCLUDED
#define VRCARTESIANKINEMATICS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "../VREngineeringFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRCartesianKinematics : public std::enable_shared_from_this<VRCartesianKinematics> {
	private:
	public:
		VRCartesianKinematics();
		~VRCartesianKinematics();

		static VRCartesianKinematicsPtr create();
		VRCartesianKinematicsPtr ptr();
};

OSG_END_NAMESPACE;

#endif VRCARTESIANKINEMATICS_H_INCLUDED
