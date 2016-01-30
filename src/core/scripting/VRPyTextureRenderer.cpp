#include "VRPyTextureRenderer.h"
#include "VRPyCamera.h"
#include "VRPyMaterial.h"
#include "VRPyBaseT.h"

using namespace OSG;

simpleVRPyType(TextureRenderer, New_VRObjects_ptr);

PyMethodDef VRPyTextureRenderer::methods[] = {
    {"setup", (PyCFunction)VRPyTextureRenderer::setup, METH_VARARGS, "Setup texture renderer - setup( cam, width, height)" },
    {"getMaterial", (PyCFunction)VRPyTextureRenderer::getMaterial, METH_NOARGS, "Get the material with the rendering - getMaterial()" },
    {"addLink", (PyCFunction)VRPyTextureRenderer::addLink, METH_VARARGS, "Link subtree - addLink( object )" },
    {"remLink", (PyCFunction)VRPyTextureRenderer::remLink, METH_VARARGS, "Unlink subtree - remLink( object )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyTextureRenderer::addLink(VRPyTextureRenderer* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyObject* o;
    if (!PyArg_ParseTuple(args, "O", &o)) return NULL;
    self->objPtr->addLink( o->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyTextureRenderer::remLink(VRPyTextureRenderer* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyObject* o;
    if (!PyArg_ParseTuple(args, "O", &o)) return NULL;
    self->objPtr->remLink( o->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyTextureRenderer::setup(VRPyTextureRenderer* self, PyObject* args) {
    if (!self->valid()) return NULL;
    VRPyCamera* cam; int w, h;
    if (!PyArg_ParseTuple(args, "Oii", &cam, &w, &h)) return NULL;
    self->objPtr->setup(cam->objPtr, w, h);
    Py_RETURN_TRUE;
}

PyObject* VRPyTextureRenderer::getMaterial(VRPyTextureRenderer* self) {
    if (!self->valid()) return NULL;
    return VRPyMaterial::fromSharedPtr( self->objPtr->getMaterial() );
}



