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

    public:
        VRMapDescriptor();
        ~VRMapDescriptor();

		static VRMapDescriptorPtr create();
		static VRMapDescriptorPtr create(double n, double e, double s, string f);

		string getMap(int i);
		Vec3d getParameters();

		void setMap(int i, string s);
		void setParameters(double n, double e, double s);
};

class VRMapManager : public std::enable_shared_from_this<VRMapManager> {
	private:
        string server;
        string vault;

        VRRestClientPtr client;

        void storeFile(const string& filename, const string& data);
        string constructFilename(double N, double E, double S);
        void requestFile(string filename, double N, double E, double S, VRMapCbPtr cb);

        void triggerCB(VRMapCbPtr mcb, VRMapDescriptorPtr data);

	public:
		VRMapManager();
		~VRMapManager();

		static VRMapManagerPtr create();
		VRMapManagerPtr ptr();

		void setServer(string s);
		void setVault(string v);

		string getMap(double N, double E, double S, VRMapCbPtr cb);
};

OSG_END_NAMESPACE;

#endif //VRMAPMANAGER_H_INCLUDED
