#ifndef VRMATERIALT_H_INCLUDED
#define VRMATERIALT_H_INCLUDED

#include "VRMaterial.h"
#include <OpenSG/OSGSimpleSHLChunk.h>

template<typename T>
void OSG::VRMaterial::setShaderParameter(string name, const T &value) {
    ShaderProgramMTRecPtr p = getShaderProgram();
    if (!p) { cout << "Warning! setShaderParameter failed for parameter " << name << endl; return; }
    T t;
    if (p->getUniformVariable(name.c_str(), t)) p->updateUniformVariable(name.c_str(), value);
    else p->addUniformVariable(name.c_str(), value);
}

#endif // VRMATERIALT_H_INCLUDED
