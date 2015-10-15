#ifndef PYVRLOGISTICS_H_INCLUDED
#define PYVRLOGISTICS_H_INCLUDED

#include "core/scripting/VRPyObject.h"
#include "VRLogistics.h"

struct FPyNode : VRPyBaseT<FNode> {
    static PyMethodDef methods[];

    static PyObject* connect(FPyNode* self, PyObject* args);
    static PyObject* set(FPyNode* self, PyObject* args);

    static PyObject* setTransform(FPyNode* self, PyObject* args);
    static PyObject* getTransform(FPyNode* self);
};

struct FPyNetwork : VRPyBaseT<FNetwork> {
    static PyMethodDef methods[];

    static PyObject* addNodes(FPyNetwork* self, PyObject* args);
    static PyObject* stroke(FPyNetwork* self, PyObject* args);
};

struct FPyPath : VRPyBaseT<FPath> {
    int i;
    static PyMethodDef methods[];

    static PyObject* set(FPyPath* self, PyObject* args);
    static PyObject* add(FPyPath* self, PyObject* args);
    static PyObject* tp_iter(FPyPath* self);
    static PyObject* tp_iternext(FPyPath* self);
};

struct FPyTransporter : VRPyBaseT<FTransporter> {
    static PyMethodDef methods[];

    static PyObject* setPath(FPyTransporter* self, PyObject* args);
    static PyObject* setSpeed(FPyTransporter* self, PyObject* args);
};

struct FPyContainer : VRPyBaseT<FContainer> {
    static PyMethodDef methods[];
    static PyObject* setCapacity(FPyContainer* self, PyObject* args);
    static PyObject* getCapacity(FPyContainer* self);
    static PyObject* isEmpty(FPyContainer* self);
    static PyObject* isFull(FPyContainer* self);
    static PyObject* clear(FPyContainer* self);
    static PyObject* getCount(FPyContainer* self);
    static PyObject* add(FPyContainer* self, PyObject* args);
    static PyObject* get(FPyContainer* self);
    static PyObject* peek(FPyContainer* self);
};

struct FPyProduct : VRPyBaseT<FProduct> {
    static PyMethodDef methods[];
    static PyObject* getGeometry(FPyProduct* self);
};

struct FPyLogistics : VRPyBaseT<FLogistics> {
    static PyMethodDef methods[];

    static PyObject* addProduct(FPyLogistics* self, PyObject* args);
    static PyObject* addNetwork(FPyLogistics* self);
    static PyObject* addPath(FPyLogistics* self);
    static PyObject* addTransporter(FPyLogistics* self, PyObject* args);
    static PyObject* addContainer(FPyLogistics* self, PyObject* args);
    static PyObject* fillContainer(FPyLogistics* self, PyObject* args);
    static PyObject* update(FPyLogistics* self);
    static PyObject* destroy(FPyLogistics* self);
    static PyObject* getContainers(FPyLogistics* self);
};

#endif // PYVRLOGISTICS_H_INCLUDED
