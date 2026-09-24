#ifndef VRIMGUICOMBO_H_INCLUDED
#define VRIMGUICOMBO_H_INCLUDED

#include <imgui.h>
#include <string>
#include <vector>

using namespace std;

class ImCombo {
    public:
        string ID;
        string label;
        int flags = 0;

        int current = 0;
        vector<string> strings;
        vector<const char*> cstrings;

    public:
        ImCombo(string ID, string label, int flags = 0);

        bool render(int width);
        void signal(string s);

        void setList(vector<string> v);
        void setList(string v);
        void appendList(string s);
        void clearList();

        void set(string s);
        string get();
};

#endif // VRIMGUICOMBO_H_INCLUDED
