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

        double headFlow = 0.0;
        double maxFlow = 0.0;
        double flow = 0.0;

        double heatFlux = 0.0;
        double temperature = 20.0;

        double hydraulicHead = 0.0;
        double pressure = 1.0;
        bool pressurized = false;

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
        double liquidMin = 0.0;
        double liquidMax = 0.0;

        double resistance = 0.0;
        double density = 1000.0; // kg / m3
        double viscosity = 1e-3; // Pa s
        double level = 0.0;
        double hydraulicHead = 0.0;
        double lastVizLevel = -1.0;
        double friction = 0.02;
        double thermalConductance = 1.0;
        bool pressurized = false;
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
        VRAnimCbPtr gaugeCb;

        // for control valves
        map<int, int> endGroup;
        map<int, double> pathOpenings;

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
        //double atmosphericPressure = 0.0; // set to 0 because its just a global offset
        double waterDensity = 1000.0; // kg / m3

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
        void computeHydraulicHead(VRPipeEndPtr e);

        void assignBoundaryPressures();
        void solveNodeHeads();
        void computeHeadFlows(double dt);
        void computeMaxFlows(double dt);
        void updateLevels(double dt);
        void computeAdvectiveHeatTransfer(double dt);
        void updatePressures(double dt);

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
		double getTankPressure(int nID);
		double getTankDensity(int nID);
		double getTankLevel(int nID);
		double getPump(int nID);
		double getValveState(int nID);

		void setValve(int nID, double b);
		void setPump(int nID, double h, bool io);
		void setTankPressure(int nID, double p);
		void setTankDensity(int nID, double p);
		void setOutletDensity(int nID, double p);
		void setOutletPressure(int nID, double p);
		void setPipeRadius(int i, double r);
		void setPipePressure(int i, double p1, double p2);

		void addControlValvePath(int i, int A, int B, double x0, double xs, double K);

		void setGaugeCb(int i, VRAnimCbPtr cb);

		double computeTotalMass();
        void printSystem();
};

OSG_END_NAMESPACE;

#endif //VRPIPESYSTEM_H_INCLUDED
