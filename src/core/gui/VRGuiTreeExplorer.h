#ifndef VRGUITREEEXPLORER_H_INCLUDED
#define VRGUITREEEXPLORER_H_INCLUDED

#include "VRGuiFwd.h"
#include <OpenSG/OSGConfig.h>
#include <gtkmm.h>
#include <gtkmm/treemodel.h>

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRGuiTreeExplorer {
    private:
        Gtk::WindowPtr win;
        string cols;

        Glib::RefPtr<Gtk::TreeStore> m_refTreeModel;
        map<int, Gtk::TreeModel::iterator> rows;

    public:
        VRGuiTreeExplorer(string cols);
        ~VRGuiTreeExplorer();

        static VRGuiTreeExplorerPtr create(string cols);

        int add(int parent, ...);

        void move(int, int);
        void remove(int);
};

OSG_END_NAMESPACE;

#endif // VRGUITREEEXPLORER_H_INCLUDED
