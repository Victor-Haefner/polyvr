#ifndef XML_H_INCLUDED
#define XML_H_INCLUDED

#include <string>
#include "VRUtilsFwd.h"

using namespace std;

struct _xmlNode;

class XMLElement {
    private:
        _xmlNode* node;

    public:
        XMLElement(_xmlNode* node);
        ~XMLElement();

        static XMLElementPtr create(_xmlNode* node);

        string getName();
        string getNameSpace();
        vector<XMLElementPtr> getChildren();
};

class XML {
    private:
        XMLElementPtr root = 0;

    public:
        XML();
        ~XML();

        static XMLPtr create();

        void read(string path);
        void write(string path);

        XMLElementPtr getRoot();
};

#endif // XML_H_INCLUDED
