#ifndef VRMAPMANAGER_H_INCLUDED
#define VRMAPMANAGER_H_INCLUDED

#include <string>
#include <OpenSG/OSGConfig.h>
#include "core/networking/VRNetworkingFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "GISFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRMapManager : public std::enable_shared_from_this<VRMapManager> {
	private:
        string server;
        string vault;

        VRRestClientPtr client;

        void storeFile(const string& filename, const string& data);
        string constructFilename(double N, double E, double S);
        void requestFile(string filename, double N, double E, double S, VRMessageCbPtr cb);

	public:
		VRMapManager();
		~VRMapManager();

		static VRMapManagerPtr create();
		VRMapManagerPtr ptr();

		void setServer(string s);
		void setVault(string v);

		string getMap(double N, double E, double S, VRMessageCbPtr cb);
};

OSG_END_NAMESPACE;

#endif //VRMAPMANAGER_H_INCLUDED
