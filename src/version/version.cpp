#include "version.h"

// version.h is touched each time
//  this is why the function is defined here

const char* getVersionString() { return PVR_COMMIT_ID "\n" PVR_COMMIT_TIME; }
