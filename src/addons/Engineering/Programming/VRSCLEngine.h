#ifndef VRSCLENGINE_H_INCLUDED
#define VRSCLENGINE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/objects/VRTransform.h"
#include "addons/Engineering/VREngineeringFwd.h"

#include <string>
#include <vector>
#include <map>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRSCLScript : public std::enable_shared_from_this<VRSCLScript> {
    private:
        string scl;
        string python;

    public:
		VRSCLScript();
		~VRSCLScript();

		static VRSCLScriptPtr create();
		VRSCLScriptPtr ptr();

		void readSCL(string path);
		void convert();

		string getScl();
		string getPython();
};

class VRSCLEngine : public std::enable_shared_from_this<VRSCLEngine> {
	private:
		VRElectricSystemPtr elSystem;
		//VRPythonEnginePtr pyEngine; // TODO: wrap scriptmanager py engine

		map<string, VRSCLScriptPtr> scripts;

	public:
		VRSCLEngine();
		~VRSCLEngine();

		static VRSCLEnginePtr create();
		VRSCLEnginePtr ptr();

		void setElectricEngine(VRElectricSystemPtr esystem);
		VRSCLScriptPtr readSCL(string name, string path);
		VRSCLScriptPtr getScript(string name);

		void iterate();
};

OSG_END_NAMESPACE;

#endif // VRSCLENGINE_H_INCLUDED
