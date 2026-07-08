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

class VRFluidComposition {
    public:
        struct ParticleBin {
            string ID;
            double sizeMin;
            double sizeMax;
            double density;
            double volumeFraction;
        };

        double temperature = 20.0;
        double effectiveDensity = 1000.0; // kg / m3
        double effectiveViscosity = 1e-3; // Pa s

        double baseDensity = 1000.0; // kg / m3 at 20 C
        double baseViscosity = 1e-3; // Pa s at 20 C

        map<string, ParticleBin> particles;

        void updateThermalDependencies();

        void mixIn(const VRFluidComposition& fluid, const double& percentage);
        void fromEntity(VREntityPtr e);
        bool toEntity(VREntityPtr e);

        double computeFluidMass(double Volume);
        double computeParticlesMass(double Volume);
};

class VRPipeEnd {
    public:
        VRPipeSegmentWeakPtr pipe;
        int nID = -1;

        double height = 0.0;
        double heightMax = 0.0;
        double offsetHeight = 0.0; // used for tank offset
        Vec3d offset; // offset relative to node center

        double pressure = 0.0;
        double hydraulicHead = 0.0;
        int stateID = -1;
        bool pressurized = false;

        double headFlow = 0.0;
        double maxFlow = 0.0;
        double flow = 0.0;
        double flowClamp = 0.0;

        VRFluidComposition fluid;

    public:
        VRPipeEnd(VRPipeSegmentPtr s, int nID, double height = 0.0);
        ~VRPipeEnd();

        void updateGeometry(GraphPtr graph);
        void setInitialHead();

        static VRPipeEndPtr create(VRPipeSegmentPtr s, int nID, double height = 0.0);
};

class VRPipeSegment {
    public:
        int eID = 0;
        double radius = 0.0;
        double length = 0.0;
        double area = 0.0;
        double volume = 0.0;
        double fluidMin = 0.0;
        double fluidMax = 0.0;
        double fluidLvl = 0.0;
        double zRadius = 0.0;

        double gravity = 9.81;
        double resistanceLaminar = 0.0;
        double resistanceTurbulent = 0.0;
        //double resistance = 0.0;
        double regime = 1.0; // 0 laminar, 1 turbulent
        double level = 0.0;
        double hydraulicHead = 0.0;
        int stateID = -1;
        double lastVizLevel = -1.0;
        bool pressurized = false;
        bool flowBlocked = false;

        int materialID = 0;
        int environmentID = 0;

        VRFluidComposition fluid;

        VRPipeEndWeakPtr end1;
        VRPipeEndWeakPtr end2;

        double computeExchange(double hole, VRPipeSegmentPtr other, double dt, bool p1, bool op1);

    public:
        VRPipeSegment(int eID, double radius, double length, double level);
        ~VRPipeSegment();

        static VRPipeSegmentPtr create(int eID, double radius, double length, double level);
        VRPipeEndPtr otherEnd(VRPipeEndPtr e);

        double computeRegime(double Q);
        void updateResistance(double friction);
        void setLevel(double lvl);
        void updateGeometry(GraphPtr graph, double friction);

        double computeEffectiveResistance(const double& flow);
};

class VRPipeNode {
    public:
        int nID = 0;
        int stateID = -1;
        VREntityPtr entity;
        string name;
        vector<VRPipeEndPtr> pipes;
        VRAnimCbPtr userCb;

        // for control valves
        map<int, int> endsGroup; // key is pipe end i, value is group ID
        map<int, vector<VRPipeEndPtr>> endGroups; // key is group ID
        map<int, double> pathOpenings;

    public:
        VRPipeNode(VREntityPtr entity);
        ~VRPipeNode();

        static VRPipeNodePtr create(VREntityPtr entity);
};

class VRPipeSystem : public VRTransform {
    public:
        struct Environment {
            double temperature = 20.0;
            double volume = 100.0;
        };

        struct Material {
            double thermalConductance = 10.0;
            double friction = 0.02;
        };

        using EnvironmentPtr = shared_ptr<Environment>;
        using MaterialPtr = shared_ptr<Material>;

