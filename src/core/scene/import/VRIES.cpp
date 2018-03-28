#include "VRIES.h"
#include "core/objects/material/VRTexture.h"
#include "core/utils/toString.h"

using namespace OSG;

VRIES::VRIES() {}
VRIES::~VRIES() {}

void VRIES::parseLabels(vector<string>& lines, int& i) {
    while (true) {
        if (i >= lines.size()) break;
        string line = lines[i];
        i++;

        labels.push_back(line);
        if (startswith(line, "TILT=")) break;
    }
}

void VRIES::parseParameters(vector<string>& lines, int& i) {
    while (true) {
        if (i >= lines.size()) break;
        auto params = splitString( lines[i] );
        i++;

        if (params.size() >= 10) {
            Nlamps = toInt(params[0]); // number of lamps
            lampLumens = toFloat(params[1]); // lumen per lamp
            lScale = toFloat(params[2]); // candela multiplier
            NvAngles = toInt(params[3]); // number of vertical angles
            NhAngles = toInt(params[4]); // number of horizontal angles
            photometricType = toInt(params[5]); // photometric type
            unitsType = toInt(params[6]); // type of units
            extents = Vec3d(toFloat(params[7]), toFloat(params[8]), toFloat(params[9])); // width length height
        }

        if (params.size() == 3) {
            ballastFactor = toFloat(params[0]); // ballast factor
            photometricFactor = toFloat(params[1]); // ballast-lamp photometric factor
            inputWatts = toFloat(params[2]); // input watts
            return;
        }
    }
}

void VRIES::parseData(vector<string>& lines, int& i) {
    string data;
    for (; i<lines.size(); i++) data += lines[i] + " ";
    int N = NvAngles*NhAngles;

    stringstream ss(data);
    vAngles = vector<float>(NvAngles, 0);
    hAngles = vector<float>(NhAngles, 0);
    candela = vector<float>(N, 0);
    for (int i=0; i<NvAngles; i++) ss >> vAngles[i];
    for (int i=0; i<NhAngles; i++) ss >> hAngles[i];
    for (int i=0; i<N; i++) ss >> candela[i];
}

VRTexturePtr VRIES::resample() { // TODO
    auto getMinDelta = [&](vector<float>& v) {
        float d = 1e6;
        for (auto f : v) if (f<d && f > 1e-3) d = f;
        return d;
    };

    int aNv2 = 180.0/getMinDelta(vAngles);
    int aNh2 = 90.0/getMinDelta(hAngles);
    int N2 = aNv2*aNh2;

    texData = vector<float>(N2, 0);
    for (int i=0; i<aNh2; i++) {
        for (int j=0; j<aNv2; j++) {
            float c = 0;

            if (i < NhAngles && j < NvAngles) { // TODO: quick hack, resolve properly
                int k = i*NvAngles + j;
                c = candela[k];
            }

            int k2 = i*aNv2 + j;
            texData[k2] = c;
        }
    }

    if (texData.size() == 0) { cout << "Error, VRIES::resample failed" << endl; return 0; }

    int Nv = aNv2;
    int Nh = aNh2;

    float cMax = 0;
    for (auto& c : texData) if (c > cMax) cMax = c;
    for (auto& c : texData) c /= cMax;

    /*for (int i=0; i<Nv; i++) {
        for (int j=0; j<Nh; j++) {
            int k = i*Nh+j;
            cout << " " << texData[k];
        }
        cout << endl;
    }*/
    auto tex = VRTexture::create();
    //tex->read("imgres.png");
    //tex->read("checkers.jpg");

    tex->setInternalFormat(GL_ALPHA32F_ARB); // important for unclamped float
    auto img = tex->getImage();
    img->set( Image::OSG_A_PF, Nv, Nh, 1, 1, 1, 0, (const uint8_t*)&texData[0], Image::OSG_FLOAT32_IMAGEDATA, true, 1);
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

    return resample();
}

bool VRIES::startswith(const string& a, const string& b) {
    if (a.size() < b.size()) return false;
    return a.compare(0, b.length(), b) == 0;
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

    s += "  N vAngles: " + ::toString(vAngles.size()) + "\n";
    s += "  N hAngles: " + ::toString(hAngles.size()) + "\n";
    s += "  N candela: " + ::toString(candela.size()) + "\n";
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

