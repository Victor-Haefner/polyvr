#include "VRPyRendering.h"
#include "VRPyBaseT.h"

#include "core/scene/VRScene.h"

using namespace OSG;

simpleVRPyType(Rendering, 0);

PyMethodDef VRPyRendering::methods[] = {
    {"addStage", (PyCFunction)VRPyRendering::addStage, METH_VARARGS, "Add a stage to the rendering pipeline - addStage( str name, str insert_point )\n\ta common insertion point is the 'shading' stage" },
    {"setStageShader", (PyCFunction)VRPyRendering::setStageShader, METH_VARARGS, "Set the stage shader by name - setStageShader( str stage, str VPpath, str FPpath, bool deferred )" },
    {"setStageActive", (PyCFunction)VRPyRendering::setStageActive, METH_VARARGS, "Activate/deactivate the stage - setStageActive( str stage, bool deferred, bool layer )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyRendering::addStage(VRPyRendering* self, PyObject* args) {
    if (!self->valid()) return NULL;
    const char* name = 0;
    const char* parent = 0;
    if (!PyArg_ParseTuple(args, "ss", &name, &parent)) return NULL;
    string sname, sparent;
    if (name); sname = string(name);
    if (parent); sparent = string(parent);
    if (auto s = VRScene::getCurrent()) s->addStage(sname, sparent);
    Py_RETURN_TRUE;
}

PyObject* VRPyRendering::setStageShader(VRPyRendering* self, PyObject* args) {
    if (!self->valid()) return NULL;
    const char* name = 0;
    const char* vp = 0;
    const char* fp = 0;
    int doDef;
    if (!PyArg_ParseTuple(args, "sssi", &name, &vp, &fp, &doDef)) return NULL;
    string sname, svp, sfp;
    if (name); sname = string(name);
    if (vp); svp = string(vp);
    if (fp); sfp = string(fp);
    if (auto s = VRScene::getCurrent()) s->setStageShader(sname, svp, sfp, doDef);
    Py_RETURN_TRUE;
}

PyObject* VRPyRendering::setStageActive(VRPyRendering* self, PyObject* args) {
    if (!self->valid()) return NULL;
    const char* name = 0;
    int da, la;
    if (!PyArg_ParseTuple(args, "sii", &name, &da, &la)) return NULL;
    string sname;
    if (name); sname = string(name);
    if (auto s = VRScene::getCurrent()) s->setStageActive(sname, da, la);
    Py_RETURN_TRUE;
}
