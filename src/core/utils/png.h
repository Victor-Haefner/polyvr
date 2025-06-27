#ifndef PNG_H_INCLUDED
#define PNG_H_INCLUDED

#include <vector>
#include <string>

using namespace std;

class VRImage {
    public:
        vector<unsigned char> pixels;
        int channels = 4;
        int width = 0;
        int height = 0;

    public:
        void setup(int w, int h, int c);
        void resize(int w, int h);
};

void readPNG(string path, VRImage& img);
void writePNG(string path, VRImage& img);

#endif // PNG_H_INCLUDED
