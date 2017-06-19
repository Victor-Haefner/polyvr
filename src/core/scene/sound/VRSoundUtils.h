#ifndef VRSOUNDUTILS_H_INCLUDED
#define VRSOUNDUTILS_H_INCLUDED

#include <string>

typedef int ALenum;
std::string toString(ALenum a);

#define ALCHECK(x) { \
x; \
ALenum error = alGetError(); \
if (error != AL_NO_ERROR) { \
        fprintf(stderr, "\nRuntime error: %s got %s at %s:%d", #x, toString(error).c_str(), __FILE__, __LINE__); \
} }

#define ALCHECK_BREAK(x) { \
x; \
ALenum error = alGetError(); \
if (error != AL_NO_ERROR) { \
        fprintf(stderr, "\nRuntime error: %s got %s at %s:%d", #x, toString(error).c_str(), __FILE__, __LINE__); \
        break; \
} }

#endif // VRSOUNDUTILS_H_INCLUDED
