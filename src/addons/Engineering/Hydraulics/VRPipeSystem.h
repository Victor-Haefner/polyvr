#ifndef VRPIPESYSTEM_H_INCLUDED
#define VRPIPESYSTEM_H_INCLUDED

#include <map>
#include <vector>
#include <OpenSG/OSGConfig.h>
#include "../VREngineeringFwd.h"
#include "core/math/VRMathFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/objects/geometry/VRGeometry.h"
#include "addons/Semantics/VRSemanticsFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRPipeEnd {
    public:
        VRPipeSegmentWeakPtr pipe;
        int nID = -1;

        double height = 0.0;
        double offsetHeight = 0.0; // used for tank offset
        Vec3d offset; // offset relative to node center

        double flow = 0.0;
        double hydraulicHead = 0.0;
        double pressure = 1.0;

    public:
        VRPipeEnd(VRPipeSegmentPtr s, int nID, double height = 0.0);
        ~VRPipeEnd();

        static VRPipeEndPtr create(VRPipeSegmentPtr s, int nID, double height = 0.0);
};

class VRPipeSegment {
    public:
        int eID = 0;
        double radius = 0.0;
        double length = 0.0;
        double area = 0.0;
        double volume = 0.0;
        double resistance = 0.0;
        double dynamicResistance = 0.0; // set by valves and pumps
        double density = 1000.0; // kg / m3
        double viscosity = 1e-3; // Pa s
        double level = 0.0;
        double friction = 0.02;
        bool flowBlocked = false;

        VRPipeEndWeakPtr end1;
        VRPipeEndWeakPtr end2;

        double computeExchange(double hole, VRPipeSegmentPtr other, double dt, bool p1, bool op1);

    public:
        VRPipeSegment(int eID, double radius, double length, double level);
        ~VRPipeSegment();

        static VRPipeSegmentPtr create(int eID, double radius, double length, double level);
        VRPipeEndPtr otherEnd(VRPipeEndPtr e);

        void setLength(double l);
        void computeGeometry();
};

class VRPipeNode {
    public:
        int nID = 0;
        VREntityPtr entity;
        string name;
        vector<VRPipeEndPtr> pipes;

    public:
        VRPipeNode(VREntityPtr entity);
        ~VRPipeNode();

        static VRPipeNodePtr create(VREntityPtr entity);
};

class VRPipeSystem : public VRGeometry {
	private:
        GraphPtr graph;
        VROntologyPtr ontology;

        VRUpdateCbPtr updateCb;
        VRAnalyticGeometryPtr inspectionTool;
        int inspected = -1;

        bool doVisual = false;
        bool rebuildMesh = true;
        float spread = 0.1;
        double latency = 0.001;
        vector<string> layers = { "p", "d", "v", "n" };

        double gravity = 9.81;
        double atmosphericPressure = 101325; // Pa at sea level (1 atm)

        map<int, VRPipeNodePtr> nodes;
        map<string, int> nodesByName;
        map<int, VRPipeSegmentPtr> segments;

        void initOntology();

        vector<VRPipeSegmentPtr> getPipes(int nID);
        vector<VRPipeSegmentPtr> getInPipes(int nID);
        vector<VRPipeSegmentPtr> getOutPipes(int nID);

        bool goesIn(VRPipeSegmentPtr s, int nID);
        bool goesOut(VRPipeSegmentPtr s, int nID);
        VREntityPtr getEntity(string name);
        void setupMaterial();

        void computeEndOffset(VRPipeEndPtr e);

        void assignBoundaryPressures();
        void computeDynamicPipeResistances();
        void solveNodeHeads();
        void computePipeFlows(double dt);
        void updateLevels(double dt);

	public:
		VRPipeSystem();
		~VRPipeSystem();

		static VRPipeSystemPtr create();
		VRPipeSystemPtr ptr();

        GraphPtr getGraph();
        VROntologyPtr getOntology();

		int addNode(string name, PosePtr pos, string type, map<string, string> params);
		int addSegment(double radius, int n1, int n2, double level, double h1, double h2);
		void remNode(int nID);
		void remSegment(int eID);
		int getNode(string name);
		string getNodeName(int nID);
		VREntityPtr getNodeEntity(int nID);
		int getSegment(int n1, int n2);

		void setNodePose(int nID, PosePtr p);
        int disconnect(int nID, int sID);
        int insertSegment(int nID, int sID, float radius);
		void setFlowParameters(float latency);

		void setDoVisual(bool b, float spread = 0.1);
		void setVisuals(vector<string> layers);

		void update();
		void updateVisual();
		void updateInspection(int nID);

		PosePtr getNodePose(int i);
		double getPipeRadius(int i);
		double getSegmentPressure(int i);
		Vec2d getSegmentGradient(int i);
		double getSegmentDensity(int i);
		double getSegmentFlow(int i);
		double getTankPressure(string n);
		double getTankDensity(string n);
		double getTankLevel(string n);
		double getPump(string n);
		bool getValveState(string n);

		void setValve(string n, bool b);
		void setPump(string n, double h, bool io);
		void setTankPressure(string n, double p);
		void setTankDensity(string n, double p);
		void setOutletDensity(string n, double p);
		void setOutletPressure(string n, double p);
		void setPipeRadius(int i, double r);
		void setPipePressure(int i, double p1, double p2);

		double computeTotalMass();
        void printSystem();
};

OSG_END_NAMESPACE;

#endif //VRPIPESYSTEM_H_INCLUDED
