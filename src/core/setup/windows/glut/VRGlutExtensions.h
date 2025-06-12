#ifndef VRGLUTEXTENSIONS_H_INCLUDED
#define VRGLUTEXTENSIONS_H_INCLUDED

#include <stdint.h>
#include <vector>
#include <string>

using namespace std;

struct Icon {
    uint64_t* data = 0;
    int w = 0;
    int h = 0;

    Icon(int w, int h);
};

struct IconList {
    int size = 0;
    uint64_t* data = 0;
    vector<Icon> images;

    uint64_t* add(int w, int h);
    void addTest();
    void load(string path);
    void compile();
    void apply();
};

void initGlutExtensions();
void initGlutDialogExtensions(string name);
void cleanupGlutExtensions();
void setWindowIcon(string path, bool dialog);
void startGrabShiftTab();
void maximizeWindow();

#endif // VRGLUTEXTENSIONS_H_INCLUDED
