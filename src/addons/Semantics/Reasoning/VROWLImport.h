#ifndef VROWLIMPORT_H_INCLUDED
#define VROWLIMPORT_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "../VRSemanticsFwd.h"

#include <map>
#include <vector>
#include <string>

/* no compiling?
    install the raptor2 ubuntu package for rdfxml parsing
    sudo apt-get install libraptor2-dev
*/

#include <raptor2/raptor2.h> // http://librdf.org/raptor/api/

using namespace std;
OSG_BEGIN_NAMESPACE;

class VROWLImport {
    private:
        struct RDFStatement {
            string type;
            string graph;
            string object;
            string predicate;
            string subject;

            RDFStatement(raptor_statement* s);

            string toString(raptor_term* t);
            string toString();
        };

        map<string, bool> blacklist;

        map<string, vector<RDFStatement> > subjects;
        map<string, map<string, string> > objects;

        map<string, VRConceptPtr> concepts;
        map<string, VREntityPtr> entities;
        map<string, VRPropertyPtr> datproperties;
        map<string, VRPropertyPtr> objproperties;
        map<string, VRPropertyPtr> annproperties;

        VROntologyPtr onto;

        void clear();
        bool blacklisted(string s);
        VRConceptPtr getConcept(string concept);
        VRPropertyPtr getProperty(string prop);

        void AgglomerateData();
        bool ProcessSubject(RDFStatement& s);

        void printState(RDFStatement& s);
        void printTripleStore();

    public:
        VROWLImport();

        void load(VROntologyPtr o, string path);
        void processTriple(raptor_statement* rs);
};

OSG_END_NAMESPACE;

#endif // VROWLIMPORT_H_INCLUDED
