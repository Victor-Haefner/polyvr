#ifndef VRMATERIALT_H_INCLUDED
#define VRMATERIALT_H_INCLUDED

#include "VRMaterial.h"
#include <OpenSG/OSGSimpleSHLChunk.h>

template<typename T>
void OSG::VRMaterial::setShaderParameter(string name, const T &value) {
    //shaderChunk->updateUniformVariable(name.c_str(), value);
    //shaderChunk->subUniformVariable(name.c_str());
    //shaderChunk->addUniformVariable(name.c_str(), value);
    auto p = getShaderProgram();
    if (!p) cout << "Warning! setShaderParameter failed for parameter " << name << endl;
    if (p) p->subUniformVariable(name.c_str());
    if (p) p->addUniformVariable(name.c_str(), value);
}

#endif // VRMATERIALT_H_INCLUDED
