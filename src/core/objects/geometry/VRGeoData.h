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

        void updateType(int t, int N);

    public:
        VRGeoData();
        VRGeoData(VRGeometryPtr geo);

        int size();

        void reset();
        bool valid();

        int pushVert(Pnt3f p);
        int pushVert(Pnt3f p, Vec3f n);
        int pushVert(Pnt3f p, Vec3f n, Vec3f c);
        int pushVert(Pnt3f p, Vec3f n, Vec4f c);
        int pushVert(Pnt3f p, Vec3f n, Vec2f t);
        int pushVert(Pnt3f p, Vec3f n, Vec3f c, Vec2f t);
        int pushVert(Pnt3f p, Vec3f n, Vec4f c, Vec2f t);

        int pushVert(VRGeoData& other, int i);
        int pushVert(VRGeoData& other, int i, Matrix m);

        void pushPoint(int i = -1);
        void pushTri(int i, int j, int k);
        void pushQuad(int i, int j, int k, int l);

        void apply(VRGeometryPtr geo);
        VRGeometryPtr asGeometry(string name);

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

        int primN(int type);
        int primNOffset(int lID, int type);
        bool setIndices(Primitive& p);
        void pushPrim(Primitive p);
        Primitive* next();

        struct PrimItr : public std::iterator<std::forward_iterator_tag, Primitive*> {
            Primitive* itr;
            VRGeoData* data;

            PrimItr(VRGeoData* d, Primitive* p);

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

        Primitive current;
        PrimItr pend;

        PrimItr begin();
        PrimItr begin() const;
        PrimItr cbegin() const;
        PrimItr end();
        PrimItr end() const;
        PrimItr cend() const;

        string status();
};

OSG_END_NAMESPACE;

#endif // VRGEODATA_H_INCLUDED
