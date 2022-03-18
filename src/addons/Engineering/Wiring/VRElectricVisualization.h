#ifndef VRELECTRICVISUALIZATION_H_INCLUDED
#define VRELECTRICVISUALIZATION_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGColor.h>
#include "addons/Engineering/VREngineeringFwd.h"
#include "core/objects/object/VRObject.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRElectricVisualization : public VRObject {
	private:
	    VRElectricSystemPtr system;
	    VRAnalyticGeometryPtr cableViz;
	    map<string, VRAnalyticGeometryPtr> modulesViz;

        GraphPtr vizEGraph;
        map<int, VRElectricComponentPtr> vizEGraphData;
        map<string, int> vizEGraphMap;
        vector<int> vizEGraphFixed;

        GraphPtr vizPGraph;
        map<int, VRElectricComponentPtr> vizPGraphData;
        map<string, int> vizPGraphMap;
        vector<int> vizPGraphFixed;

	    void addModuleViz(string module, Color4f c);
	    void init();

	    void computeGraphs();
        void drawNodes();
        void drawWires();

	public:
		VRElectricVisualization();
		~VRElectricVisualization();

		static VRElectricVisualizationPtr create();
		VRElectricVisualizationPtr ptr();

		void setSystem(VRElectricSystemPtr system);
		void clear();
		void update();
		void updateWires();
};

OSG_END_NAMESPACE;

#endif //VRELECTRICVISUALIZATION_H_INCLUDED
