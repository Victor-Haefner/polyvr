#ifndef VRNAME_H_INCLUDED
#define VRNAME_H_INCLUDED

#include <string>
#include "VRStorage.h"

// forward declarations
namespace xmlpp{ class Element; }

using namespace std;

struct VRNamePool;

struct VRNameSpace {
    string nspace = "__global__";
    bool unique = true;
    char separator = '.';
    string filter;
    char filter_rep;
    map<string, VRNamePool> nameDict; // key is base name

    VRNameSpace(string nspace = "");

    int getSuffix(const string& base, const int& hint = -1);
    void removeName(const string& base, const int& suffix);
    string compileName(const string& base, const int& suffix);
    int getNameNumber();
    void print();
    void applyFilter(string& name);

    void setSeparator(char s);
    void setUniqueNames(bool b);
    void filterNameChars(string chars, char replacement);
};

OSG_BEGIN_NAMESPACE;

class VRName : public VRStorage {
    protected:
        string name;
        string base_name;
        int name_suffix = 0;
        string nameSpaceName = "__global__";
        VRNameSpace* nameSpace = 0;

    public:
        VRName();
        ~VRName();

        string setName(string name);
        VRNameSpace* setNameSpace(string s);
        VRNameSpace* resetNameSpace();

        string getName();
        string getBaseName();
        int getNameSuffix();
        VRNameSpace* getNameSpace();

        void compileName();

        static void printInternals();
};

OSG_END_NAMESPACE;

#endif // VRNAME_H_INCLUDED
