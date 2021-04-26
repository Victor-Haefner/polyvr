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
        static string getPublicIP();
};

}

#endif // VRTCPUTILS_H_INCLUDED
