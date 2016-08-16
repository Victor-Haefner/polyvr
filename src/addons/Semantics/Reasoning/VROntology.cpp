#include "VROntology.h"
#include "VRReasoner.h"
#include "VRProperty.h"
#include "core/utils/toString.h"
#include "core/utils/VRStorage_template.h"

/* no compiling?
    install the raptor2 ubuntu package for rdfxml parsing
    sudo apt-get install libraptor2-dev
*/

#include <raptor2/raptor2.h> // http://librdf.org/raptor/api/
#include <iostream>

using namespace OSG;

VROntology::VROntology(string name) {
    setStorageType("Ontology");
    setPersistency(0);
    setNameSpace("Ontology");
    setName(name);

    storeMap("Instances", &instances, true);
    storeMap("Rules", &rules, true);
}

VROntologyPtr VROntology::create(string name) {
    auto o = VROntologyPtr( new VROntology(name) );
    o->thing = VRConcept::create("Thing", o);
    o->concepts["Thing"] = o->thing;
    o->storeObj("thing", o->thing);
    return o;
}

VRConceptPtr VROntology::getConcept(string name) {
    if (concepts.count(name) == 0) { cout << "Warning: concept " << name << " not found!" << endl; return 0; }
    /*cout << "add concept " << name << "in: ";
    for (auto c : concepts) if (auto p = c.second.lock()) cout << " " << c.first << " " << p->ID << " ";
    cout << endl;*/
    auto p = concepts[name].lock();
    //cout << "found " << p->name << " " << p->ID << endl;
    return p;
}

vector<VRConceptPtr> VROntology::getConcepts() {
    vector<VRConceptPtr> res;
    for (auto c : concepts) res.push_back(c.second.lock());
    return res;
}

VRConceptPtr VROntology::addConcept(string concept, string parent) {
    if (concepts.count(concept)) { cout << "WARNING in VROntology::addConcept, " << concept << " known, skipping!\n"; return 0;  }

    auto p = thing;
    if (parent != "") {
        p = getConcept(parent);
        if (!p) { cout << "WARNING in VROntology::addConcept, " << parent << " not found while adding " << concept << "!\n"; return 0;  }
    }
    //cout << "VROntology::addConcept " << concept << " " << parent << " " << p->name << " " << p->ID << endl;
    p = p->append(concept);
    addConcept(p);
    return p;
}

void VROntology::addConcept(VRConceptPtr c) {
    if (concepts.count(c->getName())) { cout << "WARNING in VROntology::addConcept, " << c->getName() << " known, skipping!\n"; return;  }
    if (c == thing) return;
    concepts[c->getName()] = c;
    if (!c->parent.lock()) thing->append(c);
}

void VROntology::remConcept(VRConceptPtr c) {
    if (c == thing) return;
    if (!concepts.count(c->getName())) return;
    if (auto p = c->parent.lock()) p->remove(c);
    concepts.erase(c->getName());
}

void VROntology::renameConcept(VRConceptPtr c, string newName) {
    if (c == thing) return;
    if (!concepts.count(c->getName())) return;
    concepts.erase(c->getName());
    c->setName(newName);
    addConcept(c);
}

void VROntology::merge(VROntologyPtr o) { // Todo: check it well!
    for (auto c : o->rules) rules[c.first] = c.second;
    for (auto c : o->thing->children) {
        auto cn = c.second->copy();
        thing->append(cn);
        vector<VRConceptPtr> cpts;
        cn->getDescendance(cpts);
        for (auto c : cpts) {
            concepts[c->getName()] = c;
            c->ontology = shared_from_this();
        }
    }
}

VROntologyPtr VROntology::copy() {
    auto o = create(name);
    o->merge(shared_from_this());
    return o;
}

vector<VROntologyRulePtr> VROntology::getRules() {
    vector<VROntologyRulePtr> res;
    for (auto r : rules) res.push_back(r.second);
    return res;
}

VROntologyRulePtr VROntology::addRule(string rule) {
    VROntologyRulePtr r = VROntologyRule::create(rule);
    rules[r->ID] = r;
    return r;
}

VREntityPtr VROntology::addVectorInstance(string name, string concept, string x, string y, string z) {
    vector<string> v;
    v.push_back(x); v.push_back(y); v.push_back(z);
    return addVectorInstance(name, concept, v);
}

