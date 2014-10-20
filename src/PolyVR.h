#ifndef VRFRAMEWORK_H_INCLUDED
#define VRFRAMEWORK_H_INCLUDED

#include <OpenSG/OSGConfig.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

void setMultisampling(bool on);
void initPolyVR(int argc, char **argv);

void startPolyVR();
void exitPolyVR();

OSG_END_NAMESPACE;

#endif // VRFRAMEWORK_H_INCLUDED