	private:
        GraphPtr graph;
        VROntologyPtr ontology;

        VRUpdateCbPtr updateCb;
        VRObjectPtr visuals;
        VRAnalyticGeometryPtr inspectionTool;
        int inspected = -1;

        bool doVisual = false;
        bool rebuildMesh = true;
        float spread = 0.1;
        double latency = 0.001;
        vector<string> layers = { "p", "d", "v", "n" };

        double timeScale = 1.0;
        double gravity = 9.81;
        double atmosphericPressure = 101325; // Pa at sea level (1 atm)
        //double atmosphericPressure = 0.0; // set to 0 because its just a global offset
        double waterDensity = 1000.0; // kg / m3

        map<int, VRPipeNodePtr> nodes;
        map<string, int> nodesByName;
        map<int, VRPipeSegmentPtr> segments;
        vector<EnvironmentPtr> environments;
        vector<MaterialPtr> materials;

        void initOntology();

        vector<VRPipeSegmentPtr> getPipes(int nID);
        vector<VRPipeSegmentPtr> getInPipes(int nID);
        vector<VRPipeSegmentPtr> getOutPipes(int nID);

        double clamp(double x, double a, double b, bool warn = false, string label = "");

        bool goesIn(VRPipeSegmentPtr s, int nID);
        bool goesOut(VRPipeSegmentPtr s, int nID);
        VREntityPtr getEntity(string name);
        VRMaterialPtr setupMaterial();

        void computeEndOffset(VRPipeEndPtr e);

        void updateNodePaths();
        void assignBoundaryPressures(double dt);
        void solveNodeHeads(double dt);
        void computeHeadFlows(double dt);
        void computeMaxFlows(double dt);
        void updateLevels(double dt);
        void updatePressurization(double dt);
        void computeFlowMixing(double dt);
        void radiateHeat(double dt);
        void updateThermalDependencies(double dt);
        void updatePressures(double dt);
        void updateRegimes(double dt);

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
		void setTimeScale(double s);

		void update();
		void updateVisual();
		void updateInspection(int nID);

		PosePtr getNodePose(int i);
		double getPipeRadius(int i);
		double getSegmentPressure(int i);
		Vec2d getSegmentGradient(int i);
		double getSegmentDensity(int i);
		Vec2d getSegmentFlow(int i);
		Vec2d getSegmentHeadFlow(int i);
		Vec2d getSegmentHead(int i);
		Vec2d getSegmentTemperature(int i);
		double getTankPressure(int nID);
		double getTankDensity(int nID);
		double getTankLevel(int nID);
		double getPump(int nID);
		double getValveState(int nID);

		void setValve(int nID, double b);
		void setPump(int nID, double h, bool io);
		void setTankPressure(int nID, double p);
		void setTankTemperature(int nID, double p);
		void setTankDensity(int nID, double p);
		void setOutletDensity(int nID, double p);
		void setOutletPressure(int nID, double p);
		void setPipeRadius(int i, double r);
		void setPipePressure(int i, double p1, double p2);

		double getTankParticles(int i, string type);
		void setTankParticles(int i, string type, double volFrac);
		void addTankParticles(int i, string type, double mass);
		double removeTankParticles(int i, string type, double part);
		void addTankParticleBin(int i, string type, Vec2d sizeRange, double density);

		void addControlValvePath(int i, int A, int B, double x0, double xs, double K);

		int addMaterial();
		void setSegmentMaterial(int sID, int mID);
		void setMaterialFriction(int mID, double f);
		void setMaterialThermalConductivity(int mID, double c);

		int addEnvironment();
		void setSegmentEnvironment(int sID, int eID);
		void setEnvironmentVolume(int eID, double V);
		void setEnvironmentTemperature(int eID, double T);
		double getEnvironmentTemperature(int eID);


		void setNodeCb(int i, VRAnimCbPtr cb);

		Vec2d computeTotalMass();
        void printSystem();
};

OSG_END_NAMESPACE;

#endif //VRPIPESYSTEM_H_INCLUDED
