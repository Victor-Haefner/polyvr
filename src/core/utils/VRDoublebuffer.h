#ifndef OSGDOUBLEBUFFER_H_INCLUDED
#define OSGDOUBLEBUFFER_H_INCLUDED

#include <OpenSG/OSGMatrix.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class doubleBuffer {
    private:

    Matrix4d t1;
    Matrix4d t2;

    bool read1;
    bool read2;
    bool write1;
    bool write2;

    bool reading;
    bool writing;
    bool newest;

    public:

    doubleBuffer();

    ~doubleBuffer();

    void read(Matrix4d& result);

    void write(Matrix4d m);
};

OSG_END_NAMESPACE;

#endif // OSGDOUBLEBUFFER_H_INCLUDED
