#ifndef VRNAME_H_INCLUDED
#define VRNAME_H_INCLUDED

#include <string>

// forward declarations
namespace xmlpp{ class Element; }

using namespace std;

class VRName {
    private:
        int name_suffix;
        void compileName();
        char separator;
        string nameSpace;

    protected:
        string name;
        string base_name;

    public:
        VRName();
        ~VRName();

        string setName(string name);
        string getName();
        string getBaseName();
        int getNameSuffix();
        void setSeparator(char s);
        void setNameSpace(string s);

        void saveName(xmlpp::Element* e);
        void loadName(xmlpp::Element* e);

        static int getNameNumber();
        static int getBaseNameNumber();
        static void printNameDict();
};

#endif // VRNAME_H_INCLUDED
