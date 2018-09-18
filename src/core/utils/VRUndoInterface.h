#ifndef VRUNDOINTERFACE_H_INCLUDED
#define VRUNDOINTERFACE_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include "core/tools/VRToolsFwd.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRUndoInterface {
    private:
        VRUndoManagerWeakPtr undo;
        bool initiated = false;

    public:
        VRUndoInterface();
        ~VRUndoInterface();

        void setUndoManager(VRUndoManagerPtr mgr);

        template<class F, class O, class V>
        void recUndo(F f, std::shared_ptr<O> o, V v1, V v2);

        bool isUndoing();
        bool undoInitiated();
};

OSG_END_NAMESPACE;

#endif // VRUNDOINTERFACE_H_INCLUDED
