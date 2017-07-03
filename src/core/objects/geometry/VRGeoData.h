#ifndef VRGEODATA_H_INCLUDED
#define VRGEODATA_H_INCLUDED

#include "core/objects/VRObjectFwd.h"
#include <OpenSG/OSGVector.h>
#include <OpenSG/OSGMatrix.h>
#include <OpenSG/OSGConfig.h>

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
        void updateType(int t, int N);

    public:
        VRGeoData();
        VRGeoData(VRGeometryPtr geo);

        static VRGeoDataPtr create();

        int size() const;

        void reset();
        bool valid() const;

        Pnt3f getPosition(int i);
        Vec3f getNormal(int i);
        Vec4f getColor(int i);
        int getNIndices();

        int pushVert(Pnt3f p);
        int pushVert(Pnt3f p, Vec3f n);
        int pushVert(Pnt3f p, Vec3f n, Vec3f c);
        int pushVert(Pnt3f p, Vec3f n, Vec4f c);
        int pushVert(Pnt3f p, Vec3f n, Vec2f t);
        int pushVert(Pnt3f p, Vec3f n, Vec2f t, Vec2f t2);
        int pushVert(Pnt3f p, Vec3f n, Vec3f c, Vec2f t);
        int pushVert(Pnt3f p, Vec3f n, Vec4f c, Vec2f t);
        int pushVert(Pnt3f p, Vec3f n, Vec3f c, Vec2f t, Vec2f t2);
        int pushVert(Pnt3f p, Vec3f n, Vec4f c, Vec2f t, Vec2f t2);

        int pushColor(Vec3f c);
        int pushColor(Vec4f c);

        bool setVert(int i, Pnt3f p);
        bool setVert(int i, Pnt3f p, Vec3f n);
        bool setVert(int i, Pnt3f p, Vec3f n, Vec3f c);
        bool setVert(int i, Pnt3f p, Vec3f n, Vec4f c);
        bool setVert(int i, Pnt3f p, Vec3f n, Vec2f t);
        bool setVert(int i, Pnt3f p, Vec3f n, Vec2f t, Vec2f t2);
        bool setVert(int i, Pnt3f p, Vec3f n, Vec3f c, Vec2f t);
        bool setVert(int i, Pnt3f p, Vec3f n, Vec4f c, Vec2f t);
        bool setVert(int i, Pnt3f p, Vec3f n, Vec3f c, Vec2f t, Vec2f t2);
        bool setVert(int i, Pnt3f p, Vec3f n, Vec4f c, Vec2f t, Vec2f t2);

        int pushVert(const VRGeoData& other, int i);
        int pushVert(const VRGeoData& other, int i, Matrix m);

        void pushPoint(int i = -1);
        void pushLine(int i, int j);
        void pushTri(int i, int j, int k);
        void pushQuad(int i, int j, int k, int l);

        void pushLine();
        void pushTri();
        void pushQuad();
        void pushPatch(int N);

        void pushQuad(Vec3f p, Vec3f n, Vec3f u, Vec2f s, bool addInds = false);

        void apply(VRGeometryPtr geo, bool check = true) const;
        VRGeometryPtr asGeometry(string name) const;
        void append(VRGeometryPtr geo, const Matrix& m = Matrix());
        void append(const VRGeoData& geo, const Matrix& m = Matrix());

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

        void addVertexColors(Vec3f c);

        string status();
        void test_copy(VRGeoData& g);
};

OSG_END_NAMESPACE;

#endif // VRGEODATA_H_INCLUDED
