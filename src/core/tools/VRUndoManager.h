#ifndef VRUNDOMANAGER_H_INCLUDED
#define VRUNDOMANAGER_H_INCLUDED

#include "VRToolsFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include "core/utils/VRManager.h"

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRUndoAtom : public VRName {
    private:
        VRUpdateCbPtr f_undo;
        VRUpdateCbPtr f_redo;
        VREvalCbPtr f_valid;

    public:
        VRUndoAtom(string name);

        static VRUndoAtomPtr create(string name = "");

        void set(VRUpdateCbPtr f_undo, VRUpdateCbPtr f_redo, VREvalCbPtr f_valid);

        bool valid();
        bool undo();
        bool redo();
};

class VRUndoManager : public VRManager<VRUndoAtom>, public std::enable_shared_from_this<VRUndoManager> {
    private:
        std::map<int, VRUndoAtomPtr>::reverse_iterator current = data.rend();
        bool ward = false;
        int key = 0;

    public:
        VRUndoManager();

        static VRUndoManagerPtr create();
        VRUndoManagerPtr ptr();

        void addObject(VRObjectPtr o);
        void recUndo(VRUpdateCbPtr f_undo, VRUpdateCbPtr f_redo, VREvalCbPtr f_valid);

        void undo();
        void redo();
};

OSG_END_NAMESPACE;

#endif // VRUNDOMANAGER_H_INCLUDED
