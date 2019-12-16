#include "VROWLExport.h"
#include "VROntology.h"
#include "core/utils/system/VRSystem.h"

#include <libxml++/libxml++.h>
#include <libxml++/nodes/element.h>
#include <iostream>

using namespace OSG;


VROWLExport::VROWLExport() {}

void VROWLExport::write(VROntologyPtr o, string path) {
    xmlpp::Document doc;
    XMLElementPtr root = doc.create_root_node("rdf:RDF", "", "rdf"); // name, ns_uri, ns_prefix

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
        root->set_attribute("xmlns", ontology_ns);
        root->set_attribute("xmlns:base", ontology_ns);
        root->set_attribute("xmlns:"+o->getName(), ontology_ns);
        root->set_attribute("xmlns:rdf", "http://www.w3.org/1999/02/22-rdf-syntax-ns#");
        root->set_attribute("xmlns:rdfs", "http://www.w3.org/2000/01/rdf-schema#");
        root->set_attribute("xmlns:xsd", "http://www.w3.org/2001/XMLSchema#");
        root->set_attribute("xmlns:owl", "http://www.w3.org/2002/07/owl#");
        root->set_attribute("xmlns:swrl", "http://www.w3.org/2003/11/swrl#");
        root->set_attribute("xmlns:protege", "http://protege.stanford.edu/plugins/owl/protege#");
        root->set_attribute("xmlns:xsp", "http://www.owl-ontologies.com/2005/08/07/xsp.owl#");
        root->set_attribute("xmlns:swrlb", "http://www.w3.org/2003/11/swrlb#");

        auto o = root->add_child("owl:Ontology");
        o->set_attribute("rdf:about", ontology_ns);
    };

    auto writeProperties = [&]() {
        for (auto prop : properties) {
            string ptag = "owl:ObjectProperty";
            string pns = ontology_ns;

            if (isDataProperty(prop.second->type)) {
                ptag = "owl:DataProperty";
                pns = dp_ns;
            }

            auto a = root->add_child(ptag);
            a->set_attribute("rdf:about", ontology_ns + prop.first);
            auto r = a->add_child("rdfs:range");
            r->set_attribute("rdf:resource", pns + prop.second->type);
            propertyNodes[prop.first] = a;
        }
    };

    auto writeClasses = [&]() {
        for (auto concept : o->getConcepts()) {
            if (concept->getName() == "Thing") continue;
            auto a = root->add_child("owl:Class");
            a->set_attribute("rdf:about", ontology_ns + concept->getName());    // concept name
            for (auto cp : concept->getParents()) {                             // concept parents
                if (cp->getName() == "Thing") continue;
                auto p = a->add_child("rdfs:subClassOf");
                p->set_attribute("rdf:resource", ontology_ns + cp->getName());
            }

            for (auto p : concept->getProperties(false)) {
                auto d = propertyNodes[p->getName()]->add_child("rdfs:domain");
                d->set_attribute("rdf:resource", ontology_ns + concept->getName());
            }
        }
    };

    auto writeRules = [&]() { // TODO
        ;
    };

    auto writeInstances = [&]() {
        for (auto entity : o->entitiesByName) {
            auto e = root->add_child("owl:NamedIndividual");
            e->set_attribute("rdf:about", ontology_ns + entity.first);
            auto t = e->add_child("rdf:type");
            t->set_attribute("rdf:resource", ontology_ns + entity.second->getConcept()->getName());
            for (auto prop : entity.second->getAll()) {
                auto p = e->add_child(prop->getName());
                if (isDataProperty(prop->type)) {
                    p->add_child_text(prop->getValue());
                    p->set_attribute("rdf:datatype", dp_ns + prop->getType());
                } else {
                    p->set_attribute("rdf:resource", ontology_ns + prop->getValue());
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
    doc.write_to_file_formatted(path);
}
