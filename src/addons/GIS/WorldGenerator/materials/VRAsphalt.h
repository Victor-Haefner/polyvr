#ifndef VRASPHALT_H_INCLUDED
#define VRASPHALT_H_INCLUDED

#include "core/objects/material/VRMaterial.h"
#include "addons/GIS/WorldGenerator/VRWorldGeneratorFwd.h"

OSG_BEGIN_NAMESPACE;

class VRAsphalt : public VRMaterial {
    private:
        static string asphalt_vp;
        static string asphalt_fp;
        static string asphalt_dfp;

    public:
        VRAsphalt();
        ~VRAsphalt();
        static VRAsphaltPtr create();
};

OSG_END_NAMESPACE;

#endif // VRASPHALT_H_INCLUDED
