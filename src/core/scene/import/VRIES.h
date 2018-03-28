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

        // texture parameters
        vector<float> texData;

        /*float resolution = 0;
        Vec2d vARange = 0;
        Vec2d hARange = 0;
        int vNAngles = 0;
        int hNAngles = 0;*/

        void parseLabels(vector<string>& lines, int& i);
        void parseParameters(vector<string>& lines, int& i);
        void parseData(vector<string>& lines, int& i);
        VRTexturePtr resample();

        bool startswith(const string& a, const string& b);

    public:
        VRIES();
        ~VRIES();

        VRTexturePtr read(string path);

        string toString(bool withData = false);
};

OSG_END_NAMESPACE;

#endif // VRIES_H_INCLUDED
