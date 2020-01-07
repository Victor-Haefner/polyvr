#include "VRIES.h"
#include "core/objects/material/VRTexture.h"
#include "core/utils/toString.h"

// specs: http://lumen.iee.put.poznan.pl/kw/iesna.txt

using namespace OSG;

VRIES::VRIES() {}
VRIES::~VRIES() {}

void VRIES::parseLabels(vector<string>& lines, int& i) {
    while (true) {
        if (i >= (int)lines.size()) break;
        string line = lines[i];
        i++;

        labels.push_back(line);
        if (startswith(line, "TILT=")) { // end of labels reached
            tiltMode = splitString(line, '=')[1];
            break;
        }
    }
}

void VRIES::parseParameters(vector<string>& lines, int& i) {
    if (tiltMode == "INCLUDE") {
        i += 4; // TODO: parse tilt parameters
    }

    auto params = splitString(lines[i]); i++;
    Nlamps = toInt(params[0]); // number of lamps
    lampLumens = toFloat(params[1]); // lumen per lamp
    lScale = toFloat(params[2]); // candela multiplier
    NvAngles = toInt(params[3]); // number of vertical angles
    NhAngles = toInt(params[4]); // number of horizontal angles
    photometricType = toInt(params[5]); // photometric type
    unitsType = toInt(params[6]); // type of units
    extents = Vec3d(toFloat(params[7]), toFloat(params[8]), toFloat(params[9])); // width length height

    params = splitString(lines[i]); i++;
    ballastFactor = toFloat(params[0]); // ballast factor
    photometricFactor = toFloat(params[1]); // ballast-lamp photometric factor
    inputWatts = toFloat(params[2]); // input watts
}

void VRIES::parseData(vector<string>& lines, int& i) {
    string data;
    for (; i<(int)lines.size(); i++) data += lines[i] + " ";
    int N = NvAngles*NhAngles;
    //float S = lScale*ballastFactor*photometricFactor;
    float S = 1.0;

    stringstream ss(data);
    vAngles = vector<float>(NvAngles, 0);
    hAngles = vector<float>(NhAngles, 0);
    candela = vector<float>(N, 0);
    for (int i=0; i<NvAngles; i++) ss >> vAngles[i];
    for (int i=0; i<NhAngles; i++) ss >> hAngles[i];
    for (int i=0; i<N; i++) { ss >> candela[i]; candela[i] *= S; }
}

void VRIES::resample() {
    vAngleRes = getMinDelta(vAngles);
    hAngleRes = getMinDelta(hAngles);
    vAngleRange = Vec2i( round(getMin(vAngles)), round(getMax(vAngles)) );
    hAngleRange = Vec2i( round(getMin(hAngles)), round(getMax(hAngles)) );

    symmetry = "NONE";
    if (hAngleRange[1] ==   0) symmetry = "UNI";
    if (hAngleRange[1] ==  90) symmetry = "QUAD";
    if (hAngleRange[1] == 180) symmetry = "MID";

    texWidth = 360.0/getMinDelta(hAngles)+1;
    texHeight = 180.0/getMinDelta(vAngles)+1;
    int N2 = texWidth*texHeight;
    texData = vector<float>(N2, 0);

    auto getCandela = [&](int v, int h) -> float {
        int k = h*NvAngles + v;
        return candela[k];
    };

    auto trilinearInterpolation = [&](int i, int j, float u, float v) -> float {
        float c00 = getCandela(i  ,j  );
        float c10 = getCandela(i+1,j  );
        float c01 = getCandela(i  ,j+1);
        float c11 = getCandela(i+1,j+1);
        return ( c00*(1-u) + c10*u )*(1-v) + ( c01*(1-u) + c11*u )*v;
    };

    auto bilinearInterpolation = [&](int i, int j, float u) -> float {
        float c0 = getCandela(i  ,j  );
        float c1 = getCandela(i+1,j  );
        return c0*(1-u) + c1*u;
    };

    auto getInterpolatedCandela = [&](float va, float ha) -> float {
        if (va < vAngles[0]) va = vAngles[0];
        if (va > vAngles[NvAngles-1]) va = vAngles[NvAngles-1];
        if (ha < hAngles[0]) ha = hAngles[0];
        if (ha > hAngles[NhAngles-1]) ha = hAngles[NhAngles-1];

        for (int v = 1; v<NvAngles; v++) {
            if (NhAngles == 1) { // UNI symmetry
                if (va >= vAngles[v-1] && va <= vAngles[v]) {
                    float u = (va-vAngles[v-1])/(vAngles[v]-vAngles[v-1]);
                    return bilinearInterpolation(v-1, 0, u);
                }
            }

            for (int h = 1; h<NhAngles; h++) {
                if (va >= vAngles[v-1] && va <= vAngles[v]) {
                    if (ha >= hAngles[h-1] && ha <= hAngles[h]) {
                        //int k = h*NvAngles + v;
                        float u = (va-vAngles[v-1])/(vAngles[v]-vAngles[v-1]);
                        float w = (ha-hAngles[h-1])/(hAngles[h]-hAngles[h-1]);
                        return trilinearInterpolation(v-1, h-1, u, w);
                    }
                }
            }
        }
        return 0;
    };

    for (int i=0; i<texHeight; i++) {
        for (int j=0; j<texWidth; j++) {
            float va = i*vAngleRes;
            float ha = j*hAngleRes;

            if (symmetry == "UNI") ha = 0;
            else if (symmetry == "MID" && ha > 180) ha = 360-ha;
            else if (symmetry == "QUAD" && ha > 90  && ha <= 180) ha = 180-ha;
            else if (symmetry == "QUAD" && ha > 180 && ha <= 270) ha = ha-180;
            else if (symmetry == "QUAD" && ha > 270 && ha <= 360) ha = 360-ha;

            float c = getInterpolatedCandela(va, ha);
            int k = i*texWidth + j;
            texData[k] = c;
        }
    }

    if (texData.size() == 0) cout << "Error, VRIES::resample failed" << endl;
}

