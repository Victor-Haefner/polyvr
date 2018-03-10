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

        static string asphalt_fp_head;
        static string asphalt_fp_core;
        static string asphalt_dfp_core;

        static string asphaltArrow_fp;
        static string asphaltArrow_dfp;

        struct road {
            int markingsN = 0;
            int tracksN = 0;
            int rDataLengths = 1; // pixel length of line at rID
        };

        map<int, road> roadData;
        VRTextureGeneratorPtr texGen;
        VRTexturePtr pathTex;
        VRTexturePtr noiseTex;
        VRTexturePtr mudTex;

        VRTexturePtr noiseTexture();
        VRTexturePtr mudTexture();
        void addPath(PathPtr path, int rID, float width, float dashL, float offset);

    public:
        VRAsphalt();
        ~VRAsphalt();
        static VRAsphaltPtr create();

        void setMarkingsColor(Color4f);
        void setArrowMaterial();

        void clearTexture();
        void updateTexture();
        void addTrack(int rID, PathPtr track, float width, float dashL, float offset = 0);
        void addMarking(int rID, PathPtr marking, float width, float dashL, float offset = 0);

        double getMemoryConsumption();
};

OSG_END_NAMESPACE;

#endif // VRASPHALT_H_INCLUDED
