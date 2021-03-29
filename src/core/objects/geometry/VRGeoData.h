#ifndef VRGEODATA_H_INCLUDED
#define VRGEODATA_H_INCLUDED

static const int PositionsIndex       = 0;
static const int NormalsIndex         = 2;
static const int ColorsIndex          = 3;
static const int SecondaryColorsIndex = 4;
static const int TexCoordsIndex       = 8;
static const int TexCoords1Index      = 9;
static const int TexCoords2Index      = 10;
static const int TexCoords3Index      = 11;
static const int TexCoords4Index      = 12;
static const int TexCoords5Index      = 13;
static const int TexCoords6Index      = 14;
static const int TexCoords7Index      = 15;

#include "core/objects/VRObjectFwd.h"
#include "core/math/OSGMathFwd.h"
#include <OpenSG/OSGMatrix.h>
#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGColor.h>

#include <iterator>     // iterator
#include <type_traits>  // remove_cv
#include <utility>      // swap

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRGeoData {
    private:
        struct Data;
        shared_ptr<Data> data;
        VRGeometryPtr geo;

        bool isStripOrFan(int t);
        void extentType(int N);

    public:
        VRGeoData();
        VRGeoData(VRGeometryPtr geo);

        static VRGeoDataPtr create();

        int size() const;
        int sizeNormals() const;
        int getNFaces() const;
        int getFaceSize(int fID) const;
        int getNTypes() const;

        void reset();
        bool valid() const;
        bool validIndices() const;

        int getType(int i);
        int getLength(int i);
        int getIndex(int i, int a = PositionsIndex);
        Pnt3d getPosition(int i);
        Vec3d getNormal(int i);
        Color4f getColor(int i);
        Color3f getColor3(int i);
        int getNIndices();
        string getDataName(int type);
        int getDataSize(int type);
        string getDataAsString(int type);

        int pushVert(Pnt3d p);
        int pushVert(Pnt3d p, Vec3d n);
        int pushVert(Pnt3d p, Vec3d n, Color3f c);
        int pushVert(Pnt3d p, Vec3d n, Color4f c);
        int pushVert(Pnt3d p, Vec3d n, Vec2d t);
        int pushVert(Pnt3d p, Vec3d n, Vec2d t, Vec2d t2);
        int pushVert(Pnt3d p, Vec3d n, Color3f c, Vec2d t);
        int pushVert(Pnt3d p, Vec3d n, Color4f c, Vec2d t);
        int pushVert(Pnt3d p, Vec3d n, Color3f c, Vec2d t, Vec2d t2);
        int pushVert(Pnt3d p, Vec3d n, Color4f c, Vec2d t, Vec2d t2);

        void updateType(int t, int N);

        int pushType(int t);
        int pushLength(int l);
        int pushIndex(int i);
        int pushPos(Pnt3d p);
        int pushNorm(Vec3d n);
        int pushTexCoord(Vec2d t);
        int pushTexCoord2(Vec2d t);
        int pushColor(Color3f c);
        int pushColor(Color4f c);
        int pushNormalIndex(int i);
        int pushColorIndex(int i);
        int pushTexCoordIndex(int i);

        bool setVert(int i, Pnt3d p);
        bool setVert(int i, Pnt3d p, Vec3d n);
        bool setVert(int i, Pnt3d p, Vec3d n, Color3f c);
        bool setVert(int i, Pnt3d p, Vec3d n, Color4f c);
        bool setVert(int i, Pnt3d p, Vec3d n, Vec2d t);
        bool setVert(int i, Pnt3d p, Vec3d n, Vec2d t, Vec2d t2);
        bool setVert(int i, Pnt3d p, Vec3d n, Color3f c, Vec2d t);
        bool setVert(int i, Pnt3d p, Vec3d n, Color4f c, Vec2d t);
        bool setVert(int i, Pnt3d p, Vec3d n, Color3f c, Vec2d t, Vec2d t2);
        bool setVert(int i, Pnt3d p, Vec3d n, Color4f c, Vec2d t, Vec2d t2);

        bool setType(int i, int t);
        bool setLength(int i, int l);
        bool setIndex(int i, int I);
        bool setPos(int i, Pnt3d p);
        bool setNorm(int i, Vec3d n);
        bool setTexCoord(int i, Vec2d t);
        bool setTexCoord2(int i, Vec2d t);
        bool setColor(int i, Color3f c);
        bool setColor(int i, Color4f c);

        int pushVert(const VRGeoData& other, int i);
        int pushVert(const VRGeoData& other, int i, Matrix4d m);

        void pushPoint(int i = -1);
        void pushLine(int i = -2, int j = -1);
        void pushTri(int i = -3, int j = -2, int k = -1);
        void pushQuad(int i = -4, int j = -3, int k = -2, int l = -1);

        void pushPatch(int N);
        void pushQuad(Vec3d p, Vec3d n, Vec3d u, Vec2d s, bool addInds = false);

        void apply(VRGeometryPtr geo, bool check = true, bool checkIndices = false) const;
        VRGeometryPtr asGeometry(string name) const;
        vector<VRGeometryPtr> splitByVertexColors(const Matrix4d& m = Matrix4d());
        void append(VRGeometryPtr geo, const Matrix4d& m = Matrix4d());
        void append(const VRGeoData& geo, const Matrix4d& m = Matrix4d());
        void makeSingleIndex();

        // primitive iterator
        struct Primitive {
            int type = 0;
            int tid = 0;
            int lid = 0;
            int tID = 0;
            int lID = 0;
            int pID = 0;
            vector<int> indices;

            bool same(const Primitive& p) {
                if (p.tID != tID) return false;
                if (p.lID != lID) return false;
                return true;
            }

            string asString();
        };

        int primN(int type) const;
        int primNOffset(int lID, int type) const;
        bool setIndices(Primitive& p) const ;
        void pushPrim(Primitive p);
        Primitive* next() const;

        struct PrimItr : public std::iterator<std::forward_iterator_tag, Primitive*> {
            Primitive* itr;
            const VRGeoData* data;

            PrimItr(const VRGeoData* d, Primitive* p);

            PrimItr& operator++() { itr = data->next(); return *this; }
            PrimItr operator++(int) { itr = data->next(); return *this; }
            bool operator== (const PrimItr& p) {
                if (p.itr == 0 && itr == 0) return true;
                if (p.itr == 0) return false;
                return itr->same(*p.itr);
            }
            bool operator!= (const PrimItr& p)  {
                if (p.itr == 0 && itr == 0) return false;
                if (p.itr == 0) return true;
                return !itr->same(*p.itr);
            }
            Primitive& operator*() { return *itr; }
        };

        mutable Primitive current;
        PrimItr pend;

        PrimItr begin();
        PrimItr begin() const;
        PrimItr cbegin() const;
        PrimItr end();
        PrimItr end() const;
        PrimItr cend() const;

        void addVertexColors(Color3f c);
        void addVertexColors(Color4f c);

        string status();
        void test_copy(VRGeoData& g);
};

OSG_END_NAMESPACE;

#endif // VRGEODATA_H_INCLUDED
