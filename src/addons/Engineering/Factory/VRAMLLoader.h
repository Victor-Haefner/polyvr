#ifndef VRAMLLOADER_H_INCLUDED
#define VRAMLLOADER_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGGeometry.h>
#include <string>
#include <map>
#include <vector>

#include <boost/property_tree/ptree.hpp>


OSG_BEGIN_NAMESPACE;
using namespace std;

class VRObject;

class VRAMLLoader {
    private:

        VRAMLLoader();

        void buildMesh(string path, Matrix4f m);
        void print(boost::property_tree::ptree const& pt, Matrix4f m);
        void loadProducts(string path, Matrix4f m);

    public:
        static VRAMLLoader* get();

        VRObject* load(string path);
};

OSG_END_NAMESPACE;

#endif // VRAMLLOADER_H_INCLUDED
