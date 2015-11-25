#include "VROntology.h"
#include "VRReasoner.h"
#include "VRProperty.h"
#include "core/utils/toString.h"

/* no compiling?
    install the raptor2 ubuntu package for rdfxml parsing
    sudo apt-get install libraptor2-dev
*/

#include <raptor2/raptor2.h> // http://librdf.org/raptor/api/
#include <iostream>

VROntology::VROntology() {
    thing = VRConcept::create("Thing");
}

VROntologyPtr VROntology::create() { return VROntologyPtr( new VROntology() ); }

VRConceptPtr VROntology::getConcept(string name, VRConceptPtr p) {
    if (p == 0) p = thing;
    if (p->name == name) return p;
    VRConceptPtr c = 0;
    for (auto ci : p->children) {
        c = getConcept(name, ci.second);
        if (c) return c;
    }
    return c;
}

vector<VRConceptPtr> VROntology::getConcepts() {
    vector<VRConceptPtr> res;
    for (auto c : concepts) res.push_back(c.second.lock());
    return res;
}

VRConceptPtr VROntology::addConcept(string concept, string parent) {
    if (parent == "") return thing->append(concept);
    auto p = getConcept(parent);
    if (p == 0) { cout << "WARNING in VROntology::addConcept, " << parent << " not found while adding " << concept << "!\n"; return 0;  }
    return getConcept(parent)->append(concept);
}

void VROntology::addConcept(VRConceptPtr c) { thing->append(c); }

string VROntology::answer(string question) {
    auto res = VRReasoner::get()->process(question, this);
    return "";//res.toString();
}

void VROntology::merge(VROntology* o) {
    for (auto c : o->thing->children)
        thing->append(c.second);
    for (auto c : o->rules)
        rules[c.first] = c.second;
}

vector<VROntologyRule*> VROntology::getRules() {
    vector<VROntologyRule*> res;
    for (auto r : rules) res.push_back(r.second);
    return res;
}

VROntologyRule* VROntology::addRule(string rule) {
    VROntologyRule* r = new VROntologyRule(rule);
    rules[r->ID] = r;
    return r;
}

VREntityPtr VROntology::addVectorInstance(string name, string concept, string x, string y, string z) {
    auto i = addInstance(name, concept);
    i->set("x", x);
    i->set("y", y);
    i->set("z", z);
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
    for (auto i : instances) if (i.second->name == instance) return i.second;
    return 0;
}

vector<VREntityPtr> VROntology::getInstances(string concept) {
    vector<VREntityPtr> res;
    for (auto i : instances) if (i.second->concept->is_a(concept)) res.push_back(i.second);
    return res;
}

string VROntology::toString() {
    return thing->toString();
}

// RDF import

struct RDFStatement {
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
        return "Statement:\n graph: "+graph+"\n object: "+object+"\n predicate: "+predicate+"\n subject: "+subject;
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

void postProcessRDFSubjects(VROntology* onto, RDFdata& data) {
    map<string, VRConceptPtr> concepts;
    map<string, VREntityPtr> entities;
    map<string, VRPropertyPtr> datproperties;
    map<string, VRPropertyPtr> objproperties;
    map<string, VRPropertyPtr> annproperties;

    map<string, vector<RDFStatement> > tmp;
    map<string, vector<RDFStatement> > stack = data.subjects;

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

                auto statprint = [&]() {
                    cout << statement.toString() << endl;
                    cout << concepts.count(subject) << entities.count(subject) << datproperties.count(subject) << objproperties.count(subject) << annproperties.count(subject) << endl;
                    cout << concepts.count(predicate) << entities.count(predicate) << datproperties.count(predicate) << objproperties.count(predicate) << annproperties.count(predicate) << endl;
                    cout << concepts.count(object) << entities.count(object) << datproperties.count(object) << objproperties.count(object) << annproperties.count(object) << endl;
                };

                if (predicate == "type") {
                    if (object == "Ontology") continue;
                    if (object == "Class") { concepts[subject] = VRConcept::create(subject); continue; }
                    if (object == "NamedIndividual") { entities[subject] = VREntity::create(subject); continue; }
                    if (object == "DatatypeProperty") { datproperties[subject] = VRProperty::create(subject); continue; }
                    if (object == "ObjectProperty") { objproperties[subject] = VRProperty::create(subject); continue; }
                    if (object == "AnnotationProperty") { annproperties[subject] = VRProperty::create(subject); continue; }

                    if (!entities.count(subject)) { postpone(statement); continue; }
                    if (!concepts.count(object)) { postpone(statement); continue; }
                    entities[subject]->setConcept( concepts[object] );
                    continue;
                }

                if (predicate == "subClassOf") {
                    if (!concepts.count(subject)) { postpone(statement); continue; }
                    if (!concepts.count(object)) { postpone(statement); continue; }
                    concepts[object]->append(concepts[subject]);
                    continue;
                }

                if (predicate == "subPropertyOf") {
                    continue;
                }

                if (predicate == "inverseOf") {
                    continue;
                }

                if (predicate == "range") {
                    if (!datproperties.count(subject)) { postpone(statement); continue; }
                    datproperties[subject]->setType(object);
                    continue;
                }

                if (predicate == "domain") {
                    if (!objproperties.count(subject) && !annproperties.count(subject)) { postpone(statement); continue; }
                    if (annproperties.count(subject)) annproperties[subject]->setType(object);
                    else objproperties[subject]->setType(object);
                    continue;
                }

                if (predicate == "comment" || predicate == "seeAlso") { continue; }

                if (annproperties.count(predicate)) {
                    if (!concepts.count(subject) && !entities.count(subject)) { postpone(statement); continue; }
                    if (concepts.count(subject)) {
                        concepts[subject]->addProperty(predicate, object);
                    } else {
                        if (!entities[subject]->concept->getProperty(predicate)) { postpone(statement); continue; }
                        entities[subject]->set(predicate, object);
                    }
                    continue;
                }

                if (objproperties.count(predicate)) {
                    if (!entities.count(subject)) { postpone(statement); continue; }
                    if (!entities.count(object)) { postpone(statement); continue; }
                    if (!entities[subject]->concept->getProperty(predicate)) { postpone(statement); continue; }
                    entities[subject]->set(predicate, object);
                    continue;
                }

                postpone(statement);
            }
        }
        stack = tmp;
        tmp.clear();
        cout << "STACK SWAP " << i << " " << stack.size() << endl;
    }

    for (auto c : concepts) if (!c.second->parent.lock()) onto->addConcept(c.second);
    for (auto e : entities) onto->addInstance(e.second);

    // cout << onto->toString() << endl;
    // TODO: pack ontology and print!
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

    postProcessRDFSubjects(this, RDFSubjects);
}
