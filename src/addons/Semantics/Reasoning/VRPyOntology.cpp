#include "VRPyOntology.h"
#include "VRProperty.h"
#include "VROntologyRule.h"
#include "core/scripting/VRPyTypeCaster.h"
#include "core/scripting/VRPyBaseT.h"
#include "core/scripting/VRPyBaseFactory.h"
#include "core/scripting/VRPyObject.h"
#include "core/scripting/VRPyMath.h"
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
    {"toString", PyWrap(Property, toString, "Return the property as string - str toString()", string ) },
    {"getType", PyWrap(Property, getType, "Return the type - str getType()", string ) },
    {"getValue", PyWrap(Property, getValue, "Return value - val getValue()", string ) },
    {NULL}  /* Sentinel */
};

// --------------------- Concept --------------------

PyMethodDef VRPyConcept::methods[] = {
    {"toString", PyWrapOpt(Concept, toString, "Return the concept as string - str toString()", "", string, string ) },
    {"getProperty", PyWrapOpt(Concept, getProperty, "Return a property by name - property getProperty( str name )", "1", VRPropertyPtr, string, bool ) },
    {"getProperties", PyWrap(Concept, getProperties, "Return all properties - [property] getProperties()", vector<VRPropertyPtr> ) },
    {"addProperty", PyWrap(Concept, addProperty, "Add new property - property addProperty( str name, str value )", VRPropertyPtr, string, string ) },
    {"append", PyWrapOpt(Concept, append, "Add another existing concept as child - concept append( str name )", "0", VRConceptPtr, string, bool ) },
    {NULL}  /* Sentinel */
};

// --------------------- OntologyRule --------------------

PyMethodDef VRPyOntologyRule::methods[] = {
    {"toString", PyWrap(OntologyRule, toString, "Return the rule as string - str toString()", string ) },
    {NULL}  /* Sentinel */
};

// --------------------- Entity --------------------

template<> PyObject* VRPyTypeCaster::cast(const VRPropertyValue& v) {
    VRPropertyPtr p = v.p;
    VROntologyPtr o = v.o;
    if (!p) Py_RETURN_NONE;
    if (p->value == "") Py_RETURN_NONE;
    if (p->type == "int") return PyInt_FromLong( toInt(p->value) );
    if (p->type == "float") return PyFloat_FromDouble( toFloat(p->value) );
    if (p->type == "string") return PyString_FromString( p->value.c_str() );
    if (o) {
        if (auto e = o->getEntity(p->value)) return VRPyEntity::fromSharedPtr(e);
    }
    return PyString_FromString( p->value.c_str() );
};

PyMethodDef VRPyEntity::methods[] = {
    {"getSGObject", PyWrap(Entity, getSGObject, "Get linked entity", VRObjectPtr) },
    {"toString", PyWrap(Entity, toString, "Return the entity as string", string ) },
    {"getConcept", PyWrap(Entity, getConcept, "Return the concept", VRConceptPtr ) },
    {"getProperties", PyCastWrapOpt(Entity, getAll, "Return all properties or the properties of a certain type", "", vector<VRPropertyPtr>, string ) },
    {"set", PyWrapOpt(Entity, set, "Set a property, prop, value, pos = 0", "0", void, string, string, int ) },
    {"add", PyWrap(Entity, add, "Add a property, prop, value", void, string, string ) },
    {"clear", PyWrap(Entity, clear, "Clear property", void, string ) },
    {"setVector", PyWrapOpt(Entity, setVec3, "Set a vector property, prop, value [x,y,z], concept = Vector, pos = 0", "Vector|0", void, string, Vec3d, string, int ) },
    {"addVector", PyWrapOpt(Entity, addVec3, "Add a vector property, prop, value [x,y,z], concept = Vector", "Vector", void, string, Vec3d, string ) },
    {"get", PyCastWrapOpt(Entity, getStringValue, "Get the value of ith property, prop, pos = 0", "0", VRPropertyValue, string, int ) },
    {"getVector", PyCastWrapOpt(Entity, getStringVector, "Get the value of ith vector property, prop, pos = 0", "0", vector<VRPropertyValue>, string, int ) },
    {"getAll", PyCastWrapOpt(Entity, getAllStringValues, "Get all values of property", "", vector<VRPropertyValue>, string ) },
    {"getAllVector", PyCastWrap(Entity, getAllStringVector, "Get all values of vector property", vector<vector<VRPropertyValue>>, string ) },
    {"setSGObject", PyWrap(Entity, setSGObject, "Link the entity to its scene graph object", void, VRObjectPtr ) },
    {"is_a", PyCastWrap(Entity, is_a, "Check is entity is of a given type", bool, string) },
    {NULL}  /* Sentinel */
};

// --------------------- Ontology --------------------

PyMethodDef VRPyOntology::methods[] = {
    {"open", PyWrap(Ontology, openOWL, "Open OWL file", void, string) },
    {"save", PyWrap(Ontology, saveOWL, "Write to OWL file", void, string) },
    {"toString", PyWrap(Ontology, toString, "Return the full ontology as string - str toString()", string ) },
    {"getConcept", PyWrap(Ontology, getConcept, "Return a concept by name - concept getConcept( str name )\n\tThe first concept is named 'Thing'", VRConceptPtr, string ) },
    {"getConcepts", PyWrap(Ontology, getConcepts, "Return all concepts - [concept] getConcepts()", vector<VRConceptPtr> ) },
    {"getEntities", PyWrapOpt(Ontology, getEntities, "Return all entities by concept name - [entity] getEntities( str concept )", "", vector<VREntityPtr>, string ) },
    {"addConcept", PyWrapOpt(Ontology, addConcept, "Add a new concept - concept addConcept( str concept, str parent = "", dict properties {str:str} )", "||", VRConceptPtr, string, string, map<string, string>, string ) },
    {"addEntity", PyWrapOpt(Ontology, addEntity, "Add a new entity, str name, str concept", "|", VREntityPtr, string, string, map<string, string>) },
    {"addVectorEntity", PyWrapOpt(Ontology, addVec3Entity, "Add a new entity, str name, str concept", "|", VREntityPtr, string, string, Vec3d) },
    {"getEntity", PyWrap(Ontology, getEntity, "Get an entity by name - entity getEntity( str name )", VREntityPtr, string ) },
    {"remEntity", PyWrap(Ontology, remEntity, "Remove an entity by name - remEntity( str name )", void, string ) },
    {"remEntities", PyWrap(Ontology, remEntities, "Remove all entity from concept - remEntities( str concept )", void, string ) },
    {"addRule", PyWrapOpt(Ontology, addRule, "Add a new rule - addRule( str rule )", "", VROntologyRulePtr, string, string ) },
    {"merge", PyWrap(Ontology, merge, "Merge in another ontology - merge( ontology )", void, VROntologyPtr ) },
    {"copy", PyWrap(Ontology, copy, "Copy the ontology - ontology copy()", VROntologyPtr ) },
    {"addModule", PyWrap(Ontology, addModule, "Add module from library", void, string) },
    {"process", PyWrapOpt(Ontology, process, "Process a query - process( str query )", "0", vector<VREntityPtr>, string, bool ) },
    {NULL}  /* Sentinel */
};

// --------------------- Ontology --------------------

PyMethodDef VRPyReasoner::methods[] = {
    {"process", PyWrap(Reasoner, process, "Process query - process( str query, ontology )", vector<VREntityPtr>, string, VROntologyPtr ) },
    {NULL}  /* Sentinel */
};

