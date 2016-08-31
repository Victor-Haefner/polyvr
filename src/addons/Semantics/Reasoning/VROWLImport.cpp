#include "VROWLImport.h"
#include "VROntology.h"
#include "VRProperty.h"
#include "core/utils/toString.h"

using namespace OSG;

VROWLImport::RDFStatement::RDFStatement(raptor_statement* s) {
    graph = toString(s->graph);
    object = toString(s->object);
    predicate = toString(s->predicate);
    subject = toString(s->subject);
}

string VROWLImport::RDFStatement::toString(raptor_term* t) {
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
            if (ss.size() == 1) ss = splitString(s, '/');
            //raptor_free_memory(uri);
            return ss[ss.size()-1];
    }
    return "";
}

string VROWLImport::RDFStatement::toString() {
    return "Statement: type "+type+"  predicate "+predicate+"  subject "+subject+"  object "+object;
}

VROWLImport::VROWLImport() {}

void VROWLImport::clear() {
    subjects.clear();
    objects.clear();
    concepts.clear();
    entities.clear();
    datproperties.clear();
    objproperties.clear();
    annproperties.clear();
    onto.reset();

    annproperties["label"] = VRProperty::create("label");
    annproperties["comment"] = VRProperty::create("comment");
    annproperties["label"]->type = "aprop";
    annproperties["comment"]->type = "aprop";

    blacklist["subPropertyOf"] = 1;
    blacklist["inverseOf"] = 1;
    blacklist["disjointWith"] = 1;
    blacklist["propertyDisjointWith"] = 1;
    blacklist["equivalentClass"] = 1;
    blacklist["versionInfo"] = 1;
    blacklist["imports"] = 1;
}

void VROWLImport::printState(RDFStatement& s) {
    string& object = s.object;
    string& predicate = s.predicate;
    string& subject = s.subject;
    cout << s.toString() << endl;
    cout << concepts.count(subject) << entities.count(subject) << datproperties.count(subject) << objproperties.count(subject) << annproperties.count(subject) << endl;
    cout << concepts.count(predicate) << entities.count(predicate) << datproperties.count(predicate) << objproperties.count(predicate) << annproperties.count(predicate) << endl;
    cout << concepts.count(object) << entities.count(object) << datproperties.count(object) << objproperties.count(object) << annproperties.count(object) << endl;
}

void VROWLImport::printTripleStore() {
    for (auto& sv : subjects) {
        for (auto& st : sv.second) {
            string s = "Statement:";
            s += " subject " + st.subject;
            s += " object " + st.object;
            s += " predicate " + st.predicate;
            cout << s << endl;
        }
    }
}

bool VROWLImport::blacklisted(string s) {
    if (!blacklist.count(s)) return 0;
    else return blacklist[s];
}

bool VROWLImport::ProcessSubject(RDFStatement& statement) {
    string& type = statement.type;
    string& subject = statement.subject;
    string& predicate = statement.predicate;
    string& object = statement.object;

    //if (subject == "hasModelComponentLable") statprint();
    //if (subject == "DefaultValue_DefaultAction") printState(statement);

    if (predicate == "BLANK" || object == "BLANK" || subject == "BLANK") return 0; // TODO?
    if (blacklisted(predicate)) return 0;

    if (predicate == "type") {
        if (object == "Ontology") return 0;
        if (object == "Class") { concepts[subject] = VRConcept::create(subject, onto); return 0; }
        if (object == "NamedIndividual") { entities[subject] = VREntity::create(subject); return 0; }
        if (object == "DatatypeProperty") { datproperties[subject] = VRProperty::create(subject); return 0; }
        if (object == "ObjectProperty") { objproperties[subject] = VRProperty::create(subject); return 0; }
        if (object == "AnnotationProperty") { annproperties[subject] = VRProperty::create(subject); return 0; }

        // other object properties
        if (object == "FunctionalProperty") { objproperties[subject] = VRProperty::create(subject); return 0; }
        if (object == "InverseFunctionalProperty") { objproperties[subject] = VRProperty::create(subject); return 0; }
        if (object == "IrreflexiveProperty") { objproperties[subject] = VRProperty::create(subject); return 0; }
    }

    if (type == "") { // resolve the subject type of the statement
        if (getConcept(subject)) statement.type = "concept";
        if (entities.count(subject)) statement.type = "entity";
        if (datproperties.count(subject)) statement.type = "dprop";
        if (objproperties.count(subject)) statement.type = "oprop";
        if (annproperties.count(subject)) statement.type = "aprop";
        if (statement.type == "") return 1;
    }

    if (predicate == "type" && type == "entity") { // Entity(subject) is of type concept(object)
        if (auto c = getConcept(object)) { entities[subject]->addConcept( c ); return 0; }
    }

    if (predicate == "subClassOf" && type == "concept") { // Concept(subject) is a sub concept of concept(object)
        if (auto co = getConcept(object))
            if (auto cs = getConcept(subject)) { co->append(cs); return 0; }
    }

    if (predicate == "range") { // property(subject) has type(object)
        printState(statement);
        if (type == "oprop") { objproperties[subject]->setType(object); return 0; }
        if (type == "dprop") { datproperties[subject]->setType(object); return 0; }
        if (type == "aprop") { annproperties[subject]->setType(object); return 0; }
    }

    if (predicate == "domain") { // property(subject) belongs to concept(object)
        if (auto c = getConcept(object)) {
            if (type == "dprop" && datproperties.count(subject)) { c->addProperty( datproperties[subject] ); return 0; }
            if (type == "oprop" && objproperties.count(subject)) { c->addProperty( objproperties[subject] ); return 0; }
            if (type == "aprop" && annproperties.count(subject)) { c->addAnnotation( annproperties[subject] ); return 0; }
        }
    }

    if (annproperties.count(predicate) && annproperties[predicate]->type == "aprop") { // concept(subject) or entity(subject) or property(subject) have an annotation(predicate) with value(object)
        if (type == "concept") {
            auto p = annproperties[predicate]->copy(); // copy annotations
            p->value = object;
            getConcept(subject)->addAnnotation(p);
            return 0;
        }

        if (type == "entity" && entities[subject]->getProperty(predicate)) {
            entities[subject]->set(predicate, object); return 0;
        }

        // must be property
        if (auto p = getProperty(subject)) {
            return 0; // TODO
        }
    }

    if (objproperties.count(predicate)) { // concept(subject) or entity(subject) have a property(predicate) with value(object)
        if (type == "concept") { getConcept(subject)->addProperty(predicate, object); return 0; }
        if (type == "entity" && entities.count(subject)) {
            auto pv = entities[subject]->getValues(predicate);
            if (pv.size()) { pv[0]->value = object; return 0; }
            auto p = entities[subject]->getProperty(predicate);
            if (p && p->type != "") { entities[subject]->set(predicate, object); return 0; }
        }
    }

    if (datproperties.count(predicate)) { // concept(subject) or entity(subject) have a property(predicate) with value(object)
        if (type == "concept") { getConcept(subject)->addProperty(predicate, object); return 0; }
        if (type == "entity" && entities.count(subject)) {
            auto pv = entities[subject]->getValues(predicate);
            if (pv.size()) { pv[0]->value = object; return 0; }
            auto p = entities[subject]->getProperty(predicate);
            if (p) {
                if (p->type == "") cout << "Warning: data property " << predicate << " has no data type!\n";
                entities[subject]->set(predicate, object); return 0;
            }
        }
    }

    return 1;
}

