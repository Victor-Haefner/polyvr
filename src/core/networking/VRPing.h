#ifndef VRPING_H_INCLUDED
#define VRPING_H_INCLUDED

#include <string>
#include <memory>

namespace OSG {

class VRPing {
    private:
    public:
        VRPing();
        static std::shared_ptr<VRPing> create();

        bool startOnPort(std::string IP, std::string port, int timeout);
        bool start(std::string IP, int timeout);
        std::string getMAC(std::string IP, std::string interface);
};

}

#endif // VRPING_H_INCLUDED
