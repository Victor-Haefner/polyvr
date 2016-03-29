#ifndef VRPING_H_INCLUDED
#define VRPING_H_INCLUDED

#include <string>

class VRPing {
    private:
    public:
        VRPing();

        bool start(std::string IP, std::string port, int timeout);
        bool start(std::string IP, int timeout);
};

#endif // VRPING_H_INCLUDED
