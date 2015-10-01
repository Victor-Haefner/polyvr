#ifndef VRFRAMEWORK_H_INCLUDED
#define VRFRAMEWORK_H_INCLUDED

#include <OpenSG/OSGConfig.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class Node;

class PolyVR {
    private:
        void setMultisampling(bool on);

        PolyVR();

    public:
        ~PolyVR();
        static PolyVR& get();

        void init(int argc, char **argv);

        void start();
        void exit();

        void startTestScene(Node* n);
};

OSG_END_NAMESPACE;

#endif // VRFRAMEWORK_H_INCLUDED
