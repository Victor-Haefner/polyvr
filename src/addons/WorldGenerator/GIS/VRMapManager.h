#ifndef VRMAPMANAGER_H_INCLUDED
#define VRMAPMANAGER_H_INCLUDED

#include <string>
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGVector.h>
#include "core/networking/VRNetworkingFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "GISFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;


ptrFctFwd( VRMap, VRMapDescriptorPtr );

class VRMapDescriptor {
    private:
        map<int, string> layers;
        double N = 0;
        double E = 0;
        double S = 0;
        bool complete = true;

    public:
        VRMapDescriptor();
        ~VRMapDescriptor();

		static VRMapDescriptorPtr create();
		static VRMapDescriptorPtr create(double n, double e, double s);

		string getMap(int i);
		Vec3d getParameters();

		void setMap(int i, string s);
		void setParameters(double n, double e, double s);

		void setCompleteness(bool c);
		bool isComplete();
};

class VRMapManager : public std::enable_shared_from_this<VRMapManager> {
	private:
        struct MapType {
            int ID;
            string vault;
            string servScript;
            string fileExt;
        };

        string server;
        map<int, MapType> mapTypes;

        VRRestClientPtr client;

        void storeFile(const string& filename, const string& data);
        string constructFilename(double N, double E, double S, int mapType);

        void handleRequestAnswer(VRRestResponsePtr response, string filename, VRMapCbPtr mcb, double N, double E, double S, vector<int> types);
        void requestFile(string filename, double N, double E, double S, int mapType, vector<int> types, VRMapCbPtr cb);

        void triggerCB(VRMapCbPtr mcb, VRMapDescriptorPtr data);

	public:
		VRMapManager();
		~VRMapManager();

		static VRMapManagerPtr create();
		VRMapManagerPtr ptr();

		void setServer(string address);
		void addMapType(int ID, string vault, string servScript, string fileExt);

		VRMapDescriptorPtr getMap(double N, double E, double S, vector<int> types, VRMapCbPtr cb);
};

OSG_END_NAMESPACE;

#endif //VRMAPMANAGER_H_INCLUDED
