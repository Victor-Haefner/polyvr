#ifndef VRCARTESIANKINEMATICS_H_INCLUDED
#define VRCARTESIANKINEMATICS_H_INCLUDED

#include <memory>

using namespace std

class VRCartesianKinematics : public std::enable_shared_from_this<VRCartesianKinematics> {
	private:
	public:
		VRCartesianKinematics();
		~VRCartesianKinematics();

		VRCartesianKinematicsPtr create();
		VRCartesianKinematicsPtr ptr();
}

#endif VRCARTESIANKINEMATICS_H_INCLUDED
