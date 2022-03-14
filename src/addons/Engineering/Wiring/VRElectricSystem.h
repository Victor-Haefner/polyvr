#ifndef VRELECTRICSYSTEM_H_INCLUDED
#define VRELECTRICSYSTEM_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/math/VRMathFwd.h"
#include "core/objects/VRObjectFwd.h"
#include "addons/Engineering/VREngineeringFwd.h"

#include <map>
#include <vector>
#include <string>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRElectricSystem : public std::enable_shared_from_this<VRElectricSystem> {
	private:
        map<size_t, VRElectricComponentPtr> components;
        map<string, vector<VRElectricComponentPtr>> componentIDs;
        map<string, VRLADVariablePtr> profinetVariables;

        GraphPtr electricGraph;
        GraphPtr profinetGraph;
        map<size_t, VRElectricComponentPtr> componentsByEGraphID;
        map<size_t, VRElectricComponentPtr> componentsByPGraphID;

        map<string, VRObjectPtr> objectsByName;

	public:
		VRElectricSystem();
		~VRElectricSystem();

		static VRElectricSystemPtr create();
		VRElectricSystemPtr ptr();

		VRElectricComponentPtr newComponent(string name, string eID, string mID);

		VRElectricComponentPtr getComponent(size_t ID);
		map<size_t, VRElectricComponentPtr> getComponents();
		vector<VRElectricComponentPtr> getRegistred(string ID);
		VRLADVariablePtr getVariable(string ID);

		void setVariable(string HWaddr, int c);
		void registerID(string ID, VRElectricComponentPtr c);

		void importECAD();
		void buildECADgraph();
};

OSG_END_NAMESPACE;

#endif //VRELECTRICSYSTEM_H_INCLUDED
