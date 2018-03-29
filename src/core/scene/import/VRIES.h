#ifndef VRIES_H_INCLUDED
#define VRIES_H_INCLUDED

#include <OpenSG/OSGVector.h>
#include "core/objects/VRObjectFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRIES {
    private:
        // data from parsing the file
        vector<string> labels;
        string tiltMode = "NONE";

        int Nlamps = 1;
        float lampLumens = 1000;
        float lScale = 1;
        int NvAngles = 0;
        int NhAngles = 0;
        int photometricType = 0;
        int unitsType = 0;
        Vec3d extents;

        float ballastFactor = 1;
        float photometricFactor = 1;
        float inputWatts = 0;

        vector<float> vAngles;
        vector<float> hAngles;
        vector<float> candela;


        // computed parameters
        float vAngleRes = 0;
        float hAngleRes = 0;
        Vec2i vAngleRange = 0;
        Vec2i hAngleRange = 0;
        string symmetry = "NONE";

        vector<float> texData;
        int texWidth = 1;
        int texHeight = 1;


        // utility functions
        void parseLabels(vector<string>& lines, int& i);
        void parseParameters(vector<string>& lines, int& i);
        void parseData(vector<string>& lines, int& i);

        void resample();
        VRTexturePtr setupTexture();

        bool startswith(const string& a, const string& b);
        void normalize(vector<float>& v);
        float getMin(vector<float>& v);
        float getMax(vector<float>& v);
        float getMinDelta(vector<float>& v);

    public:
        VRIES();
        ~VRIES();

        VRTexturePtr read(string path);

        string toString(bool withData = false);
};

OSG_END_NAMESPACE;

#endif // VRIES_H_INCLUDED
