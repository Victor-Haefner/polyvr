#include "VRPyOntology.h"
#include "VRProperty.h"
#include "VROntologyRule.h"
#include "core/scripting/VRPyTypeCaster.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyBaseFactory.h"
#include "core/utils/toString.h"

using namespace OSG;

simpleVRPyType(Ontology, New_named_ptr);
simpleVRPyType(Entity, 0);
simpleVRPyType(Concept, 0);
simpleVRPyType(Property, 0);
simpleVRPyType(OntologyRule, 0);
simpleVRPyType(Reasoner, New_ptr);

// --------------------- Property --------------------

PyMethodDef VRPyProperty::methods[] = {
    {"getName", (PyCFunction)VRPyProperty::getName, METH_NOARGS, "Return the name - str getName()" },
    {"toString", (PyCFunction)VRPyProperty::toString, METH_NOARGS, "Return the property as string - str toString()" },
    {"getType", (PyCFunction)VRPyProperty::getType, METH_NOARGS, "Return the type - str getType()" },
    {"getValue", (PyCFunction)VRPyProperty::getValue, METH_NOARGS, "Return value - val getValue()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyProperty::getName(VRPyProperty* self) {
    return PyString_FromString( self->objPtr->getName().c_str() );
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
    {"addProperty", (PyCFunction)VRPyConcept::addProperty, METH_VARARGS, "Add new property - property addProperty( str name, str value )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyConcept::getName(VRPyConcept* self) {
    return PyString_FromString( self->objPtr->getName().c_str() );
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

PyObject* VRPyConcept::addProperty(VRPyConcept* self, PyObject* args) {
    const char* prop = 0;
    const char* val = 0;
    if (! PyArg_ParseTuple(args, "ss:addProperty", (char*)&prop, (char*)&val)) return NULL;
    string pname; if (prop) pname = prop;
    string pval; if (val) pval = val;
    return VRPyProperty::fromSharedPtr( self->objPtr->addProperty(pname, pval) );
}

// --------------------- OntologyRule --------------------

PyMethodDef VRPyOntologyRule::methods[] = {
    {"toString", (PyCFunction)VRPyOntologyRule::toString, METH_NOARGS, "Return the rule as string - str toString()" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyOntologyRule::toString(VRPyOntologyRule* self) {
    return PyString_FromString( self->objPtr->toString().c_str() );
}

// --------------------- Entity --------------------

PyMethodDef VRPyEntity::methods[] = {
    {"getName", (PyCFunction)VRPyEntity::getName, METH_NOARGS, "Return the name - str getName()" },
    {"toString", (PyCFunction)VRPyEntity::toString, METH_NOARGS, "Return the entity as string - str toString()" },
    {"getConcept", (PyCFunction)VRPyEntity::getConcept, METH_NOARGS, "Return the concept - concept getConcept()" },
    {"getProperties", (PyCFunction)VRPyEntity::getProperties, METH_VARARGS, "Return all properties or the properties of a certain type - [property] getProperties( str )" },
    {"set", (PyCFunction)VRPyEntity::set, METH_VARARGS, "Set a property - set( str prop, str value )" },
    {"add", (PyCFunction)VRPyEntity::add, METH_VARARGS, "Add a property - add( str prop, str value )" },
    {"setVector", (PyCFunction)VRPyEntity::setVector, METH_VARARGS, "Set a vector property - set( str prop, str value [x,y,z], str vector concept )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyEntity::add(VRPyEntity* self, PyObject* args) {
    const char* prop = 0;
    const char* val = 0;
    if (! PyArg_ParseTuple(args, "ss:add", (char*)&prop, (char*)&val)) return NULL;
    string pname; if (prop) pname = prop;
    string pval; if (val) pval = val;
    self->objPtr->add( pname, pval );
    Py_RETURN_TRUE;
}

PyObject* VRPyEntity::set(VRPyEntity* self, PyObject* args) {
    const char* prop = 0;
    const char* val = 0;
    if (! PyArg_ParseTuple(args, "ss:set", (char*)&prop, (char*)&val)) return NULL;
    string pname; if (prop) pname = prop;
    string pval; if (val) pval = val;
    self->objPtr->set( pname, pval );
    Py_RETURN_TRUE;
}

PyObject* VRPyEntity::setVector(VRPyEntity* self, PyObject* args) {
    const char* prop = 0;
    const char* vectype = "Vector";
    PyObject* val = 0;
    if (! PyArg_ParseTuple(args, "sO|s:setVector", (char*)&prop, (char*)&val, (char*)&vectype)) return NULL;

    auto o_vals = pyListToVector(val);
    vector<string> vals;
    for (auto v : o_vals) {
        string s = ::toString( PyFloat_AsDouble(v) );
        vals.push_back(s);
    }

    string pname; if (prop) pname = prop;
    string pvt; if (vectype) pvt = vectype;
    self->objPtr->setVector( pname, vals, pvt );
    Py_RETURN_TRUE;
}

PyObject* VRPyEntity::getName(VRPyEntity* self) {
    return PyString_FromString( self->objPtr->getName().c_str() );
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
    {"addConcept", (PyCFunction)VRPyOntology::addConcept, METH_VARARGS, "Add a new concept - concept addConcept( str concept, str parent = "", dict properties {str:str} )" },
    {"addEntity", (PyCFunction)VRPyOntology::addEntity, METH_VARARGS, "Add a new entity - entity addEntity( str name, str concept )" },
    {"getEntity", (PyCFunction)VRPyOntology::getEntity, METH_VARARGS, "Get a entity by name - entity getEntity( str name )" },
    {"addRule", (PyCFunction)VRPyOntology::addRule, METH_VARARGS, "Add a new rule - addRule( str rule )" },
    {"merge", (PyCFunction)VRPyOntology::merge, METH_VARARGS, "Merge in another ontology - merge( ontology )" },
    {"copy", (PyCFunction)VRPyOntology::copy, METH_NOARGS, "Copy the ontology - ontology copy()" },
    //{"addModule", (PyCFunction)proxy<string, VRPyOntology, void (VROntology::*)(string), &VROntology::addModule>::set, METH_VARARGS, "Add module from library - addModule( str name )" },
    {"addModule", PySetter(Ontology, addModule, string), "Add module from library - addModule( str name )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyOntology::copy(VRPyOntology* self) {
    return VRPyOntology::fromSharedPtr( self->objPtr->copy() );
}

PyObject* VRPyOntology::merge(VRPyOntology* self, PyObject* args) {
    VRPyOntology* other;
    if (! PyArg_ParseTuple(args, "O:merge", &other)) return NULL;
    if (!isNone((PyObject*)other)) self->objPtr->merge( other->objPtr );
    Py_RETURN_TRUE;
}

PyObject* VRPyOntology::addRule(VRPyOntology* self, PyObject* args) {
    const char *name = 0;
    if (! PyArg_ParseTuple(args, "s:addRule", (char*)&name)) return NULL;
    string sname;
    if (name) sname = name;
    VROntologyRulePtr r = self->objPtr->addRule(sname, ""); // TODO, optionally pass associated concept name
    return VRPyOntologyRule::fromSharedPtr( r );
}

PyObject* VRPyOntology::addEntity(VRPyOntology* self, PyObject* args) {
    const char* name = 0;
    const char* concept = 0;
    PyDictObject* propDict = 0;
    if (! PyArg_ParseTuple(args, "s|sO:addEntity", (char*)&name, (char*)&concept, &propDict)) return NULL;
    string sname, sconcept;
    if (name) sname = name;
    if (concept) sconcept = concept;
    auto entity = self->objPtr->addInstance(sname, sconcept);
    if (propDict) {
        PyObject* keys = PyDict_Keys((PyObject*)propDict);
        for (int i=0; i<PyList_Size(keys); i++) {
            PyObject* key = PyList_GetItem(keys, i);
            PyObject* item = PyDict_GetItem((PyObject*)propDict, key);
            entity->set(PyString_AsString(key), PyString_AsString(item));
        }
    }
    return VRPyEntity::fromSharedPtr( entity );
}

PyObject* VRPyOntology::getEntity(VRPyOntology* self, PyObject* args) {
    const char* name = 0;
    if (! PyArg_ParseTuple(args, "s:addEntity", (char*)&name) ) return NULL;
    string sname;
    if (name) sname = name;
    auto entity = self->objPtr->getInstance(sname);
    return VRPyEntity::fromSharedPtr( entity );
}

PyObject* VRPyOntology::addConcept(VRPyOntology* self, PyObject* args) {
    const char* name = 0;
    const char* parent = 0;
    PyDictObject* propDict = 0;
    if (! PyArg_ParseTuple(args, "s|sO:addConcept", (char*)&name, (char*)&parent, &propDict)) return NULL;
    string sname, sparent;
    if (name) sname = name;
    if (parent) sparent = parent;
    VRConceptPtr concept = self->objPtr->addConcept(sname, sparent);
    if (!concept) return setErr("Failed to add concept " + sname + " to parent " + sparent);

    if (propDict) {
        PyObject* keys = PyDict_Keys((PyObject*)propDict);
        for (int i=0; i<PyList_Size(keys); i++) {
            PyObject* key = PyList_GetItem(keys, i);
            PyObject* item = PyDict_GetItem((PyObject*)propDict, key);
            concept->addProperty(PyString_AsString(key), PyString_AsString(item));
        }
    }
    return VRPyConcept::fromSharedPtr( concept );
}

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

// --------------------- Ontology --------------------

PyMethodDef VRPyReasoner::methods[] = {
    {"process", (PyCFunction)VRPyReasoner::process, METH_VARARGS, "Process query - process( str query, ontology )" },
    {NULL}  /* Sentinel */
};

PyObject* VRPyReasoner::process(VRPyReasoner* self, PyObject* args) {
    const char* query = 0;
    VRPyOntology* onto = 0;
    if (! PyArg_ParseTuple(args, "sO:process", (char*)&query, &onto)) return NULL;

    auto pyres = PyList_New(0);
    if (!query) return pyres;
    auto res = self->objPtr->process(string(query), onto->objPtr);
    for (auto r : res) {
        for (auto i : r.instances) PyList_Append(pyres, VRPyEntity::fromSharedPtr(i));
    }
    return pyres;
}

