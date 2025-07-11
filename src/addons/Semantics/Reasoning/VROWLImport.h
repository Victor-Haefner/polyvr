#ifndef VROWLIMPORT_H_INCLUDED
#define VROWLIMPORT_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "../VRSemanticsFwd.h"

#include <map>
#include <vector>
#include <string>

#ifndef WITHOUT_RAPTOR
#include <raptor2/raptor2.h> // http://librdf.org/raptor/api/
#endif

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

#ifndef WITHOUT_RAPTOR
            RDFStatement(raptor_statement* s);
#endif
            RDFStatement(string g, string o, string p, string s, string t);

#ifndef WITHOUT_RAPTOR
            string toString(raptor_term* t);
#endif
            string toString();
        };

        struct OWLRestriction {
            string property;
            int min = -1;
            int max = -1;
            string concept_;
            string dataRange;
            string someValuesFrom;
            string allValuesFrom;
            string hasValue;
        };

        struct OWLAxiom {
            string property;
            int min = -1;
            int max = -1;
            string concept_;
            string dataRange;
            string someValuesFrom;
            string allValuesFrom;
            string hasValue;
        };

        struct OWLList {
            vector<string> entries;
            bool complete = false;
            string listID;
        };

        string ontologyName;
        map<string, bool> predicate_blacklist;
        map<string, bool> type_blacklist;
        map<string, bool> list_types;
        map<string, string> labels;

        map<string, vector<RDFStatement> > subjects;
        map<string, map<string, string> > objects;
        map<string, OWLList> lists;
        map<string, string> list_ends;
        map<string, OWLRestriction> restrictions;
        map<string, OWLAxiom> axioms;
        map<string, string> minInclusives;
        map<string, string> maxInclusives;

        map<string, VRConceptPtr> concepts;
        map<string, VREntityPtr> entities;
        map<string, VRPropertyPtr> datproperties;
        map<string, VRPropertyPtr> objproperties;
        map<string, VRPropertyPtr> annproperties;
        map<string, VROntologyRulePtr> rules;
        map<string, VariablePtr> variables;
        map<string, VRStatementPtr> ruleStatements;

        VROntologyPtr onto;

        void clear();
        bool blacklisted(string& s, map<string, bool>& data);
        VRConceptPtr getConcept(string concept_);
        VRPropertyPtr getProperty(string prop);
        VROntologyRulePtr getRule(string rule);

        void AgglomerateData();
        bool ProcessSubject(RDFStatement& s, vector<RDFStatement>& statements, map<string, vector<RDFStatement> >& stack);

        string whereIs(string s);
        void printState(RDFStatement& s, string ID = "");
        void printTripleStore();

    public:
        VROWLImport();

        void read(VROntologyPtr o, string path);
#ifndef WITHOUT_RAPTOR
        void processTriple(raptor_statement* rs);
#endif
};

OSG_END_NAMESPACE;

#endif // VROWLIMPORT_H_INCLUDED
