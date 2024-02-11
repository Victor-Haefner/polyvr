#ifndef VRIMGUISCENENETWORK_H_INCLUDED
#define VRIMGUISCENENETWORK_H_INCLUDED

#include "VRImguiUtils.h"

using namespace std;

class ImNetwork {
    private:
        struct Node {
            string name;
            float x = 0;
            float y = 0;
        };

        struct Flow {
            int w = 0;
            int h = 0;
            float x = 0;
            float y = 0;
            vector<double> curve;
        };

        map<string, Node> nodes;
        map<string, Flow> flows;

        void clear();
        void addFlow(string ID, int w, int h);
        void addNode(string ID, string name);
        void resizeFlow(string ID, int w, int h);
        void placeWidget(string ID, float x, float y);
        void setCurve(string ID, string curveData);

    public:
        ImNetwork();
        void render();
};

#endif // VRIMGUISCENENETWORK_H_INCLUDED
