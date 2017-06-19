#include "VRSoundUtils.h"
#include "core/utils/toString.h"
#include <AL/al.h>

using namespace std;

string toString(ALenum a) {
    switch (a) {
        case AL_NO_ERROR: return "ok";
        case AL_INVALID_NAME: return "invalid name";
        case AL_INVALID_ENUM: return "invalid enum";
        case AL_INVALID_VALUE: return "invalid value";
        case AL_INVALID_OPERATION: return "invalid operation";
        case AL_OUT_OF_MEMORY: return "out of memory";
    }
    return "unknown";
}
