#ifndef VRIMGUICONSOLES_H_INCLUDED
#define VRIMGUICONSOLES_H_INCLUDED

#include "VRImguiUtils.h"

using namespace std;

class ImConsole {
    public:
        bool sensitive = true;
        string ID;
        string name;
        string data;

        ImConsole() {}
        ImConsole(string ID);
        void render();
};

class ImConsoles : public ImWidget {
    public:
        vector<string> consolesOrder;
        map<string,ImConsole> consoles;

        void newConsole(string ID);
        void clearConsole(string ID);
        void setupConsole(string ID, string name);
        void pushConsole(string ID, string data);

        ImConsoles();
        void begin() override;
};

#endif // VRIMGUICONSOLES_H_INCLUDED
