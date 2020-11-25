#ifndef VRMACHININGCODE_H_INCLUDED
#define VRMACHININGCODE_H_INCLUDED

#include <string>

#include <OpenSG/OSGConfig.h>

#include "../VREngineeringFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRMachiningCode : public std::enable_shared_from_this<VRMachiningCode> {
	private:
	public:
		VRMachiningCode();
		~VRMachiningCode();

		static VRMachiningCodePtr create();
		VRMachiningCodePtr ptr();

		void readGCode(string path);
};

OSG_END_NAMESPACE;

#endif VRMACHININGCODE_H_INCLUDED
