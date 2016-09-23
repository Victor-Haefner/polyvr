#include "OSGMaterial.h"

using namespace OSG;

OSGMaterial::OSGMaterial(MultiPassMaterialMTRecPtr m) : mat(m) {}
OSGMaterialPtr OSGMaterial::create(MultiPassMaterialMTRecPtr mat) { return OSGMaterialPtr( new OSGMaterial(mat) ); }
