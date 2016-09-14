#ifndef VRPROCESSLAYOUT_H_INCLUDED
#define VRPROCESSLAYOUT_H_INCLUDED

#include "../VRSemanticsFwd.h"
#include "core/objects/VRTransform.h"
#include "VRProcess.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRProcessLayout : public VRTransform {
    private:
        VRProcessPtr process;

        VRTransformPtr newWidget(VRProcess::Node& n, float height);

    public:
        VRProcessLayout(string name = "");
        ~VRProcessLayout();

        static VRProcessLayoutPtr create(string name = "");
        VRProcessLayoutPtr ptr();

        void setProcess(VRProcessPtr p);
};

OSG_END_NAMESPACE;

#endif // VRPROCESSLAYOUT_H_INCLUDED
