#include "VRPyLogistics.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyTypeCaster.h"
#include "core/scripting/VRPyTransform.h"
#include "core/scripting/VRPyObject.h"


// ------------------------------------------------------------------------ NODE

template<> PyTypeObject VRPyBaseT<FNode>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Factory.Node",             /*tp_name*/
    sizeof(FPyNode),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "Logistics node binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    FPyNode::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    0,                 /* tp_new */
};

PyMethodDef FPyNode::methods[] = {
    {"connect", (PyCFunction)FPyNode::connect, METH_VARARGS, "Connect node to this node" },
    {"set", (PyCFunction)FPyNode::set, METH_VARARGS, "Set the node content" },
    {"setTransform", (PyCFunction)FPyNode::setTransform, METH_VARARGS, "Set the node transformation" },
    {"getTransform", (PyCFunction)FPyNode::getTransform, METH_NOARGS, "Return the node transformation" },
    {NULL}  /* Sentinel */
};

PyObject* FPyNode::connect(FPyNode* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyNode::connect - Object is invalid"); return NULL; }

    FPyNode* node;
    if (! PyArg_ParseTuple(args, "O", &node)) return NULL;

    self->obj->connect(node->obj);
    Py_RETURN_TRUE;
}

PyObject* FPyNode::set(FPyNode* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyNode::set - Object is invalid"); return NULL; }

    PyObject* o;
    if (! PyArg_ParseTuple(args, "O", &o)) return NULL;
    FPyProduct* object = (FPyProduct*)o; // actually needs FPyObject, but should work like this

    self->obj->set(object->obj);
    Py_RETURN_TRUE;
}

PyObject* FPyNode::setTransform(FPyNode* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyNode::setTransform - Object is invalid"); return NULL; }

    VRPyTransform* t;
    if (! PyArg_ParseTuple(args, "O", &t)) return NULL;

    self->obj->setTransform(t->obj);
    //self->obj->getTransform()->setWorldMatrix(t->obj->getWorldMatrix());
    Py_RETURN_TRUE;
}

PyObject* FPyNode::getTransform(FPyNode* self) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyNode::getTransform - Object is invalid"); return NULL; }
    return VRPyTransform::fromPtr(self->obj->getTransform());
}


// ------------------------------------------------------------------------ NETWORK

template<> PyTypeObject VRPyBaseT<FNetwork>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Factory.Network",             /*tp_name*/
    sizeof(FPyNetwork),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "Logistics network binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    FPyNetwork::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    0,                 /* tp_new */
};

PyMethodDef FPyNetwork::methods[] = {
    {"addNodes", (PyCFunction)FPyNetwork::addNodes, METH_VARARGS, "Add new node to network" },
    {"stroke", (PyCFunction)FPyNetwork::stroke, METH_VARARGS, "Stroke the network" },
    {NULL}  /* Sentinel */
};

PyObject* FPyNetwork::addNodes(FPyNetwork* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyNetwork::addNodes - Object is invalid"); return NULL; }

    FPyNode* n;
    FNode* node;
    int i = 0;
    if (! PyArg_ParseTuple(args, "iO", &i, &n)) return NULL;
    if ((PyObject*)n == Py_None) node = 0;
    else node = n->obj;

    return FPyNode::fromPtr( self->obj->addNodeChain(i, node) );
}

PyObject* FPyNetwork::stroke(FPyNetwork* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyNetwork::stroke - Object is invalid"); return NULL; }
    float r,g,b,k;
    if (! PyArg_ParseTuple(args, "ffff", &r, &g, &b, &k)) return NULL;
    return VRPyObject::fromPtr( (OSG::VRObject*)self->obj->stroke(OSG::Vec3f(r,g,b), k) );
}


// ------------------------------------------------------------------------ PATH

