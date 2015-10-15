#ifndef VRPYMATERIAL_H_INCLUDED
#define VRPYMATERIAL_H_INCLUDED

#include "core/objects/material/VRMaterial.h"
#include "VRPyBase.h"

struct VRPyMaterial : VRPyBaseT<OSG::VRMaterial> {
    static PyMemberDef members[];
    static PyMethodDef methods[];

    static PyObject* addPass(VRPyMaterial* self);
    static PyObject* remPass(VRPyMaterial* self, PyObject* args);
    static PyObject* setActivePass(VRPyMaterial* self, PyObject* args);

    static PyObject* setLit(VRPyMaterial* self, PyObject* args);
    static PyObject* getAmbient(VRPyMaterial* self);
    static PyObject* setAmbient(VRPyMaterial* self, PyObject* args);
    static PyObject* getDiffuse(VRPyMaterial* self);
    static PyObject* setDiffuse(VRPyMaterial* self, PyObject* args);
    static PyObject* getSpecular(VRPyMaterial* self);
    static PyObject* setSpecular(VRPyMaterial* self, PyObject* args);
    static PyObject* getTransparency(VRPyMaterial* self);
    static PyObject* setTransparency(VRPyMaterial* self, PyObject* args);
    static PyObject* getShininess(VRPyMaterial* self);
    static PyObject* setShininess(VRPyMaterial* self, PyObject* args);
    static PyObject* setTexture(VRPyMaterial* self, PyObject* args);
    static PyObject* setTextureType(VRPyMaterial* self, PyObject* args);
    static PyObject* setStencilBuffer(VRPyMaterial* self, PyObject* args);

    static PyObject* setPointSize(VRPyMaterial* self, PyObject* args);
    static PyObject* setLineWidth(VRPyMaterial* self, PyObject* args);
    static PyObject* setWireFrame(VRPyMaterial* self, PyObject* args);
    static PyObject* setZOffset(VRPyMaterial* self, PyObject* args);
    static PyObject* setSortKey(VRPyMaterial* self, PyObject* args);

    static PyObject* setQRCode(VRPyMaterial* self, PyObject* args);

    static PyObject* setMagMinFilter(VRPyMaterial* self, PyObject* args);
    static PyObject* setVertexProgram(VRPyMaterial* self, PyObject* args);
    static PyObject* setFragmentProgram(VRPyMaterial* self, PyObject* args);
    static PyObject* setGeometryProgram(VRPyMaterial* self, PyObject* args);
    static PyObject* setShaderParameter(VRPyMaterial* self, PyObject* args);
    static PyObject* setDefaultVertexShader(VRPyMaterial* self);
};

#endif // VRPYMATERIAL_H_INCLUDED
