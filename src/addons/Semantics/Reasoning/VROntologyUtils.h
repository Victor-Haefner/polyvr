#ifndef VRONTOLOGYUTILS_H_INCLUDED
#define VRONTOLOGYUTILS_H_INCLUDED

#include <string>

using namespace std;

#ifndef WITHOUT_GTK

#include "core/gui/VRGuiManager.h"
#include "core/gui/VRGuiConsole.h"

#define WARN(x) \
VRGuiManager::get()->getConsole( "Errors" )->write( x+"\n" );

#else

#define WARN(x) \
cout << x << endl;

#endif

int guid();

struct VROntoID {
    int ID;
    VROntoID();
};

#endif
