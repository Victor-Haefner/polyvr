#ifndef VRGUITREEEXPLORER_H_INCLUDED
#define VRGUITREEEXPLORER_H_INCLUDED

#include "VRGuiFwd.h"
#include "core/utils/VRFunctionFwd.h"
#include <OpenSG/OSGConfig.h>

struct _GtkWindow;
struct _GtkTextView;
struct _GtkTreeView;
struct _GtkEntry;
struct _GtkTreeIter;
struct _GtkTreeStore;
struct _GtkTextBuffer;

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRGuiTreeExplorer {
    private:
        _GtkWindow* win = 0;
        _GtkTextView* m_TextView = 0;
        _GtkTreeView* m_TreeView = 0;
        _GtkEntry* searchEntry = 0;
        string cols;

        shared_ptr< VRFunction<VRGuiTreeExplorer*> > cb;
        string info;

        _GtkTreeStore* m_refTreeModel;
        _GtkTextBuffer* infoBuffer;
        map<int, _GtkTreeIter> rows;
        _GtkTreeIter* selected;

        void on_search_edited();
        void on_row_select();

    public:
        VRGuiTreeExplorer(string cols, string title);
        ~VRGuiTreeExplorer();

        static VRGuiTreeExplorerPtr create(string cols, string title);

        int add(int parent, int N, ...);

        void move(int, int);
        void remove(int);

        void setInfo(string s);
        void setSelectCallback( shared_ptr< VRFunction<VRGuiTreeExplorer*> > cb );

        _GtkTreeIter* getSelected();
        template<class T> T get(_GtkTreeIter* itr, int i);
};

OSG_END_NAMESPACE;

#endif // VRGUITREEEXPLORER_H_INCLUDED
