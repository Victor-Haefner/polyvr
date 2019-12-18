#ifndef VRPROTOCOL_H_INCLUDED
#define VRPROTOCOL_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#undef _XOPEN_SOURCE
#undef _POSIX_C_SOURCE
#include <Python.h>
#include <string>
#include <map>
#include "core/utils/VRFunction.h"
#include "VRSocket.h"
#include "core/utils/VRName.h"

namespace xmlpp{ class Element; }

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRProtocol : public VRName {
    public:
        struct VRProtocolBinding {
            string name;
            string type;
            int port;
            string ip;
            string callback;
            string signal;

            VRProtocolBinding() : port(0) {}
        };

    private:
        map<string, VRProtocolBinding> bindings;
        map<string, VRProtocolBinding>::iterator itr;

        map<string, VRSocket*> sockets;

        void updateSocket();

    public:
        VRProtocol(string name);

        void addBinding(VRProtocolBinding a);
        void remBinding(string name);
        VRProtocolBinding& getBinding(string name);
        map<string, VRProtocolBinding> getBindings();

        void changeBindingName(string name, string _new);
        void changeType(string name, string type);
        void changePort(string name, int port);
        void changeIP(string name, string ip);
        void changeCallback(string name, string callback) ;
        void changeSignal(string name, string signal);

        void save(XMLElementPtr e);
        void load(XMLElementPtr e);
};

OSG_END_NAMESPACE;

#endif // VRPROTOCOL_H_INCLUDED