template<> PyTypeObject VRPyBaseT<FPath>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Factory.Path",             /*tp_name*/
    sizeof(FPyPath),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_ITER, /*tp_flags*/
    "Logistics path binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    (getiterfunc)FPyPath::tp_iter,		               /* tp_iter */
    (iternextfunc)FPyPath::tp_iternext,		               /* tp_iternext */
    FPyPath::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    0,                 /* tp_new */
};

PyMethodDef FPyPath::methods[] = {
    {"set", (PyCFunction)FPyPath::set, METH_VARARGS, "Set path" },
    {"add", (PyCFunction)FPyPath::add, METH_VARARGS, "Add to path" },
    {NULL}  /* Sentinel */
};

PyObject* FPyPath::set(FPyPath* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyPath::set - Object is invalid"); return NULL; }

    FPyNode *n1, *n2;
    if (! PyArg_ParseTuple(args, "OO", &n1, &n2)) return NULL;

    self->obj->set(n1->obj, n2->obj);
    Py_RETURN_TRUE;
}

PyObject* FPyPath::add(FPyPath* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyPath::set - Object is invalid"); return NULL; }

    FPyNode *n;
    if (! PyArg_ParseTuple(args, "O", &n)) return NULL;

    self->obj->add(n->obj);
    Py_RETURN_TRUE;
}

PyObject* FPyPath::tp_iter(FPyPath* self) {
    self->i = 0;
    Py_INCREF(self);
    return (PyObject*)self;
}

PyObject* FPyPath::tp_iternext(FPyPath* self) {
    int i = self->i;
    if (i < (int)self->obj->get().size()) {
        PyObject *tmp = FPyNode::fromPtr(self->obj->get()[i]);
        self->i = i+1;
        return tmp;
    } else {
        self->i = 0;
        PyErr_SetNone(PyExc_StopIteration);
        return NULL;
    }
}


// ------------------------------------------------------------------------ TRANSPORTER

template<> PyTypeObject VRPyBaseT<FTransporter>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Factory.Transporter",             /*tp_name*/
    sizeof(FPyTransporter),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "Logistics transporter binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    FPyTransporter::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    0,                 /* tp_new */
};

PyMethodDef FPyTransporter::methods[] = {
    {"setPath", (PyCFunction)FPyTransporter::setPath, METH_VARARGS, "Set transporter path" },
    {"setSpeed", (PyCFunction)FPyTransporter::setSpeed, METH_VARARGS, "Set transporter speed" },
    {NULL}  /* Sentinel */
};

PyObject* FPyTransporter::setPath(FPyTransporter* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyTransporter::setPath - Object is invalid"); return NULL; }

    FPyPath* p;
    if (! PyArg_ParseTuple(args, "O", &p)) return NULL;

    self->obj->setPath(p->obj);
    Py_RETURN_TRUE;
}

PyObject* FPyTransporter::setSpeed(FPyTransporter* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyTransporter::setSpeed - Object is invalid"); return NULL; }
    float s = parseFloat(args);
    self->obj->setSpeed(s);
    Py_RETURN_TRUE;
}



// ------------------------------------------------------------------------ CONTAINER

template<> PyTypeObject VRPyBaseT<FContainer>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Factory.Container",             /*tp_name*/
    sizeof(FPyContainer),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "Logistics container binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    FPyContainer::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    0,                 /* tp_new */
};

PyMethodDef FPyContainer::methods[] = {
    {"setCapacity", (PyCFunction)FPyContainer::setCapacity, METH_VARARGS, "Set container capacity" },
    {"getCapacity", (PyCFunction)FPyContainer::getCapacity, METH_NOARGS, "Set container capacity" },
    {"isEmpty", (PyCFunction)FPyContainer::isEmpty, METH_NOARGS, "Set container capacity" },
    {"isFull", (PyCFunction)FPyContainer::isFull, METH_NOARGS, "Set container capacity" },
    {"clear", (PyCFunction)FPyContainer::clear, METH_NOARGS, "Set container capacity" },
    {"getCount", (PyCFunction)FPyContainer::getCount, METH_NOARGS, "Get number of products in the container" },
    {"add", (PyCFunction)FPyContainer::add, METH_VARARGS, "Add a product to the container - add(product)" },
    {"get", (PyCFunction)FPyContainer::get, METH_NOARGS, "Get the product last put in the container and remove it from the container - product get()." },
    {"peek", (PyCFunction)FPyContainer::peek, METH_NOARGS, "Get the product last put in the container - product peek()." },
    {NULL}  /* Sentinel */
};

