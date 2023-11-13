#ifndef VRMACHININGCODE_H_INCLUDED
#define VRMACHININGCODE_H_INCLUDED

#include <string>
#include <vector>

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>

#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRGeometry.h"


#include "../VREngineeringFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRMachiningCode : public std::enable_shared_from_this<VRMachiningCode> {
	public:
		struct Function {
		    string name;
            vector<string> lines;
            Function() {}
            Function(string n) : name(n) {}
		};

		struct Program {
		    Function main = Function("main");
            map<string, Function> subroutines;
            map<string, string> variables;
		};


		struct Instruction {
			int G = -1;
			Vec3d d;
			Vec3d p0;
			double T = 0;
		};

		struct Process {
            vector< Instruction > instructions;
            size_t pointer = 0;
		};

	private:
        Program program;
	    Process process;

		int arcPrecision = 64;
		int skippedSteps = 0;

		void translate(Vec3d vec_0, Vec3d vec_1, double v);
		void rotate(Vec3d start, Vec3d end, Vec3d center, Vec3d axis, double v_0, int mode);

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
		void parseFile(string path);
		void computePaths(double speedMultiplier);

		VRGeometryPtr asGeometry();
};

OSG_END_NAMESPACE;

#endif //VRMACHININGCODE_H_INCLUDED
