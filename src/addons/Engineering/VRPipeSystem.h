#ifndef VRPIPESYSTEM_H_INCLUDED
#define VRPIPESYSTEM_H_INCLUDED

#include <map>
#include <vector>
#include <OpenSG/OSGConfig.h>
#include "VREngineeringFwd.h"
#include "core/math/VRMathFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/objects/geometry/VRGeometry.h"
#include "addons/Semantics/VRSemanticsFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRPipeSegment {
    public:
        int eID = 0;
        double radius = 0;
        double length = 0;
        double area = 0;
        double volume = 0;
        double density = 1.0;
        double flow = 0.0;
        double dFl1 = 0.0;
        double dFl2 = 0.0;
        bool flowBlocked = false;

        double pressure1 = 1.0;
        double pressure2 = 1.0;

        double computeExchange(double hole, VRPipeSegmentPtr other, double dt, bool p1, bool op1);

    public:
        VRPipeSegment(int eID, double radius, double length);
        ~VRPipeSegment();

        static VRPipeSegmentPtr create(int eID, double radius, double length);

        void handleTank(double& pressure, double otherVolume, double& otherDensity, double dt, bool p1);
        void handleValve(double area, VRPipeSegmentPtr other, double dt, bool p1, bool op1);
        void handleOutlet(double area, double extPressure, double extDensity, double dt, bool p1);
        void handlePump(double performance, double maxPressure, bool isOpen, VRPipeSegmentPtr other, double dt, bool p1, bool op1);

        void addEnergy(double m, double d, bool p1);
        void setLength(double l);
        void computeGeometry();
};

class VRPipeNode {
    public:
        VREntityPtr entity;
        string name;
        double lastPressureDelta = 0.0;

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

        bool doVisual = false;
        bool rebuildMesh = true;
        float spread = 0.1;

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

	public:
		VRPipeSystem();
		~VRPipeSystem();

		static VRPipeSystemPtr create();
		VRPipeSystemPtr ptr();

        GraphPtr getGraph();
        VROntologyPtr getOntology();

		int addNode(string name, PosePtr pos, string type, map<string, string> params);
		int addSegment(double radius, int n1, int n2);
		void remNode(int nID);
		void remSegment(int eID);
		int getNode(string name);
		string getNodeName(int nID);
		VREntityPtr getNodeEntity(int nID);
		int getSegment(int n1, int n2);

		void setNodePose(int nID, PosePtr p);
        int disconnect(int nID, int sID);
        int insertSegment(int nID, int sID, float radius);
		void setDoVisual(bool b, float spread = 0.1);

		void update();
		void updateVisual();

		PosePtr getNodePose(int i);
		double getSegmentPressure(int i);
		Vec2d getSegmentGradient(int i);
		double getSegmentDensity(int i);
		double getSegmentFlow(int i);
		Vec2d getSegmentFlowAccelleration(int i);
		double getTankPressure(string n);
		double getTankDensity(string n);
		double getTankVolume(string n);
		double getPump(string n);
		bool getValveState(string n);

		void setValve(string n, bool b);
		void setPump(string n, double p, double pmax);
		void setTankPressure(string n, double p);
		void setTankDensity(string n, double p);
		void setPipeRadius(int i, double r);

        void printSystem();
};

OSG_END_NAMESPACE;

#endif //VRPIPESYSTEM_H_INCLUDED