PyObject* FPyContainer::add(FPyContainer* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyContainer::add - Object is invalid"); return NULL; }
    FPyProduct* p = (FPyProduct*)parseObject(args);
    self->obj->add(p->obj);
    Py_RETURN_TRUE;
}

PyObject* FPyContainer::peek(FPyContainer* self) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyContainer::get - Object is invalid"); return NULL; }
    return FPyProduct::fromPtr( self->obj->peek() );
}

PyObject* FPyContainer::get(FPyContainer* self) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyContainer::get - Object is invalid"); return NULL; }
    return FPyProduct::fromPtr( self->obj->pop() );
}

PyObject* FPyContainer::clear(FPyContainer* self) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyContainer::getCapacity - Object is invalid"); return NULL; }
    self->obj->clear();
    Py_RETURN_TRUE;
}

PyObject* FPyContainer::isFull(FPyContainer* self) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyContainer::getCapacity - Object is invalid"); return NULL; }
    return PyBool_FromLong( self->obj->isFull() );
}

PyObject* FPyContainer::isEmpty(FPyContainer* self) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyContainer::getCapacity - Object is invalid"); return NULL; }
    return PyBool_FromLong( self->obj->isEmpty() );
}

PyObject* FPyContainer::getCapacity(FPyContainer* self) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyContainer::getCapacity - Object is invalid"); return NULL; }
    return PyInt_FromLong( self->obj->getCapacity() );
}

PyObject* FPyContainer::setCapacity(FPyContainer* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyContainer::setCapacity - Object is invalid"); return NULL; }
    self->obj->setCapacity( parseFloat(args) );
    Py_RETURN_TRUE;
}

PyObject* FPyContainer::getCount(FPyContainer* self) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyContainer::getCount - Object is invalid"); return NULL; }
    return PyInt_FromLong( self->obj->getCount() );
}


// ------------------------------------------------------------------------ PRODUCT

template<> PyTypeObject VRPyBaseT<FProduct>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Factory.Product",             /*tp_name*/
    sizeof(FPyProduct),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "Logistics product binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    FPyProduct::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    0,                 /* tp_new */
};

PyMethodDef FPyProduct::methods[] = {
    {"getGeometry", (PyCFunction)FPyProduct::getGeometry, METH_NOARGS, "Get product geometry" },
    {NULL}  /* Sentinel */
};

PyObject* FPyProduct::getGeometry(FPyProduct* self) {
    return VRPyTypeCaster::cast(self->obj->getTransformation());
}

// ------------------------------------------------------------------------ LOGISTICS

template<> PyTypeObject VRPyBaseT<FLogistics>::type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "VR.Factory.Logistics",             /*tp_name*/
    sizeof(FPyLogistics),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "Logistics binding",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    FPyLogistics::methods,             /* tp_methods */
    0,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)init,      /* tp_init */
    0,                         /* tp_alloc */
    New,                 /* tp_new */
};

