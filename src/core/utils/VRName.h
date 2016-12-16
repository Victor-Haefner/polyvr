#ifndef VRNAME_H_INCLUDED
#define VRNAME_H_INCLUDED

#include <string>
#include "VRStorage.h"

// forward declarations
namespace xmlpp{ class Element; }

using namespace std;

class VRName_base {
    protected:
        string name;
        string base_name;
        int name_suffix = 0;
        bool unique = true;
        char separator = '.';
        string nameSpace = "__global__";
        string filter;
        char filter_rep;

    public:
        VRName_base();
        ~VRName_base();

        void compileName();
        string setName(string name);
        string getName();
        string getBaseName();
        int getNameSuffix();
        void setSeparator(char s);
        void setNameSpace(string s);
        void setUniqueName(bool b);
        void filterNameChars(string chars, char replacement);

        void saveName(xmlpp::Element* e);
        void loadName(xmlpp::Element* e);

        static int getNameNumber();
        static int getBaseNameNumber();
        static void printNameDict();
};

class VRName : public OSG::VRStorage, public VRName_base {
    public:
        VRName();
        ~VRName();
};

#endif // VRNAME_H_INCLUDED