VRConceptPtr VROWLImport::getConcept(string concept) {
    if (concepts.count(concept)) return concepts[concept];
    if (auto c = onto->getConcept(concept)) return c;
    return VRConceptPtr();
};

VRPropertyPtr VROWLImport::getProperty(string prop) {
    if (datproperties.count(prop)) return datproperties[prop];
    if (objproperties.count(prop)) return objproperties[prop];
    if (annproperties.count(prop)) return annproperties[prop];
    //if (auto p = onto->getProperty(prop)) return p;
    return VRPropertyPtr();
};

void VROWLImport::AgglomerateData() {
    map<string, vector<RDFStatement> > tmp;
    map<string, vector<RDFStatement> > stack = subjects;

    concepts["Thing"] = onto->getConcept("Thing");

    auto postpone = [&](RDFStatement s) {
        tmp[s.subject].push_back(s);
    };

    auto jobSize = [&]() {
        int i=0;
        for (auto s : stack) i += s.second.size();
        return i;
    };

    int lastStack = 0;
    int lastJobSize = 0;
    for( int i=0; stack.size(); i++) {
        for (auto& d : stack) {
            string subject = d.first;
            for (auto& statement : d.second) {
                if (ProcessSubject(statement)) postpone(statement);
            }
        }
        stack = tmp;
        tmp.clear();
        int jobs = jobSize();
        cout << "RDF parser: iteration: " << i << " with " << jobs << " triplets remaining" << endl;

        if (stack.size() == lastStack && jobs == lastJobSize) {
            cout << "RDF parser warning: stack not shrinking, aborting with " << jobs << " triplets remaining!" << endl;
            for (auto& sv : stack) for (auto& s : sv.second) cout << " " << s.toString() << endl;
            break;
        } else {
            lastStack = stack.size();
            lastJobSize = jobs;
        }
    }

    for (auto c : concepts) onto->addConcept(c.second);
    for (auto e : entities) onto->addInstance(e.second);
    //cout << onto->toString() << endl;
}

void processTriple(void* mgr, raptor_statement* rs) {
    auto m = (VROWLImport*)mgr;
    m->processTriple(rs);
}

void VROWLImport::processTriple(raptor_statement* rs) {
    auto s = RDFStatement(rs);
    subjects[s.subject].push_back(s);
    objects[s.subject][s.object] = s.predicate;
}

void VROWLImport::load(VROntologyPtr o, string path) {
    clear();
    raptor_world* world = raptor_new_world();
    raptor_parser* rdf_parser = raptor_new_parser(world, "rdfxml");
    unsigned char* uri_string = raptor_uri_filename_to_uri_string(path.c_str());
    raptor_uri* uri = raptor_new_uri(world, uri_string);
    raptor_uri* base_uri = raptor_uri_copy(uri);

    raptor_parser_set_statement_handler(rdf_parser, this, ::processTriple);
    raptor_parser_parse_file(rdf_parser, uri, base_uri);
    raptor_free_parser(rdf_parser);

    raptor_free_uri(base_uri);
    raptor_free_uri(uri);
    raptor_free_memory(uri_string);
    raptor_free_world(world);

    onto = o;
    AgglomerateData();
    //printTripleStore();
}


