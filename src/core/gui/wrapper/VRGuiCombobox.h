#ifndef VRGUICOMBOBOX_H_INCLUDED
#define VRGUICOMBOBOX_H_INCLUDED

#include <string>
#include "VRGuiWidget.h"

using namespace std;

struct _GtkComboBox;
struct _GtkTreeModel;
struct _GtkWidget;
struct _GtkTreeIter;

class VRGuiCombobox : public VRGuiWidget {
    private:
        _GtkComboBox* combobox = 0;
        _GtkTreeModel* tree_model = 0;
        _GtkTreeIter* selection = 0;

        void init(_GtkWidget* widget);

    public:
        VRGuiCombobox(string name);
        VRGuiCombobox(_GtkWidget* widget);
        ~VRGuiCombobox();

        bool hasSelection();
        void updateSelection();
        void selectRow(int i);

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
};

#endif // VRGUICOMBOBOX_H_INCLUDED
