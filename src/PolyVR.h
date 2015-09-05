#ifndef VRFRAMEWORK_H_INCLUDED
#define VRFRAMEWORK_H_INCLUDED

#include <OpenSG/OSGConfig.h>

OSG_BEGIN_NAMESPACE;

class Node;

using namespace std;

void setMultisampling(bool on);
void initPolyVR(int argc, char **argv);

void startPolyVR();
void exitPolyVR();

void startPolyVR_testScene(Node* n);

OSG_END_NAMESPACE;

#endif // VRFRAMEWORK_H_INCLUDED
