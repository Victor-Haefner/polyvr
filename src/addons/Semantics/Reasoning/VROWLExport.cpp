#include "VROWLExport.h"
#include "VROntology.h"
#include "core/utils/xml.h"
#include "core/utils/system/VRSystem.h"

#include <iostream>

using namespace OSG;


VROWLExport::VROWLExport() {}

void VROWLExport::write(VROntologyPtr o, string path) {
    XML xml;
    XMLElementPtr root = xml.newRoot("rdf:RDF", "", "rdf"); // name, ns_uri, ns_prefix

    string ontology_ns = "http://www.semanticweb.org/ontologies/2012/9/"+o->getName()+".owl#";
    string dp_ns = "http://www.w3.org/2001/XMLSchema#";

    map<string, VRPropertyPtr> properties;
    map<string, XMLElementPtr> propertyNodes;
    for (auto c : o->getConcepts()) {
        for (auto p : c->getProperties()) {
            properties[p->getName()] = p;
        }
    }

    auto isDataProperty = [](string type) {
        if (type == "float") return true;
        if (type == "string") return true;
        if (type == "int") return true;
        return false;
    };

    auto writeHeader = [&]() {
        root->setAttribute("xmlns", ontology_ns);
        root->setAttribute("xmlns:base", ontology_ns);
        root->setAttribute("xmlns:"+o->getName(), ontology_ns);
        root->setAttribute("xmlns:rdf", "http://www.w3.org/1999/02/22-rdf-syntax-ns#");
        root->setAttribute("xmlns:rdfs", "http://www.w3.org/2000/01/rdf-schema#");
        root->setAttribute("xmlns:xsd", "http://www.w3.org/2001/XMLSchema#");
        root->setAttribute("xmlns:owl", "http://www.w3.org/2002/07/owl#");
        root->setAttribute("xmlns:swrl", "http://www.w3.org/2003/11/swrl#");
        root->setAttribute("xmlns:protege", "http://protege.stanford.edu/plugins/owl/protege#");
        root->setAttribute("xmlns:xsp", "http://www.owl-ontologies.com/2005/08/07/xsp.owl#");
        root->setAttribute("xmlns:swrlb", "http://www.w3.org/2003/11/swrlb#");

        auto o = root->addChild("owl:Ontology");
        o->setAttribute("rdf:about", ontology_ns);
    };

    auto writeProperties = [&]() {
        for (auto prop : properties) {
            string ptag = "owl:ObjectProperty";
            string pns = ontology_ns;

            if (isDataProperty(prop.second->type)) {
                ptag = "owl:DataProperty";
                pns = dp_ns;
            }

            auto a = root->addChild(ptag);
            a->setAttribute("rdf:about", ontology_ns + prop.first);
            auto r = a->addChild("rdfs:range");
            r->setAttribute("rdf:resource", pns + prop.second->type);
            propertyNodes[prop.first] = a;
        }
    };

    auto writeClasses = [&]() {
        for (auto concept : o->getConcepts()) {
            if (concept->getName() == "Thing") continue;
            auto a = root->addChild("owl:Class");
            a->setAttribute("rdf:about", ontology_ns + concept->getName());    // concept name
            for (auto cp : concept->getParents()) {                             // concept parents
                if (cp->getName() == "Thing") continue;
                auto p = a->addChild("rdfs:subClassOf");
                p->setAttribute("rdf:resource", ontology_ns + cp->getName());
            }

            for (auto p : concept->getProperties(false)) {
                auto d = propertyNodes[p->getName()]->addChild("rdfs:domain");
                d->setAttribute("rdf:resource", ontology_ns + concept->getName());
            }
        }
    };

    auto writeRules = [&]() { // TODO
        ;
    };

    auto writeInstances = [&]() {
        for (auto entity : o->entitiesByName) {
            auto e = root->addChild("owl:NamedIndividual");
            e->setAttribute("rdf:about", ontology_ns + entity.first);
            auto t = e->addChild("rdf:type");
            t->setAttribute("rdf:resource", ontology_ns + entity.second->getConcept()->getName());
            for (auto prop : entity.second->getAll()) {
                auto p = e->addChild(prop->getName());
                if (isDataProperty(prop->type)) {
                    p->setText(prop->getValue());
                    p->setAttribute("rdf:datatype", dp_ns + prop->getType());
                } else {
                    p->setAttribute("rdf:resource", ontology_ns + prop->getValue());
                }
            }
        }
    };

    writeHeader();
    writeProperties();
    writeClasses();
    writeRules();
    writeInstances();

    if (exists(path)) path = canonical(path);
    cout << "VROWLExport::write to " << path << endl;
    xml.write(path);
}
