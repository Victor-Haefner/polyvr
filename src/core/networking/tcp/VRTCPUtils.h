#ifndef VRTCPUTILS_H_INCLUDED
#define VRTCPUTILS_H_INCLUDED

#include "../VRNetworkingFwd.h"

#include <string>
#include <functional>

using namespace std;
namespace OSG {

class VRTCPUtils {
    public:
        static string getLocalIP();
        static string getPublicIP(bool cached = false);
        static string getHostName(string uri);
        static string getHostIP(string host);
};

}

#endif // VRTCPUTILS_H_INCLUDED
