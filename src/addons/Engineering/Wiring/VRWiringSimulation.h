#ifndef VRWIRINGSIMULATION_H_INCLUDED
#define VRWIRINGSIMULATION_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "addons/Engineering/VREngineeringFwd.h"

#include <map>
#include <string>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRWiringSimulation : public std::enable_shared_from_this<VRWiringSimulation> {
	private:
	    VRElectricSystemPtr system;

	public:
		VRWiringSimulation(VRElectricSystemPtr s);
		~VRWiringSimulation();

		static VRWiringSimulationPtr create(VRElectricSystemPtr s);
		VRWiringSimulationPtr ptr();

	    void iterate();
};

OSG_END_NAMESPACE;

#endif //VRWIRINGSIMULATION_H_INCLUDED
