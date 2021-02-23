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
        float radius = 0;
        float length = 0;
        float area = 0;
        float volume = 0;

        float pressure = 1.0;
        float lastPressureDelta = 0.0;

    public:
        VRPipeSegment(float radius, float length);
        ~VRPipeSegment();

        static VRPipeSegmentPtr create(float radius, float length);

        void handleTank(float& pressure, float otherVolume, float dt);
        void handleValve(float area, VRPipeSegmentPtr other, float dt);
        void handlePump(float performance, VRPipeSegmentPtr other, float dt);

        void addMass(float m);
};

class VRPipeNode {
    public:
        VREntityPtr entity;
        float lastPressureDelta = 0.0;

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

        map<int, VRPipeNodePtr> nodes;
        map<int, VRPipeSegmentPtr> segments;

        void initOntology();

        vector<VRPipeSegmentPtr> getPipes(int nID);
        vector<VRPipeSegmentPtr> getInPipes(int nID);
        vector<VRPipeSegmentPtr> getOutPipes(int nID);

	public:
		VRPipeSystem();
		~VRPipeSystem();

		static VRPipeSystemPtr create();
		VRPipeSystemPtr ptr();

		int addNode(PosePtr pos, string type, map<string, string> params);
		int addSegment(float radius, float length, int n1, int n2);

		void setDoVisual(bool b);

		void update();
		void updateVisual();
		VROntologyPtr getOntology();

		void setValve(int nID, bool b);
};

OSG_END_NAMESPACE;

#endif //VRPIPESYSTEM_H_INCLUDED
