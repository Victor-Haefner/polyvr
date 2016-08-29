#include "VROWLImport.h"
#include "VROntology.h"
#include "VRProperty.h"
#include "core/utils/toString.h"

/* no compiling?
    install the raptor2 ubuntu package for rdfxml parsing
    sudo apt-get install libraptor2-dev
*/

#include <raptor2/raptor2.h> // http://librdf.org/raptor/api/

using namespace OSG;

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
        return "Statement: type "+type+"  predicate "+predicate+"  subject "+subject+"  object "+object;
    }
};

struct RDFdata {
    map<string, vector<RDFStatement> > subjects;
    map<string, map<string, string> > objects;
};

VROWLImport::VROWLImport() {}


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

    auto getConcept = [&](string concept) {
        if (concepts.count(concept)) return concepts[concept];
        if (auto c = onto->getConcept(concept)) return c;
        return VRConceptPtr();
    };

    auto getProperty = [&](string prop) {
        if (datproperties.count(prop)) return datproperties[prop];
        if (objproperties.count(prop)) return objproperties[prop];
        if (annproperties.count(prop)) return annproperties[prop];
        //if (auto p = onto->getProperty(prop)) return p;
        return VRPropertyPtr();
    };

    int lastStack = 0;
    for( int i=0; stack.size(); i++) {
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

                //if (subject == "hasModelComponentLable") statprint();
                if (subject == "DefaultValue_DefaultAction") statprint();

                if (predicate == "BLANK" || object == "BLANK" || subject == "BLANK") continue;
                if (predicate == "subPropertyOf") continue;
                if (predicate == "inverseOf") continue;
                if (predicate == "disjointWith") continue;
                if (predicate == "equivalentClass") continue;
                if (predicate == "versionInfo") continue;
                if (predicate == "imports") continue;
                if (predicate == "comment" || predicate == "label" || predicate == "seeAlso") continue; // build-in annotations
                //if (predicate == "first" || predicate == "rest" || predicate == "unionOf") continue;

                if (predicate == "type") {
                    if (object == "Ontology") continue;
                    if (object == "Class") { concepts[subject] = VRConcept::create(subject, onto); continue; }
                    if (object == "NamedIndividual") { entities[subject] = VREntity::create(subject); continue; }
                    if (object == "DatatypeProperty") { datproperties[subject] = VRProperty::create(subject); continue; }
                    if (object == "ObjectProperty") { objproperties[subject] = VRProperty::create(subject); continue; }
                    if (object == "AnnotationProperty") { annproperties[subject] = VRProperty::create(subject); continue; }

                    // other object properties
                    if (object == "FunctionalProperty") { objproperties[subject] = VRProperty::create(subject); continue; }
                    if (object == "InverseFunctionalProperty") { objproperties[subject] = VRProperty::create(subject); continue; }
                    if (object == "IrreflexiveProperty") { objproperties[subject] = VRProperty::create(subject); continue; }
                }

                if (type == "") { // resolve the subject type of the statement
                    if (getConcept(subject)) statement.type = "concept";
                    if (entities.count(subject)) statement.type = "entity";
                    if (datproperties.count(subject)) statement.type = "dprop";
                    if (objproperties.count(subject)) statement.type = "oprop";
                    if (annproperties.count(subject)) statement.type = "aprop";
                    if (statement.type == "") { postpone(statement); continue; }
                }

                if (predicate == "type" && type == "entity") { // Entity(subject) is of type concept(object)
                    if (auto c = getConcept(object)) { entities[subject]->setConcept( c ); continue; }
                }

                if (predicate == "subClassOf" && type == "concept") { // Concept(subject) is a sub concept of concept(object)
                    if (auto co = getConcept(object))
                        if (auto cs = getConcept(subject)) { co->append(cs); continue; }
                }

                if (predicate == "range") { // property(subject) has type(object)
                    if (type == "oprop") { objproperties[subject]->setType(object); continue; }
                    if (type == "dprop") { datproperties[subject]->setType(object); continue; }
                    if (type == "aprop") { annproperties[subject]->setType(object); continue; }
                }

                if (predicate == "domain") { // property(subject) belongs to concept(object)
                    if (auto c = getConcept(object)) {
                        if (type == "oprop") { c->addProperty( objproperties[subject] ); continue; }
                        if (type == "dprop") { c->addProperty( datproperties[subject] ); continue; }
                        if (type == "aprop") { c->addAnnotation( annproperties[subject] ); continue; }
                    }
                }

                if (annproperties.count(predicate)) { // concept(subject) or entity(subject) have an annotation(predicate) with value(object)
                    if (type == "concept" && annproperties[predicate]->type != "") {
                        auto p = annproperties[predicate]->copy(); // copy annotations
                        p->value = object;
                        getConcept(subject)->addAnnotation(p);
                        continue;
                    }
                    if (type == "entity" && entities[subject]->getConcept()->getProperty(predicate)) {
                        entities[subject]->set(predicate, object); continue;
                    }
                }

                if (objproperties.count(predicate)) { // concept(subject) or entity(subject) have a property(predicate) with value(object)
                    if (type == "concept") { getConcept(subject)->addProperty(predicate, object); continue; }
                    if (type == "entity" && entities.count(subject)) {
                        auto pv = entities[subject]->getProperties(predicate);
                        if (pv.size()) { pv[0]->value = object; continue; }
                        auto p = entities[subject]->getConcept()->getProperty(predicate);
                        if (p && p->type != "") { entities[subject]->set(predicate, object); continue; }
                    }
                }

                if (datproperties.count(predicate)) { // concept(subject) or entity(subject) have a property(predicate) with value(object)
                    if (type == "concept") { getConcept(subject)->addProperty(predicate, object); continue; }
                    if (type == "entity" && entities.count(subject)) {
                        auto pv = entities[subject]->getProperties(predicate);
                        if (pv.size()) { pv[0]->value = object; continue; }
                        auto p = entities[subject]->getConcept()->getProperty(predicate);
                        if (p && p->type != "") { entities[subject]->set(predicate, object); continue; }
                    }
                }

                //statprint();
                postpone(statement);
            }
        }
        stack = tmp;
        tmp.clear();
        cout << "RDF parser: iteration: " << i << " with " << stack.size() << " triplets remaining" << endl;

        if (stack.size() == lastStack) {
            cout << "RDF parser warning: stack not shrinking, aborting with " << stack.size() << " triplets remaining!" << endl;
            for (auto& sv : stack) for (auto& s : sv.second) cout << " " << s.toString() << endl;
            break;
        } else lastStack = stack.size();
    }

    for (auto c : concepts) onto->addConcept(c.second);
    for (auto e : entities) onto->addInstance(e.second);
    //cout << onto->toString() << endl;
}

void VROWLImport::load(VROntologyPtr o, string path) {
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

    postProcessRDFSubjects(o, RDFSubjects);
}


