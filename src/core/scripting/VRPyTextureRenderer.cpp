#include "VRPyTextureRenderer.h"
#include "VRPyCamera.h"
#include "VRPyMaterial.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(TextureRenderer, New_VRObjects_ptr);

PyMethodDef VRPyTextureRenderer::methods[] = {
    {"setup", (PyCFunction)VRPyTextureRenderer::setup, METH_VARARGS, "Setup texture renderer - setup( cam, width, height)" },
    {"getMaterial", (PyCFunction)VRPyTextureRenderer::getMaterial, METH_NOARGS, "Get the material with the rendering - getMaterial()" },
    {"setActive", (PyCFunction)VRPyTextureRenderer::setActive, METH_VARARGS, "Activate and deactivate the texture rendering - setActive( bool b )" },
    {"renderOnce", PyWrap(TextureRenderer, renderOnce, "Render once", VRTexturePtr) },
    {"getCamera", PyWrap(TextureRenderer, getCamera, "Get camera", VRCameraPtr) },
    {NULL}  /* Sentinel */
};

PyObject* VRPyTextureRenderer::setup(VRPyTextureRenderer* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyCamera* cam; int w, h;
    if (!PyArg_ParseTuple(args, "Oii", &cam, &w, &h)) return NULL;
    self->objPtr->setup(cam->objPtr, w, h);
    Py_RETURN_TRUE;
}

PyObject* VRPyTextureRenderer::setActive(VRPyTextureRenderer* self, PyObject* args) {
    if (!self->valid()) return NULL;
    bool b;
    if (!PyArg_ParseTuple(args, "i", &b)) return NULL;
    self->objPtr->setActive(b);
    Py_RETURN_TRUE;
}

PyObject* VRPyTextureRenderer::getMaterial(VRPyTextureRenderer* self) {
    if (!self->valid()) return NULL;
    return VRPyMaterial::fromSharedPtr( self->objPtr->getMaterial() );
}



