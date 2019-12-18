#ifndef VRAMLLOADER_H_INCLUDED
#define VRAMLLOADER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <string>
#include "addons/Engineering/VREngineeringFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRAMLLoader {
    private:

    public:
        VRAMLLoader();
        ~VRAMLLoader();

        static VRAMLLoaderPtr create();

        void read(string path);
        void write(string path);
};

OSG_END_NAMESPACE;

#endif // VRAMLLOADER_H_INCLUDED
