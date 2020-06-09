#ifndef VRGUITREEVIEW_H_INCLUDED
#define VRGUITREEVIEW_H_INCLUDED

#include <string>
#include "VRGuiWidget.h"

using namespace std;

struct _GtkTreeView;
struct _GtkTreeModel;
struct _GtkWidget;
struct _GtkTreeIter;
struct _GtkTreePath;
struct _GtkTreeViewColumn;

class VRGuiTreeView : public VRGuiWidget {
    private:
        _GtkTreeView* tree_view = 0;
        _GtkTreeModel* tree_model = 0;
        _GtkTreeIter* selection = 0;
        bool hasListStore = false;
        bool validSelection;

        void init(_GtkWidget* widget);

    public:
        VRGuiTreeView(string name, bool hasListStore = false);
        VRGuiTreeView(_GtkWidget* widget, bool hasListStore = false);
        ~VRGuiTreeView();

        bool hasSelection();
        void updateSelection();
        bool getSelection(_GtkTreeIter* itr);
        void selectRow(_GtkTreePath* p, _GtkTreeViewColumn* c);

        void removeRow(_GtkTreeIter* itr);
        void removeSelected();

        void  setValue(_GtkTreeIter* itr, int column, void* data);
        void  setStringValue(_GtkTreeIter* itr, int column, string data);
        void* getValue(_GtkTreeIter* itr, int column);
        string getStringValue(_GtkTreeIter* itr, int column);

        void  setSelectedValue(int column, void* data);
        void  setSelectedStringValue(int column, string data);
        void* getSelectedValue(int column);
        string getSelectedStringValue(int column);
        int getSelectedIntValue(int column);

        bool getSelectedParent(_GtkTreeIter& parent);

        void expandAll();

        string getSelectedColumnName();
};

#endif // VRGUITREEVIEW_H_INCLUDED
