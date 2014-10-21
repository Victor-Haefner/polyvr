#ifndef VRFLAGS_H_INCLUDED
#define VRFLAGS_H_INCLUDED

#include <map>

class VRFlags {
    private:
        std::map<int, bool> flags;

    public:
        VRFlags();

        void addFlag(int);
        bool hasFlag(int);
        void setFlag(int, bool = true);
        bool getFlag(int);

        void printFlags();
};

#endif // VRFLAGS_H_INCLUDED
