#ifndef VRASPHALT_H_INCLUDED
#define VRASPHALT_H_INCLUDED

#include "core/objects/material/VRMaterial.h"
#include "addons/WorldGenerator/VRWorldGeneratorFwd.h"

OSG_BEGIN_NAMESPACE;

class VRAsphalt : public VRMaterial {
    private:
        static string asphalt_vp;
        static string asphalt_fp;
        static string asphalt_dfp;

        struct road {
            int markingsN = 0;
            int tracksN = 0;
            int rDataLengths = 1; // pixel length of line at rID
        };

        map<int, road> roadData;
        VRTextureGeneratorPtr texGen;

        VRTexturePtr noiseTexture();
        VRTexturePtr mudTexture();
        void addPath(pathPtr path, int rID, float width, int dashN);

    public:
        VRAsphalt();
        ~VRAsphalt();
        static VRAsphaltPtr create();

        void clearTexture();
        void updateTexture();
        void addTrack(int rID, pathPtr track, float width, int dashN);
        void addMarking(int rID, pathPtr marking, float width, int dashN);
};

OSG_END_NAMESPACE;

#endif // VRASPHALT_H_INCLUDED
