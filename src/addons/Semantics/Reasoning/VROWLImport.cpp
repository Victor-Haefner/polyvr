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
    if (s->subject) RDFsubject = (s->subject->type == RAPTOR_TERM_TYPE_BLANK);
    if (s->object) RDFobject = (s->object->type == RAPTOR_TERM_TYPE_BLANK);
}

VROWLImport::RDFStatement::RDFStatement(string g, string o, string p, string s, string t) {
    graph = g;
    object = o;
    predicate = p;
    subject = s;
    type = t;
}

string VROWLImport::RDFStatement::toString(raptor_term* t) {
    if (t == 0) return "";
    switch(t->type) {
        case RAPTOR_TERM_TYPE_LITERAL:
            return string( (const char*)t->value.literal.string );
        case RAPTOR_TERM_TYPE_BLANK:
            return string( (const char*)t->value.blank.string );
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

VROWLImport::VROWLImport() {
    predicate_blacklist["imports"] = 1;
    predicate_blacklist["subPropertyOf"] = 1;
    predicate_blacklist["versionInfo"] = 1;
    predicate_blacklist["inverseOf"] = 1;
    predicate_blacklist["disjointWith"] = 1;
    predicate_blacklist["propertyDisjointWith"] = 1;
    predicate_blacklist["equivalentClass"] = 1;

    //type_blacklist["Restriction"] = 1;
    list_types["unionOf"] = 1;
    list_types["intersectionOf"] = 1;
    list_types["members"] = 1;
}

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

bool VROWLImport::blacklisted(string& s, map<string, bool>& data) {
    if (!data.count(s)) return 0;
    else return data[s];
}

bool VROWLImport::ProcessSubject(RDFStatement& statement, vector<RDFStatement>& statements, map<string, vector<RDFStatement> >& stack) {
    string& type = statement.type;
    string& subject = statement.subject;
    string& predicate = statement.predicate;
    string& object = statement.object;

    //if (subject == "AbstractActor" || predicate == "AbstractActor" || object == "AbstractActor") printState(statement);
    //if (subject == "genid49" || predicate == "genid49" || object == "genid49") printState(statement);

    if (blacklisted(predicate, predicate_blacklist)) return 0;

    if (statement.RDFsubject) {
        if (list_types.count(predicate)) { // RDF list (subject) starting with (object)
            lists[subject] = vector<string>();
            list_ends[object] = subject;
            return 0;
        }

        if (predicate == "first" && list_ends.count(subject)) {
            lists[list_ends[subject]].push_back(object);
            return 0;
        }

        if (predicate == "rest" && list_ends.count(subject)) {
            if (object == "nil") {
                list_ends[ list_ends[subject] ] = "nil";
                return 0;
            }
            list_ends[object] = list_ends[subject];
            return 0;
        }

        if (predicate == "type") {
            if (object == "Restriction") {
                restrictions[subject] = OWLRestriction();
                return 0;
            }
            if (object == "Class") { return 0; } // TODO
            if (object == "AllDisjointClasses") { return 0; } // TODO
        }

        if (predicate == "complementOf") { return 0; } // TODO

        if (restrictions.count(subject)) {
            if (predicate == "onProperty") { restrictions[subject].property = object; return 0; }
            if (predicate == "cardinality") { restrictions[subject].min = restrictions[subject].max = toInt(object); return 0; }
            if (predicate == "qualifiedCardinality") { restrictions[subject].min = restrictions[subject].max = toInt(object); return 0; }
            if (predicate == "minQualifiedCardinality") { restrictions[subject].min = toInt(object); return 0; }
            if (predicate == "maxQualifiedCardinality") { restrictions[subject].max = toInt(object); return 0; }
            if (predicate == "onClass") { restrictions[subject].concept = object; return 0; }
            if (predicate == "onDataRange") { restrictions[subject].dataRange = object; return 0; }
            if (predicate == "someValuesFrom") { restrictions[subject].someValuesFrom = object; return 0; }
            if (predicate == "allValuesFrom") { restrictions[subject].allValuesFrom = object; return 0; }
        }
        return 1;
    }

    if (statement.RDFobject) {
        if (lists.count(object) && list_ends[object] == "nil") { // RDF list fully parsed
            for (auto i : lists[object]) {
                auto s = statement;
                s.object = i; s.RDFobject = 0;
                stack[subject].push_back(s);
            }
            return 0;
        }

        if (predicate == "subClassOf") { // the class(subject) has some specifications on object properties
            if (restrictions.count(object)) { // the class(subject) has a restriction(object) on an object property
                return 0; // TODO
            }
            return 0; // TODO
        }
        return 1;
    }

    if (predicate == "type") {
        if (blacklisted(object, type_blacklist)) return 0;

        if (object == "Ontology") return 0;
        if (object == "Class") { concepts[subject] = VRConcept::create(subject, onto); return 0; }
        if (object == "NamedIndividual") { entities[subject] = VREntity::create(subject); return 0; }

        // properties
        if (object == "DatatypeProperty") { datproperties[subject] = VRProperty::create(subject); return 0; }
        if (object == "AnnotationProperty") { annproperties[subject] = VRProperty::create(subject); return 0; }
        if (object == "ObjectProperty") {
            objproperties[subject] = VRProperty::create(subject);
             // fix missing domain of properties
            bool hasDomain = 0;
            for (auto s : statements) if (s.predicate == "domain") hasDomain = 1;
            if (!hasDomain) { statement = RDFStatement(statement.graph, "Thing", "domain", subject, type); return 1; }
            return 0;
        }

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

    if (type == "concept") {
        if (predicate == "subClassOf") { // Concept(subject) is a sub concept of concept(object)
            if (auto co = getConcept(object))
                if (auto cs = getConcept(subject)) { co->append(cs); return 0; }
        }

        if (annproperties.count(predicate) && annproperties[predicate]->type == "aprop") { // concept(subject) has an annotation(predicate) with value(object)
            auto p = annproperties[predicate]->copy(); // copy annotations
            p->value = object;
            getConcept(subject)->addAnnotation(p);
            return 0;
        }

        if (datproperties.count(predicate) || objproperties.count(predicate)) { // concept(subject) has a property(predicate) with value(object)
            getConcept(subject)->addProperty(predicate, object); return 0;
        }
    }

    if (type == "entity" && entities.count(subject)) {
        if (predicate == "type") // Entity(subject) is of type concept(object)
            if (auto c = getConcept(object)) { entities[subject]->addConcept( c ); return 0; }

        if (annproperties.count(predicate) && annproperties[predicate]->type == "aprop") { // entity(subject) has an annotation(predicate) with value(object)
            if (entities[subject]->getProperty(predicate)) {
                entities[subject]->set(predicate, object); return 0;
            }
        }

        // entity(subject) has a property(predicate) with value(object)
        //auto pv = entities[subject]->getValues(predicate);
        //if (pv.size()) { pv[0]->value = object; return 0; }
        auto p = entities[subject]->getProperty(predicate);
        if (p) {
            //if (p->type == "") cout << "Warning: data property " << predicate << " has no data type!\n";
            entities[subject]->add(predicate, object); return 0;
        }
    }

    if (type == "oprop" && objproperties.count(subject)) {
        if (predicate == "range") { objproperties[subject]->setType(object); return 0; }
        if (predicate == "domain") if (auto c = getConcept(object)) { c->addProperty( objproperties[subject] ); return 0; }
    }

    if (type == "dprop" && datproperties.count(subject)) {
        if (predicate == "range") { datproperties[subject]->setType(object); return 0; }
        if (predicate == "domain") if (auto c = getConcept(object)) { c->addProperty( datproperties[subject] ); return 0; }
    }

    if (type == "aprop" && annproperties.count(subject)) {
        if (predicate == "range") { annproperties[subject]->setType(object); return 0; }
        if (predicate == "domain") if (auto c = getConcept(object)) { c->addAnnotation( annproperties[subject] ); return 0; }
    }

    // local properties
    if (annproperties.count(predicate) && annproperties[predicate]->type == "aprop") { // concept(subject) or entity(subject) or property(subject) have an annotation(predicate) with value(object)
        // must be property
        if (auto p = getProperty(subject)) {
            return 0; // TODO
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
                if (ProcessSubject(statement, d.second, tmp))
                    tmp[statement.subject].push_back(statement);
            }
        }
        stack = tmp;
        tmp.clear();
        int jobs = jobSize();
        cout << "RDF parser: iteration: " << i << " with " << jobs << " triplets remaining" << endl;

        if (int(stack.size()) == lastStack && jobs == lastJobSize) {
            cout << "RDF parser warning: stack not shrinking, aborting with " << jobs << " triplets remaining!" << endl;
            for (auto& sv : stack) for (auto& s : sv.second) cout << " " << s.toString() << endl;
            break;
        } else {
            lastStack = stack.size();
            lastJobSize = jobs;
        }
    }

    for (auto c : concepts) onto->addConcept(c.second);
    for (auto e : entities) onto->addEntity(e.second);
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


