#include "VRDoublebuffer.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

doubleBuffer::doubleBuffer() {
    t1 = new Matrix4d();
    t2 = new Matrix4d();

    read1 = read2 = write2 = false;
    write1 = true;

    reading = writing = newest = false;
}

doubleBuffer::~doubleBuffer() {
    delete t1;
    delete t2;
}

void doubleBuffer::read(Matrix4d& result) {
    reading = true;
    if(writing) {
        if (write1) {
            read2 = true;
            result = *t2;
            read2 = false;
            reading = false;
            return;
        }
        if (write2) {
            read1 = true;
            result = *t1;
            read1 = false;
            reading = false;
            return;
        }
    } else {
        if (newest) {
            read2 = true;
            result = *t2;
            read2 = false;
        }
        else {
            read1 = true;
            result = *t1;
            read1 = false;
        }
        reading = false;
        return;
    }
}
void doubleBuffer::write(Matrix4d m) {
    //static int i=0;i++;
    //m[0][0] = i;cout << "\n write : " << i;
    writing = true;
    if(reading) {
        if (read1) {
            write2 = true;
            *t2 = m;
            newest = true;
            write2 = false;
            writing = false;
            return;
        }
        if (read2) {
            write1 = true;
            *t1 = m;
            newest = false;
            write1 = false;
            writing = false;
            return;
        }
    } else {
        if (newest) {
            write1 = true;
            *t1 = m;
            write1 = false;
        } else {
            write2 = true;
            *t2 = m;
            write2 = false;
        }
        newest = !newest;
        writing = false;
        return;
    }
}

OSG_END_NAMESPACE;
