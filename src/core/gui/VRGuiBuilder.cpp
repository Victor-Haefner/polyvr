#include <gtk/gtk.h>

#include "VRGuiBuilder.h"
#include "VRGuiFile.h"

#include <iostream>

VRGuiBuilder::VRGuiBuilder() {}
VRGuiBuilder::~VRGuiBuilder() {}

VRGuiBuilder* VRGuiBuilder::get(bool standalone) {
	static VRGuiBuilder* b = 0;
	if (b) return b;
	b = new VRGuiBuilder();

#if GTK_MAJOR_VERSION == 2
	string path = "ressources/gui/VRDirector.glade";
#else
	string path = "ressources/gui/VRDirector3.glade";
#endif
	if (standalone) path = "ressources/gui/VRDirector_min.glade";
	if (!VRGuiFile::exists(path)) cerr << "FATAL ERROR: " << path << " not found\n";
	else {
        cout << " found glade file: " << path << endl;
        b->read(path);
		cout << "  finished importing glade file: " << path << endl;
	}
    return b;
}

void VRGuiBuilder::read(string path) {
    //XML xml;
    //xml.read(path);
    cout << "VRGuiBuilder::read " << path << endl;
    builder = gtk_builder_new();
    cout << " VRGuiBuilder::read file" << endl;
    GError* error = 0;
    gtk_builder_add_from_file(builder, path.c_str(), &error);
    cout << " VRGuiBuilder::read done" << endl;
    if (error) {
        cout << "  -- VRGuiBuilder read error: " << error->message << endl;
        g_clear_error(&error);
        g_error_free(error);
    }
}

GtkWidget* VRGuiBuilder::get_widget(string name) {
    auto w = (GtkWidget*)gtk_builder_get_object(builder, name.c_str());
    if (!w) cout << " Error in VRGuiBuilder::get_widget, no widget called " << name << endl;
    return w;

    //if (widgets.count(name)) return widgets[name];
    //return 0;
}

_GtkObject* VRGuiBuilder::get_object(string name) {
    auto o = (_GtkObject*)gtk_builder_get_object(builder, name.c_str());
    if (!o) cout << " Error in VRGuiBuilder::get_object, no object called " << name << endl;
    return o;
}