VREntityPtr VROntology::addVectorInstance(string name, string concept, vector<string> val) {
    auto i = addInstance(name, concept);
    int N = val.size();
    if (0 < N) i->set("x", val[0]);
    if (1 < N) i->set("y", val[1]);
    if (2 < N) i->set("z", val[2]);
    if (3 < N) i->set("w", val[3]);
    cout << "addVectorInstance " << name << " " << concept << endl;
    return i;
}

void VROntology::addInstance(VREntityPtr e) { instances[e->ID] = e; }

VREntityPtr VROntology::addInstance(string name, string concept) {
    auto c = getConcept(concept);
    auto e = VREntity::create(name, c);
    addInstance(e);
    return e;
}

VREntityPtr VROntology::getInstance(string instance) {
    for (auto i : instances) if (i.second->getName() == instance) return i.second;
    return 0;
}

vector<VREntityPtr> VROntology::getInstances(string concept) {
    vector<VREntityPtr> res;
    for (auto i : instances) {
        auto c = i.second->concept;
        if(c) { if(c->is_a(concept)) res.push_back(i.second); }
        else cout << "VROntology::getInstances " << i.second->getName() << " has no concept!" << endl;
    }
    return res;
}

string VROntology::toString() {
    string res = "Taxonomy:\n";
    res += thing->toString();
    res += "Entities:\n";
    for (auto e : instances) res += e.second->toString() + "\n";
    return res;
}

// RDF import

struct RDFStatement {
    string type;
    string graph;
    string object;
    string predicate;
    string subject;

    string toString(raptor_term* t) {
        if (t == 0) return "";
        switch(t->type) {
            case RAPTOR_TERM_TYPE_LITERAL:
                return string( (const char*)t->value.literal.string );
            case RAPTOR_TERM_TYPE_BLANK:
                return "BLANK";
            case RAPTOR_TERM_TYPE_UNKNOWN:
                return "UNKNOWN";
            case RAPTOR_TERM_TYPE_URI:
                auto uri = raptor_uri_as_string( t->value.uri );
                if (!uri) return "";
                string s( (const char*)uri );
                auto ss = splitString(s, '#');
                //raptor_free_memory(uri);
                return ss[ss.size()-1];
        }
        return "";
    }

    RDFStatement(raptor_statement* s) {
        graph = toString(s->graph);
        object = toString(s->object);
        predicate = toString(s->predicate);
        subject = toString(s->subject);
    }

    string toString() {
        return "Statement:\n type: "+type+"\n object: "+object+"\n predicate: "+predicate+"\n subject: "+subject;
    }
};

struct RDFdata {
    map<string, vector<RDFStatement> > subjects;
    map<string, map<string, string> > objects;
};

void print_triple(void* data, raptor_statement* rs) {
    auto RDFSubjects = (RDFdata*)data;
    auto s = RDFStatement(rs);
    RDFSubjects->subjects[s.subject].push_back(s);
    //cout << s.toString() << endl;
    RDFSubjects->objects[s.subject][s.object] = s.predicate;
}

