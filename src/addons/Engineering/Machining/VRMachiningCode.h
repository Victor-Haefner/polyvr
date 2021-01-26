#ifndef VRMACHININGCODE_H_INCLUDED
#define VRMACHININGCODE_H_INCLUDED

#include <string>
#include <vector>

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>

#include "../VREngineeringFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRMachiningCode : public std::enable_shared_from_this<VRMachiningCode> {
	public:
		struct Instruction {
			int G = -1;
			Vec3d d;
			Vec3d p0;
			double T = 0;
		};

	private:
		vector< Instruction > instructions;
		size_t pointer = 0;

		int arcPrecision = 256;
		int skippedSteps = 0;

		void translate(Vec3d vec_0, Vec3d vec_1, double v);
		void rotate(Vec3d vec_0, Vec3d vec_1, Vec3d m_0, double v_0, int mode);

	public:
		VRMachiningCode();
		~VRMachiningCode();

		static VRMachiningCodePtr create();
		VRMachiningCodePtr ptr();

		size_t length();
		void reset();
		Instruction next();

		void clear();
		void readGCode(string path, double speedMultiplier);
};

OSG_END_NAMESPACE;

#endif //VRMACHININGCODE_H_INCLUDED
