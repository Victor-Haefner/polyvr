#ifndef VRGEOMETRY_H_INCLUDED
#define VRGEOMETRY_H_INCLUDED

#include "../VRTransform.h"
#include <OpenSG/OSGFieldContainerFields.h>
#include <OpenSG/OSGImage.h> // TODO

class VRPrimitive;

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
        //VRObject* geonode;
        VRMaterial* mat = 0;
        VRPrimitive* primitive = 0;
        GeometryRecPtr mesh;
        NodeRecPtr mesh_node;
        ImageRecPtr texture;
        bool meshSet = false;

        Reference source;

        VRObject* copy(vector<VRObject*> children);

        virtual void saveContent(xmlpp::Element* e);
        virtual void loadContent(xmlpp::Element* e);

    public:

        /** initialise a geometry object with his name **/
        VRGeometry(string name = "0");
        virtual ~VRGeometry();

        /** Set the geometry mesh (OSG geometry core) **/
        void setMesh(GeometryRecPtr g);
        void setMesh(GeometryRecPtr g, Reference ref, bool keep_material = false);

        Reference getReference();
        void makeUnique();

        void setPrimitive(string primitive, string args = "");

        /** Create a mesh using vectors with positions, normals, indices and optionaly texture coordinates **/
        void create(int type, vector<Vec3f> pos, vector<Vec3f> norms, vector<int> inds, vector<Vec2f> texs = vector<Vec2f>());
        void create(int type, GeoVectorProperty* pos, GeoVectorProperty* norms, GeoIntegralProperty* inds, GeoVectorProperty* texs);

        /** Overwrites the vertex positions of the mesh **/
        void setType(int t);
        void setTypes(GeoIntegralProperty* types);
        void setPositions(GeoVectorProperty* Pos);
        void setNormals(GeoVectorProperty* Norms);
        void setColors(GeoVectorProperty* Colors);
        void setIndices(GeoIntegralProperty* Indices);
        void setTexCoords(GeoVectorProperty* Tex, int i=0);
        void setLengths(GeoIntegralProperty* lenghts);

        void setRandomColors();
        void removeDoubles(float minAngle);
        void decimate(float f);

        /** Returns the geometric center of the mesh **/
        Vec3f getGeometricCenter();

        /** Returns the average of all normals of the mesh (not normalized, can be zero) **/
        Vec3f getAverageNormal();

        /** Returns the maximum position on the x, y or z axis **/
        float getMax(int axis);

        /** Returns the minimum position on the x, y or z axis **/
        float getMin(int axis);

        /** Returns the mesh as a OSG geometry core **/
        GeometryRecPtr getMesh();
        VRPrimitive* getPrimitive();

        /** Set the material of the mesh **/
        void setMaterial(VRMaterial* mat = 0);
        void setMaterial(MaterialRecPtr mat);

        /** Returns the mesh material **/
        VRMaterial* getMaterial();

        /** Returns the texture or 0 **/
        ImageRecPtr getTexture() { return texture; }
};

OSG_END_NAMESPACE;

#endif // VRGEOMETRY_H_INCLUDED
