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
            bool RDFsubject = false;
            bool RDFobject = false;

            RDFStatement(raptor_statement* s);
            RDFStatement(string g, string o, string p, string s, string t);

            string toString(raptor_term* t);
            string toString();
        };

        struct OWLRestriction {
            string property;
            int min = -1;
            int max = -1;
            string concept;
            string dataRange;
            string someValuesFrom;
            string allValuesFrom;
        };

        map<string, bool> predicate_blacklist;
        map<string, bool> type_blacklist;
        map<string, bool> list_types;

        map<string, vector<RDFStatement> > subjects;
        map<string, map<string, string> > objects;
        map<string, vector<string> > lists;
        map<string, string> list_ends;
        map<string, OWLRestriction> restrictions;

        map<string, VRConceptPtr> concepts;
        map<string, VREntityPtr> entities;
        map<string, VRPropertyPtr> datproperties;
        map<string, VRPropertyPtr> objproperties;
        map<string, VRPropertyPtr> annproperties;

        VROntologyPtr onto;

        void clear();
        bool blacklisted(string& s, map<string, bool>& data);
        VRConceptPtr getConcept(string concept);
        VRPropertyPtr getProperty(string prop);

        void AgglomerateData();
        bool ProcessSubject(RDFStatement& s, vector<RDFStatement>& statements, map<string, vector<RDFStatement> >& stack);

        void printState(RDFStatement& s);
        void printTripleStore();

    public:
        VROWLImport();

        void load(VROntologyPtr o, string path);
        void processTriple(raptor_statement* rs);
};

OSG_END_NAMESPACE;

#endif // VROWLIMPORT_H_INCLUDED
