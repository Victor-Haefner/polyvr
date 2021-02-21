#include "version.h"

// version.h is touched each time
//  this is why the function is defined here

const char* getVersionString() { return " V: " PVR_COMMIT_ID " " PVR_COMMIT_TIME; }
