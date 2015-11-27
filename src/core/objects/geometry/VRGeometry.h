#ifndef VRGEOMETRY_H_INCLUDED
#define VRGEOMETRY_H_INCLUDED

#include "core/objects/VRObjectFwd.h"
#include "core/tools/selection/VRSelectionFwd.h"
#include "../VRTransform.h"
#include <OpenSG/OSGFieldContainerFields.h>
#include <OpenSG/OSGImage.h> // TODO

struct VRPrimitive;

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRMaterial;
class Node; OSG_GEN_CONTAINERPTR(Node);
//class Image; OSG_GEN_CONTAINERPTR(Image);
class Geometry; OSG_GEN_CONTAINERPTR(Geometry);
class Material; OSG_GEN_CONTAINERPTR(Material);

class GeoVectorProperty;
class GeoIntegralProperty;

class VRGeometry : public VRTransform {
    public:
        enum { CODE, SCRIPT, FILE, PRIMITIVE };

        struct Reference {
            int type;
            string parameter;
        };

    protected:
        VRMaterialPtr mat = 0;
        VRPrimitive* primitive = 0;
        GeometryRecPtr mesh;
        NodeRecPtr mesh_node;
        ImageRecPtr texture;
        bool meshSet = false;

        map<string, VRGeometryPtr> dataLayer;

        Reference source;

        VRObjectPtr copy(vector<VRObjectPtr> children);

        virtual void saveContent(xmlpp::Element* e);
        virtual void loadContent(xmlpp::Element* e);

        VRGeometry(string name, bool hidden);
        static VRGeometryPtr create(string name, bool hidden);

    public:
        VRGeometry(string name = "0");
        virtual ~VRGeometry();

        static VRGeometryPtr create(string name);
        static VRGeometryPtr create(string name, string primitive, string params);
        VRGeometryPtr ptr();

        /** Set the geometry mesh (OSG geometry core) **/
        void setMesh(GeometryRecPtr g);
        void setMesh(GeometryRecPtr g, Reference ref, bool keep_material = false);

        Reference getReference();
        void makeUnique();
        void setMeshVisibility(bool b);

        void setPrimitive(string primitive, string args = "");

        /** Create a mesh using vectors with positions, normals, indices && optionaly texture coordinates **/
        void create(int type, vector<Vec3f> pos, vector<Vec3f> norms, vector<int> inds, vector<Vec2f> texs = vector<Vec2f>());
        void create(int type, GeoVectorProperty* pos, GeoVectorProperty* norms, GeoIntegralProperty* inds, GeoVectorProperty* texs);

        /** Overwrites the vertex positions of the mesh **/
        void setType(int t);
        void setTypes(GeoIntegralProperty* types);
        void setPositions(GeoVectorProperty* Pos);
        void setNormals(GeoVectorProperty* Norms);
        void setColors(GeoVectorProperty* Colors, bool fixMapping = false);
        void setIndices(GeoIntegralProperty* Indices);
        void setTexCoords(GeoVectorProperty* Tex, int i=0, bool fixMapping = false);
        void setLengths(GeoIntegralProperty* lenghts);
        void setPositionalTexCoords(float scale = 1.0);

        void setRandomColors();
        void removeDoubles(float minAngle);
        void decimate(float f);
        void merge(VRGeometryPtr geo);
        void removeSelection(VRSelectionPtr sel);
        VRGeometryPtr copySelection(VRSelectionPtr sel);
        VRGeometryPtr separateSelection(VRSelectionPtr sel);
        void fixColorMapping();
        void updateNormals();

        void genTexCoords(string mapping = "CUBE", float scale = 1, int channel = 0, std::shared_ptr<pose> p = 0);

        void showGeometricData(string type, bool b);
        float calcSurfaceArea();

        Vec3f getGeometricCenter();
        Vec3f getAverageNormal();

        float getMax(int axis);
        float getMin(int axis);

        /** Returns the mesh as a OSG geometry core **/
        GeometryRecPtr getMesh();
        VRPrimitive* getPrimitive();

        /** Set the material of the mesh **/
        void setMaterial(VRMaterialPtr mat = 0);
        void setMaterial(MaterialRecPtr mat);

        /** Returns the mesh material **/
        VRMaterialPtr getMaterial();

        /** Returns the texture || 0 **/
        ImageRecPtr getTexture() { return texture; }

        void influence(vector<Vec3f> pnts, vector<Vec3f> values, int power, float color_code = -1, float dl_max = 1.0);
};

OSG_END_NAMESPACE;

#endif // VRGEOMETRY_H_INCLUDED
