#include "VRPyOntology.h"
#include "VRProperty.h"
#include "core/scripting/VRPyTypeCaster.h"
#include "core/scripting/VRPyBaseT.h"

simpleVRPyType(Ontology, New_ptr);
simpleVRPyType(Entity, New_named_ptr);
simpleVRPyType(Concept, New_named_ptr);
simpleVRPyType(Property, New_named_ptr);

// --------------------- Property --------------------

PyMethodDef VRPyProperty::methods[] = {
    {"getName", (PyCFunction)VRPyProperty::getName, METH_NOARGS, "Return the name - str getName()" },
    {"toString", (PyCFunction)VRPyProperty::toString, METH_NOARGS, "Return the property as string - str toString()" },
    {"getType", (PyCFunction)VRPyProperty::getType, METH_NOARGS, "Return the type - str getType()" },
    {"getValue", (PyCFunction)VRPyProperty::getValue, METH_NOARGS, "Return value - val getValue()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyProperty::getName(VRPyProperty* self) {
    return PyString_FromString( self->objPtr->name.c_str() );
}

PyObject* VRPyProperty::toString(VRPyProperty* self) {
    return PyString_FromString( self->objPtr->toString().c_str() );
}

PyObject* VRPyProperty::getType(VRPyProperty* self) {
    return PyString_FromString( self->objPtr->type.c_str() );
}

PyObject* VRPyProperty::getValue(VRPyProperty* self) {
    return PyString_FromString( self->objPtr->value.c_str() );
}

// --------------------- Concept --------------------

PyMethodDef VRPyConcept::methods[] = {
    {"getName", (PyCFunction)VRPyConcept::getName, METH_NOARGS, "Return the name - str getName()" },
    {"toString", (PyCFunction)VRPyConcept::toString, METH_NOARGS, "Return the concept as string - str toString()" },
    {"getProperty", (PyCFunction)VRPyConcept::getProperty, METH_VARARGS, "Return a property by name - property getProperty( str name )" },
    {"getProperties", (PyCFunction)VRPyConcept::getProperties, METH_NOARGS, "Return all properties - [property] getProperties()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyConcept::getName(VRPyConcept* self) {
    return PyString_FromString( self->objPtr->name.c_str() );
}

PyObject* VRPyConcept::toString(VRPyConcept* self) {
    return PyString_FromString( self->objPtr->toString().c_str() );
}

PyObject* VRPyConcept::getProperty(VRPyConcept* self, PyObject* args) {
    const char* prop = 0;
    if (! PyArg_ParseTuple(args, "|s:open", (char*)&prop)) return NULL;
    string pname; if (prop) pname = prop;
    return VRPyProperty::fromSharedPtr( self->objPtr->getProperty(pname) );
}

PyObject* VRPyConcept::getProperties(VRPyConcept* self) {
    auto props = self->objPtr->getProperties();
    auto res = PyList_New(props.size());
    for (int i=0; i<props.size(); i++)
        PyList_SetItem(res, i, VRPyProperty::fromSharedPtr( props[i] ) );
    return res;
}

// --------------------- Entity --------------------

PyMethodDef VRPyEntity::methods[] = {
    {"getName", (PyCFunction)VRPyEntity::getName, METH_NOARGS, "Return the name - str getName()" },
    {"toString", (PyCFunction)VRPyEntity::toString, METH_NOARGS, "Return the entity as string - str toString()" },
    {"getConcept", (PyCFunction)VRPyEntity::getConcept, METH_NOARGS, "Return the concept - concept getConcept()" },
    {"getProperties", (PyCFunction)VRPyConcept::getProperties, METH_VARARGS, "Return all properties or the properties of a certain type - [property] getProperties( str )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyEntity::getName(VRPyEntity* self) {
    return PyString_FromString( self->objPtr->name.c_str() );
}

PyObject* VRPyEntity::toString(VRPyEntity* self) {
    return PyString_FromString( self->objPtr->toString().c_str() );
}

PyObject* VRPyEntity::getConcept(VRPyEntity* self) {
    return VRPyConcept::fromSharedPtr( self->objPtr->concept );
}

PyObject* VRPyEntity::getProperties(VRPyEntity* self, PyObject* args) {
    const char* prop = 0;
    if (! PyArg_ParseTuple(args, "|s:open", (char*)&prop)) return NULL;
    string pname; if (prop) pname = prop;
    auto props = self->objPtr->getProperties(pname);
    auto res = PyList_New(props.size());
    for (int i=0; i<props.size(); i++)
        PyList_SetItem(res, i, VRPyProperty::fromSharedPtr( props[i] ) );
    return res;
}

// --------------------- Ontology --------------------

PyMethodDef VRPyOntology::methods[] = {
    {"open", (PyCFunction)VRPyOntology::open, METH_VARARGS, "Open OWL file - open( str path )" },
    {"toString", (PyCFunction)VRPyOntology::toString, METH_NOARGS, "Return the full ontology as string - str toString()" },
    {"getConcept", (PyCFunction)VRPyOntology::getConcept, METH_VARARGS, "Return a concept by name - concept getConcept( str name )\n\tThe first concept is named 'Thing'" },
    {"getConcepts", (PyCFunction)VRPyOntology::getConcepts, METH_NOARGS, "Return all concepts - [concept] getConcepts()" },
    {"getEntities", (PyCFunction)VRPyOntology::getEntities, METH_VARARGS, "Return all entities by concept name - [entity] getEntities( str concept )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyOntology::toString(VRPyOntology* self) {
    return PyString_FromString( self->objPtr->toString().c_str() );
}

PyObject* VRPyOntology::open(VRPyOntology* self, PyObject* args) {
    const char* path = 0;
    if (! PyArg_ParseTuple(args, "s:open", (char*)&path)) return NULL;
    self->objPtr->open(path);
    Py_RETURN_TRUE;
}

PyObject* VRPyOntology::getConcept(VRPyOntology* self, PyObject* args) {
    const char* name = 0;
    if (! PyArg_ParseTuple(args, "s:getConcept", (char*)&name)) return NULL;
    return VRPyConcept::fromSharedPtr( self->objPtr->getConcept(name) );
}

PyObject* VRPyOntology::getConcepts(VRPyOntology* self) {
    auto concepts = self->objPtr->getConcepts();
    auto res = PyList_New(concepts.size());
    for (int i=0; i<concepts.size(); i++)
        PyList_SetItem(res, i, VRPyConcept::fromSharedPtr( concepts[i] ) );
    return res;
}

PyObject* VRPyOntology::getEntities(VRPyOntology* self, PyObject* args) {
    const char* concept = 0;
    if (! PyArg_ParseTuple(args, "s:getEntities", (char*)&concept)) return NULL;
    auto entities = self->objPtr->getInstances(concept);
    auto res = PyList_New(entities.size());
    for (int i=0; i<entities.size(); i++)
        PyList_SetItem(res, i, VRPyEntity::fromSharedPtr( entities[i] ) );
    return res;
}


