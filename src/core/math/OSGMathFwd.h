#ifndef OSGMATHFWD_H_INCLUDED
#define OSGMATHFWD_H_INCLUDED

#ifndef OSG_BEGIN_NAMESPACE
#define OSG_BEGIN_NAMESPACE namespace OSG {
#define OSG_END_NAMESPACE }
#endif

namespace OSG {

template<class ValueTypeT, unsigned int SizeI> class Vector;

typedef Vector< float, 2 > Vec2f;
typedef Vector< float, 3 > Vec3f;
typedef Vector< float, 4 > Vec4f;

typedef Vector< double, 2 > Vec2d;
typedef Vector< double, 3 > Vec3d;
typedef Vector< double, 4 > Vec4d;

typedef Vector< int, 2 > Vec2i;
typedef Vector< int, 3 > Vec3i;
typedef Vector< int, 4 > Vec4i;

template<class ValueTypeT> class TransformationMatrix;

typedef TransformationMatrix<float> Matrix;
typedef TransformationMatrix<double> Matrix4d;

Vec3d& VEC3D();
Vec3d& DIR();
Vec3d& NDIR();
Vec3d& UP();
Vec3d& NUP();
Vec3d& SCALE();

}

#endif // OSGMATHFWD_H_INCLUDED
