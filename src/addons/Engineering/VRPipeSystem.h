#ifndef VRPIPESYSTEM_H_INCLUDED
#define VRPIPESYSTEM_H_INCLUDED

#include <map>
#include <vector>
#include <OpenSG/OSGConfig.h>
#include "VREngineeringFwd.h"
#include "core/math/VRMathFwd.h"
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

    public:
        VRPipeSegment(float radius, float length);
        ~VRPipeSegment();

        static VRPipeSegmentPtr create(float radius, float length);

        void mixPressure(float& pressure, float otherVolume, float dt);
        void addPressure(float performance, float dt);
};

class VRPipeNode {
    public:
        VREntityPtr entity;

    public:
        VRPipeNode(VREntityPtr entity);
        ~VRPipeNode();

        static VRPipeNodePtr create(VREntityPtr entity);
};

class VRPipeSystem : public std::enable_shared_from_this<VRPipeSystem> {
	private:
        GraphPtr graph;
        VROntologyPtr ontology;

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

		int addNode(string type);
		int addSegment(float radius, float length, int n1, int n2);

		void update();
		VROntologyPtr getOntology();
};

OSG_END_NAMESPACE;

#endif //VRPIPESYSTEM_H_INCLUDED