void postProcessRDFSubjects(VROntologyPtr onto, RDFdata& data) {
    map<string, VRConceptPtr> concepts;
    map<string, VREntityPtr> entities;
    map<string, VRPropertyPtr> datproperties;
    map<string, VRPropertyPtr> objproperties;
    map<string, VRPropertyPtr> annproperties;

    map<string, vector<RDFStatement> > tmp;
    map<string, vector<RDFStatement> > stack = data.subjects;

    concepts["Thing"] = onto->getConcept("Thing");

    auto postpone = [&](RDFStatement s) {
        tmp[s.subject].push_back(s);
    };

    int maxItr = 5;
    for( int i=0; i<maxItr && stack.size(); i++) {
        for (auto& d : stack) {
            string subject = d.first;
            for (auto& statement : d.second) {
                string object = statement.object;
                string predicate = statement.predicate;
                string type = statement.type;

                auto statprint = [&]() {
                    cout << statement.toString() << endl;
                    cout << concepts.count(subject) << entities.count(subject) << datproperties.count(subject) << objproperties.count(subject) << annproperties.count(subject) << endl;
                    cout << concepts.count(predicate) << entities.count(predicate) << datproperties.count(predicate) << objproperties.count(predicate) << annproperties.count(predicate) << endl;
                    cout << concepts.count(object) << entities.count(object) << datproperties.count(object) << objproperties.count(object) << annproperties.count(object) << endl;
                };

                if (predicate == "type") {
                    if (object == "Ontology") continue;
                    if (object == "Class") { concepts[subject] = VRConcept::create(subject, onto); continue; }
                    if (object == "NamedIndividual") { entities[subject] = VREntity::create(subject); continue; }
                    if (object == "DatatypeProperty") { datproperties[subject] = VRProperty::create(subject); continue; }
                    if (object == "ObjectProperty") { objproperties[subject] = VRProperty::create(subject); continue; }
                    if (object == "AnnotationProperty") { annproperties[subject] = VRProperty::create(subject); continue; }
                }

                if (type == "") { // resolve the subject type of the statement
                    if (concepts.count(subject)) statement.type = "concept";
                    if (entities.count(subject)) statement.type = "entity";
                    if (datproperties.count(subject)) statement.type = "dprop";
                    if (objproperties.count(subject)) statement.type = "oprop";
                    if (annproperties.count(subject)) statement.type = "aprop";
                    if (statement.type == "") { postpone(statement); continue; }
                }

                if (predicate == "type" && type == "entity" && concepts.count(object)) { // Entity(subject) is of type concept(object)
                    entities[subject]->setConcept( concepts[object] );
                    continue;
                }

                if (predicate == "subClassOf" && type == "concept" && concepts.count(object)) { // Concept(subject) is a sub concept of concept(object)
                    concepts[object]->append( concepts[subject] );
                    continue;
                }

                if (predicate == "subPropertyOf") { continue; }
                if (predicate == "inverseOf") { continue; }
                if (predicate == "comment" || predicate == "seeAlso") { continue; }

                if (predicate == "range") { // property(subject) has type(object)
                    if (type == "oprop") { objproperties[subject]->setType(object); continue; }
                    if (type == "dprop") { datproperties[subject]->setType(object); continue; }
                    if (type == "aprop") { annproperties[subject]->setType(object); continue; }
                }

                if (predicate == "domain" && concepts.count(object)) { // property(subject) belongs to concept(object)
                    if (type == "oprop") { concepts[object]->addProperty( objproperties[subject] ); continue; }
                    if (type == "dprop") { concepts[object]->addProperty( datproperties[subject] ); continue; }
                    if (type == "aprop") { concepts[object]->addAnnotation( annproperties[subject] ); continue; }
                }

                if (annproperties.count(predicate)) { // concept(subject) or entity(subject) have an annotation(predicate) with value(object)
                    if (type == "concept" && annproperties[predicate]->type != "") {
                        auto p = annproperties[predicate]->copy(); // copy annotations
                        p->value = object;
                        concepts[subject]->addAnnotation(p);
                        continue;
                    }
                    if (type == "entity" && entities[subject]->concept->getProperty(predicate)) {
                        entities[subject]->set(predicate, object); continue;
                    }
                }

                if (objproperties.count(predicate)) { // concept(subject) or entity(subject) have a property(predicate) with value(object)
                    if (type == "concept") { concepts[subject]->addProperty(predicate, object); continue; }
                    if (type == "entity" && entities.count(object)) {
                        auto pv = entities[subject]->getProperties(predicate);
                        if (pv.size()) { pv[0]->value = object; continue; }
                        auto p = entities[subject]->concept->getProperty(predicate);
                        if (p && p->type != "") { entities[subject]->set(predicate, object); continue; }
                    }
                }

                //statprint();
                postpone(statement);
            }
        }
        stack = tmp;
        tmp.clear();
        cout << "STACK SWAP " << i << " " << stack.size() << endl;
    }

    for (auto c : concepts) onto->addConcept(c.second);
    for (auto e : entities) onto->addInstance(e.second);
    //cout << onto->toString() << endl;
}

void VROntology::open(string path) {
    raptor_world* world = raptor_new_world();
    raptor_parser* rdf_parser = raptor_new_parser(world, "rdfxml");
    unsigned char* uri_string = raptor_uri_filename_to_uri_string(path.c_str());
    raptor_uri* uri = raptor_new_uri(world, uri_string);
    raptor_uri* base_uri = raptor_uri_copy(uri);

    RDFdata RDFSubjects;
    raptor_parser_set_statement_handler(rdf_parser, &RDFSubjects, print_triple);
    raptor_parser_parse_file(rdf_parser, uri, base_uri);
    raptor_free_parser(rdf_parser);

    raptor_free_uri(base_uri);
    raptor_free_uri(uri);
    raptor_free_memory(uri_string);
    raptor_free_world(world);

    postProcessRDFSubjects(shared_from_this(), RDFSubjects);
}

void VROntology::addModule(string mod) {
    if (!library.count(mod)) { cout << "Ontology " << mod << " not found in library " << endl; return; }
    merge(library[mod]);
}

void VROntology::setFlag(string f) { flag = f; }
string VROntology::getFlag() { return flag; }

