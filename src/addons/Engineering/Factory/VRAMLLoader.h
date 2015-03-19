#ifndef VRAMLLOADER_H_INCLUDED
#define VRAMLLOADER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include <map>
#include <vector>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRObject;

class VRAMLLoader {
    private:

        VRAMLLoader();

    public:
        static VRAMLLoader* get();

        VRObject* load(string path);
};

OSG_END_NAMESPACE;

#endif // VRAMLLOADER_H_INCLUDED
