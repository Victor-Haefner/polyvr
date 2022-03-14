#ifndef VRELECTRICCOMPONENT_H_INCLUDED
#define VRELECTRICCOMPONENT_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>

#include "core/objects/VRObjectFwd.h"
#include "addons/Semantics/VRSemanticsFwd.h"
#include "addons/Engineering/VREngineeringFwd.h"

#include <map>
#include <vector>
#include <string>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRElectricComponent : public std::enable_shared_from_this<VRElectricComponent> {
	public:
	    struct Address {
	        Address(string a = "") : address(a) {}

			string address;
			string ecadID;
			string machine;
			string component;
			string socket;
			string port;
	    };

	    struct Port {
	        Port(string n = "") : name(n) {}

			string name;
			VREntityPtr entity;
			VRWirePtr connection;
			string ladHWaddr;
			string ecadHWaddr;
			string socket;
	    };

	    VRElectricSystemWeakPtr system;

        vector<VRWirePtr> connections;
        map<string, Port> ports;
        size_t vrID = 0;
        int egraphID = -1;
        int pgraphID = -1;

        string name;
        string ecadID;
        string mcadID;
        Address address;

        Vec3d position;
        VRObjectPtr geometry;
        VREntityPtr entity;
        int flag = 0;

	public:
		VRElectricComponent(VRElectricSystemPtr sys, string name, string eID, string mID);
		~VRElectricComponent();

		static VRElectricComponentPtr create(VRElectricSystemPtr sys, string name, string eID, string mID);
		VRElectricComponentPtr ptr();

		void setCurrent(string current, string port);
        VRWirePtr getWire(VRElectricComponentPtr c2);
        VRWirePtr getConnection(string port);
        void registerID(string ID);
        void setName(string n);
        void setEcadID(string eID);
        void setMcadID(string mID);
        void setGeometry(VRObjectPtr obj);
        void setEntity(VREntityPtr obj);
        void setPosition(Vec3d p);

        int getEGraphID();
        int getPGraphID();
        void setEGraphID(int ID);
        void setPGraphID(int ID);

        void addPort(string port, string ladHWaddr, string ecadHWaddr, string socket);
        void setPortEntity(string port, VREntityPtr e);

        string getName();
        string getEcadID();
        string getMcadID();
        VRObjectPtr getGeometry();
        VREntityPtr getEntity();
        vector<VRWirePtr> getConnections();
        vector<string> getPorts();
        VRWirePtr getPortWire(string port);
        VREntityPtr getPortEntity(string port);
        Vec3d getPosition();

        Address getAddress();
        bool hasAddress();
        string getAddressMachine();
};

OSG_END_NAMESPACE;

#endif //VRELECTRICCOMPONENT_H_INCLUDED