VRTexturePtr VRIES::setupTexture() {
    auto tex = VRTexture::create();
    tex->setInternalFormat(GL_ALPHA32F_ARB); // important for unclamped float
    auto img = tex->getImage();

    //normalize(candela);
    //img->set( Image::OSG_A_PF, NvAngles, NhAngles, 1, 1, 1, 0, (const uint8_t*)&candela[0], Image::OSG_FLOAT32_IMAGEDATA, true, 1); // for testing

    normalize(texData);
    img->set( Image::OSG_A_PF, texWidth, texHeight, 1, 1, 1, 0, (const uint8_t*)&texData[0], Image::OSG_FLOAT32_IMAGEDATA, true, 1);

    return tex;
}

VRTexturePtr VRIES::read(string path) {
    ifstream file(path);
    string data((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    vector<string> lines = splitString(data, '\n');
    if (lines.size() < 10) return 0;

    int i=0;
    parseLabels(lines, i);
    parseParameters(lines, i);
    parseData(lines, i);

    resample();

    return setupTexture();
}

// utility functions

bool VRIES::startswith(const string& a, const string& b) {
    if (a.size() < b.size()) return false;
    return a.compare(0, b.length(), b) == 0;
}

float VRIES::getMinDelta(vector<float>& v) {
    float d = 1e6;
    for (uint i=1; i<v.size(); i++) {
        float D = v[i] - v[i-1];
        if (D<d && D > 1e-6) d = D;
    }
    return d;
}

float VRIES::getMin(vector<float>& v) {
    float d = 1e6;
    for (auto f : v) if (f<d) d = f;
    return d;
}

float VRIES::getMax(vector<float>& v) {
    float d = -1e6;
    for (auto f : v) if (f>d) d = f;
    return d;
}

void VRIES::normalize(vector<float>& v) {
    float m = getMax(v);
    for (auto& f : v) f /= m;
}

string VRIES::toString(bool withData) {
    string s = "IESNA photometric light data\n";
    s += " parameters:\n";
    s += "  N labels: " + ::toString(labels.size()) + "\n";

    s += "  N lamps: " + ::toString(Nlamps) + "\n";
    s += "  lamp lumens: " + ::toString(lampLumens) + "\n";
    s += "  candela scale: " + ::toString(lScale) + "\n";
    s += "  N vert. Angles: " + ::toString(NvAngles) + "\n";
    s += "  N horz. Angles: " + ::toString(NhAngles) + "\n";
    s += "  photometric type: " + ::toString(photometricType) + "\n";
    s += "  units type: " + ::toString(unitsType) + "\n";
    s += "  extents: " + ::toString(extents) + "\n";

    s += "  ballast factor: " + ::toString(ballastFactor) + "\n";
    s += "  photometric factor: " + ::toString(photometricFactor) + "\n";
    s += "  input watts: " + ::toString(inputWatts) + "\n";

    s += "  Symmetry: " + symmetry + "\n";
    s += "  Angle resolution: " + ::toString(Vec2d(vAngleRes, hAngleRes)) + "\n";
    s += "  vert. Angle range: " + ::toString(vAngleRange) + "\n";
    s += "  horz. Angle range: " + ::toString(hAngleRange) + "\n";
    s += "  N vAngles: " + ::toString(vAngles.size()) + "\n";
    s += "  N hAngles: " + ::toString(hAngles.size()) + "\n";
    s += "  N candela: " + ::toString(candela.size()) + "\n";

    s += "  tex width: " + ::toString(texWidth) + "\n";
    s += "  tex height: " + ::toString(texHeight) + "\n";
    if (withData) {
        s += "   vert. angles: ";
        for (auto a : vAngles) s += " " + ::toString(a);
        s += "\n";
        s += "   horz. angles: ";
        for (auto a : hAngles) s += " " + ::toString(a);
        s += "\n";
        s += "   candela: ";
        for (auto c : candela) s += " " + ::toString(c);
        s += "\n";
    }
    return s;
}

