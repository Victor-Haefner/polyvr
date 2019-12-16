#ifndef XML_H_INCLUDED
#define XML_H_INCLUDED

#include <string>
#include "VRUtilsFwd.h"

using namespace std;

struct _xmlDoc;
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

        string getText();
        void setText(string text);

        string getAttribute(string name);
        bool hasAttribute(string name);
        void setAttribute(string name, string value);

        vector<XMLElementPtr> getChildren(string name = "");
        XMLElementPtr getChild(string name);
        XMLElementPtr addChild(string name);
};

class XML {
    private:
        _xmlDoc* doc = 0;
        XMLElementPtr root = 0;

    public:
        XML();
        ~XML();

        static XMLPtr create();

        void read(string path);
        void write(string path);

        XMLElementPtr getRoot();
        XMLElementPtr newRoot(string name, string ns_uri, string ns_prefix);
        void printTree(XMLElementPtr e, string D = "");
};

#endif // XML_H_INCLUDED
