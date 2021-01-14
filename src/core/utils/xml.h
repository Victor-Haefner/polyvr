#ifndef XML_H_INCLUDED
#define XML_H_INCLUDED

#include <string>
#include <map>
#include <OpenSG/OSGConfig.h>
#include "VRUtilsFwd.h"

using namespace std;

struct _xmlDoc;
struct _xmlNode;

OSG_BEGIN_NAMESPACE;

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

// attrib: key=val
//    key: uri:name
struct XMLAttribute {
    string key;
    string val;

    string uri();
    string name();
    string qname();
};

class XMLInputStream {
    private:
    public:
        XMLInputStream();
        virtual ~XMLInputStream();

        virtual size_t readBytes(char* const toFill, const size_t maxToRead) = 0;
};

class XMLStreamHandler {
    private:
    public:
        XMLStreamHandler();
        virtual ~XMLStreamHandler();

        virtual void startDocument();
        virtual void endDocument();
        virtual void startElement(const string& uri, const string& name, const string& qname, const map<string, XMLAttribute>& attributes);
        virtual void endElement(const string& uri, const string& name, const string& qname);
        virtual void characters(const string& chars);
        virtual void processingInstruction(const string& target, const string& data);
        virtual void warning(const string& chars);
        virtual void error(const string& chars);
        virtual void fatalError(const string& chars);
        virtual void onException(exception& e);
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
        void stream(XMLInputStream* input, XMLStreamHandler* handler);

        void write(string path);
        string toString();

        XMLElementPtr getRoot();
        XMLElementPtr newRoot(string name, string ns_uri, string ns_prefix);
        void printTree(XMLElementPtr e, string D = "");

        friend class XMLElement;
};

OSG_END_NAMESPACE;

#endif // XML_H_INCLUDED
