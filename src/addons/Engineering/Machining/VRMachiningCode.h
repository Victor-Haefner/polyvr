#ifndef VRMACHININGCODE_H_INCLUDED
#define VRMACHININGCODE_H_INCLUDED

#include <memory>

using namespace std

class VRMachiningCode : public std::enable_shared_from_this<VRMachiningCode> {
	private:
	public:
		VRMachiningCode();
		~VRMachiningCode();

		VRMachiningCodePtr create();
		VRMachiningCodePtr ptr();
}

#endif VRMACHININGCODE_H_INCLUDED
