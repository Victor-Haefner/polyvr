#ifndef VRGUINAV_H_INCLUDED
#define VRGUINAV_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <gtkmm/builder.h>
#include <gtkmm/treemodel.h>

OSG_BEGIN_NAMESPACE;
using namespace std;

class VRGuiNav {
    private:
        void on_typebinding_changed(const Glib::ustring &sPath, const Gtk::TreeModel::iterator &iter);

    public:
        VRGuiNav();

        void update();
};

OSG_END_NAMESPACE;

#endif // VRGUINAV_H_INCLUDED
