#ifndef VRGEODATA_H_INCLUDED
#define VRGEODATA_H_INCLUDED

#include "core/objects/VRObjectFwd.h"
#include <OpenSG/OSGVector.h>
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

        void pushPoint(int i = -1);
        void pushTri(int i, int j, int k);
        void pushQuad(int i, int j, int k, int l);

        void apply(VRGeometryPtr geo);
        VRGeometryPtr asGeometry(string name);

        // primitive iterator
        struct Primitive {
            int type = 0;
            int tID = 0;
            int lID = 0;
            vector<int> indices;

            bool same(const Primitive& p) {
                if (p.tID != tID) return false;
                if (p.lID != lID) return false;
                return true;
            }
        };

        int primN(int type);
        void setIndices(Primitive& p);
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
};

/*


template
<
    class Type,
    class UnqualifiedType = std::remove_cv_t<Type>
>
class ForwardIterator
    : public std::iterator<std::forward_iterator_tag,
                           UnqualifiedType,
                           std::ptrdiff_t,
                           Type*,
                           Type&>
{
    node<UnqualifiedType>* itr;

    explicit ForwardIterator(node<UnqualifiedType>* nd)
        : itr(nd)
    {
    }

public:

    ForwardIterator()   // Default construct gives end.
        : itr(nullptr)
    {
    }

    void swap(ForwardIterator& other) noexcept
    {
        using std::swap;
        swap(itr, other.iter);
    }

    ForwardIterator& operator++ () // Pre-increment
    {
        assert(itr != nullptr && "Out-of-bounds iterator increment!");
        itr = itr->next;
        return *this;
    }

    ForwardIterator operator++ (int) // Post-increment
    {
        assert(itr != nullptr && "Out-of-bounds iterator increment!");
        ForwardIterator tmp(*this);
        itr = itr->next;
        return tmp;
    }

    // two-way comparison: v.begin() == v.cbegin() and vice versa
    template<class OtherType>
    bool operator == (const ForwardIterator<OtherType>& rhs) const
    {
        return itr == rhs.itr;
    }

    template<class OtherType>
    bool operator != (const ForwardIterator<OtherType>& rhs) const
    {
        return itr != rhs.itr;
    }

    Type& operator* () const
    {
        assert(itr != nullptr && "Invalid iterator dereference!");
        return itr->data;
    }

    Type& operator-> () const
    {
        assert(itr != nullptr && "Invalid iterator dereference!");
        return itr->data;
    }

    // One way conversion: iterator -> const_iterator
    operator ForwardIterator<const Type>() const
    {
        return ForwardIterator<const Type>(itr);
    }
};

// `iterator` and `const_iterator` used by your class:
typedef ForwardIterator<T> iterator;
typedef ForwardIterator<const T> const_iterator;

*/


/*class const_iterator : public std::iterator<random_access_iterator_tag, Type>{
    typename std::vector<Type>::iterator m_data;
    const_iterator(typename std::vector<Type>::iterator data) :m_data(data) {}
public:
    const_iterator() :m_data() {}
    const_iterator(const const_iterator& rhs) :m_data(rhs.m_data) {}
     //const iterator implementation code
};


template< typename Type>
class SomeSortedContainer{

    std::vector<Type> m_data; //we wish to iterate over this
    //container implementation code
public:
    typedef typename std::vector<Type>::iterator iterator;
    typedef typename std::vector<Type>::const_iterator const_iterator;

    iterator begin() {return m_data.begin();}
    const_iterator begin() const {return m_data.begin();}
    const_iterator cbegin() const {return m_data.cbegin();}
    iterator end() {return m_data.end();}
    const_iterator end() const {return m_data.end();}
    const_iterator cend() const {return m_data.cend();}
};*/



OSG_END_NAMESPACE;

#endif // VRGEODATA_H_INCLUDED
