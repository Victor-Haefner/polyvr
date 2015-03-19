#ifndef VRFLAGS_H_INCLUDED
#define VRFLAGS_H_INCLUDED

#include <unordered_map>
#include <string>

class VRFlags {
    private:
        std::unordered_map<int, bool> flags;

    public:
        VRFlags();

        void setFlag(std::string flag, bool b = true);
        bool hasFlag(std::string flag);
        bool getFlag(std::string flag);

        void printFlags();
};

#endif // VRFLAGS_H_INCLUDED
