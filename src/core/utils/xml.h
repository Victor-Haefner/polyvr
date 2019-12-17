#ifndef XML_H_INCLUDED
#define XML_H_INCLUDED

#include <string>
#include <map>
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
        bool hasText();
        void setText(string text);

        string getAttribute(string name);
        map<string,string> getAttributes();
        bool hasAttribute(string name);
        void setAttribute(string name, string value);

        vector<XMLElementPtr> getChildren(string name = "");
        XMLElementPtr getChild(string name);
        XMLElementPtr getChild(int i);
        XMLElementPtr addChild(string name);

        void importNode(XMLElementPtr e, bool recursive, XML& xml);

        void print();
};

class XMLStreamHandler {
    private:
    public:
        XMLStreamHandler();
        virtual ~XMLStreamHandler();

        virtual void startDocument() = 0;
        virtual void endDocument() = 0;
        virtual void startElement(const string& name, const map<string, string>& attributes) = 0;
        virtual void endElement(const string& name) = 0;
};

class XML {
    private:
        _xmlDoc* doc = 0;
        XMLElementPtr root = 0;

    public:
        XML();
        ~XML();

        static XMLPtr create();

        void read(string path, bool validate = true);
        void parse(string data, bool validate = true);
        void stream(string path, XMLStreamHandler* handler);
        void write(string path);
        string toString();

        XMLElementPtr getRoot();
        XMLElementPtr newRoot(string name, string ns_uri, string ns_prefix);
        void printTree(XMLElementPtr e, string D = "");

        friend class XMLElement;
};

#endif // XML_H_INCLUDED
