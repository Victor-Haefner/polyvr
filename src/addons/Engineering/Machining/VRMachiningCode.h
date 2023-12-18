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
            vector<string> args;
            Function() {}
            Function(string n) : name(n) {}
		};

		struct Program {
		    Function main = Function("main");
            map<string, Function> subroutines;
		};

        struct Context {
            // state variables
            int motionMode = -1; // G0, G1, G2, G3
            float speed = 50;
            float wait = 0;
            Vec3d rotationAxis = Vec3d(0,-1,0);
            Vec3d cursor;
            Vec3d target;
            Vec3d rotationCenter;
            Vec3d vec0, vec1;
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
		void parseFile(string path, bool onlySubroutines = false);
		void parseFolder(string path);

		void parseCommands(string line, Context& ctx, double speedMultiplier);
		void processFlow(function<void(string, Context&)> cb);
		void computePaths(double speedMultiplier);

		VRGeometryPtr asGeometry();
};

OSG_END_NAMESPACE;

#endif //VRMACHININGCODE_H_INCLUDED