PyMethodDef FPyLogistics::methods[] = {
    {"addProduct", (PyCFunction)FPyLogistics::addProduct, METH_VARARGS, "Add a new product from geometry - FProduct addProduct(Geometry geo)" },
    {"addNetwork", (PyCFunction)FPyLogistics::addNetwork, METH_NOARGS, "Add a new network" },
    {"addPath", (PyCFunction)FPyLogistics::addPath, METH_NOARGS, "Add a new path" },
    {"addTransporter", (PyCFunction)FPyLogistics::addTransporter, METH_VARARGS, "Add a new transporter" },
    {"addContainer", (PyCFunction)FPyLogistics::addContainer, METH_VARARGS, "Add a new container" },
    {"fillContainer", (PyCFunction)FPyLogistics::fillContainer, METH_VARARGS, "Fill container : fillContainer(container, N, obj)" },
    {"update", (PyCFunction)FPyLogistics::update, METH_NOARGS, "Update logistics simulation" },
    {"destroy", (PyCFunction)FPyLogistics::destroy, METH_NOARGS, "Destroy logistics simulation" },
    {"getContainers", (PyCFunction)FPyLogistics::getContainers, METH_NOARGS, "Destroy logistics simulation" },
    {NULL}  /* Sentinel */
};

PyObject* FPyLogistics::addProduct(FPyLogistics* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyLogistics::addProduct - Object is invalid"); return NULL; }
    VRPyTransform* t;
    if (! PyArg_ParseTuple(args, "O", &t)) return NULL;
    return FPyProduct::fromPtr( self->obj->addProduct( t->obj ) );
}

PyObject* FPyLogistics::getContainers(FPyLogistics* self) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyLogistics::getContainers - Object is invalid"); return NULL; }

    vector<FContainer*> objs = self->obj->getContainers();

    PyObject* li = PyList_New(objs.size());
    for (uint i=0; i<objs.size(); i++) {
        PyList_SetItem(li, i, FPyContainer::fromPtr(objs[i]));
    }

    return li;
}

PyObject* FPyLogistics::addNetwork(FPyLogistics* self) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyLogistics::addNetwork - Object is invalid"); return NULL; }
    return FPyNetwork::fromPtr(self->obj->addNetwork());
}

PyObject* FPyLogistics::addPath(FPyLogistics* self) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyLogistics::addPath - Object is invalid"); return NULL; }
    return FPyPath::fromPtr(self->obj->addPath());
}

PyObject* FPyLogistics::addTransporter(FPyLogistics* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyLogistics::addTransporter - Object is invalid"); return NULL; }

    string type = parseString(args);
    FTransporter::FTType t = FTransporter::PRODUCT;
    if (type == "Container full") t = FTransporter::CONTAINER_FULL;
    if (type == "Container empty") t = FTransporter::CONTAINER_EMPTY;

    return FPyTransporter::fromPtr(self->obj->addTransporter(t));
}

PyObject* FPyLogistics::addContainer(FPyLogistics* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyLogistics::addContainer - Object is invalid"); return NULL; }
    VRPyTransform* t = 0;
    if (! PyArg_ParseTuple(args, "O", &t)) return NULL;
    OSG::VRTransform* tr = 0;
    if (!isNone((PyObject*)t)) tr = t->obj;
    FContainer* c = self->obj->addContainer(tr);
    return FPyContainer::fromPtr(c);
}

PyObject* FPyLogistics::fillContainer(FPyLogistics* self, PyObject* args) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyLogistics::fillContainer - Object is invalid"); return NULL; }

    VRPyTransform* t;
    FPyContainer* c;
    int i;
    if (! PyArg_ParseTuple(args, "OiO", &c, &i, &t)) return NULL;

    self->obj->fillContainer(c->obj, i, t->obj);
    Py_RETURN_TRUE;
}

PyObject* FPyLogistics::update(FPyLogistics* self) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyLogistics::update - Object is invalid"); return NULL; }
    self->obj->update();
    Py_RETURN_TRUE;
}

PyObject* FPyLogistics::destroy(FPyLogistics* self) {
    if (self->obj == 0) { PyErr_SetString(err, "FPyLogistics::destroy - Object is invalid"); return NULL; }
    delete self->obj;
    Py_RETURN_TRUE;
}






