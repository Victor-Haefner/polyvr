#ifndef VRabq_H
#define VRabq_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <math.h>

#include "struct.h" // Nodes, Elements, vec, frame
#include "core/utils/toString.h"


using namespace std;
OSG_BEGIN_NAMESPACE;

class VRabq {
    protected:

        vector<frame*> frames;

        /**
        * Returns a frame from a given binary data.
        */
        frame* loadBin(string path) {
            ifstream fin(path.c_str(), ios::in | ios::binary);

            int N = 0;
            fin.read((char*)&N, sizeof(int));
            vector<vec>* pos = new vector<vec>(N);
            fin.read((char*)(&(*pos)[0]), N*sizeof(vec));

            fin.read((char*)&N, sizeof(int));
            vector<int>* ind = new vector<int>(N);
            fin.read((char*)(&(*ind)[0]), N*sizeof(int));

            fin.read((char*)&N, sizeof(int));
            vector<vec>* norm = new vector<vec>(N);
            fin.read((char*)(&(*norm)[0]), N*sizeof(vec));

            fin.read((char*)&N, sizeof(int));
            vector<float>* stress = new vector<float>(N);
            fin.read((char*)(&(*stress)[0]), N*sizeof(float));

            fin.read((char*)&N, sizeof(int));
            vector<float>* strain = new vector<float>(N);
            fin.read((char*)(&(*strain)[0]), N*sizeof(float));

            frame* f = new frame();
            f->pos = pos;
            f->ind = ind;
            //cout << "Ind size: " << f->ind->size() << endl;
            f->norm = norm;
            f->stress = stress;
            f->strain = strain;
            return f;
        }

        /**
        * Writes all point's coordinates from a frame into a given path of ASCII file.
        */
        void writeFramePos(frame* f, string const path ){

                ofstream outputThread(path.c_str());

                for (unsigned int i = 0 ; i < f->pos->size(); i++){
                  outputThread << i+1 << " Node = " << f->pos->at(i).x[0] <<" "<< f->pos->at(i).x[1] <<" "<< f->pos->at(i).x[2]  << endl;
                }

                outputThread.close();
        }

        /**
        * Writes all Stress informations from a frame into a given path of ASCII file.
        */
        void writeFrameStress(frame* f, string const path ){

                ofstream outputThread(path.c_str());

                for (unsigned int i = 0 ; i < f->stress->size(); i++){
                    if(i == 0)
                        outputThread << "Max Scala Stress: " << f->stress->at(i) << endl;
                    else

                        outputThread << i+1 << " Element = " << i << ", Stress: "<<  (float)f->stress->at(i) <<  endl;  // <<
                }

                outputThread.close();
        }

    public:
        VRabq() {
            ;
        }

        void loadAbaqusFile(int from, int to) {
            frames = vector<frame*>(to-from);
            string path = "abacusTest/aba_frame";
            for (int i=from;i<=to;i++) {
                string p = path + toString(i+1) + ".dat";
                cout << "\nLOAD " << p << flush;
                frames[i-from] = loadBin(p);
                frames[i-from]->id = i;
            }
        }
};
OSG_END_NAMESPACE
#endif // VRabq_H
