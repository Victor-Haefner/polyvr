#ifndef VRGUIUTILS_H_INCLUDED
#define VRGUIUTILS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGColor.h>
#include <gtkmm/treemodelcolumn.h>
#include <gtkmm-2.4/gtkmm/treemodel.h>
#include <sigc++/functors/slot.h>
#include "core/objects/VRObjectFwd.h"

//OSG_BEGIN_NAMESPACE;
using namespace std;

namespace Gtk { class Builder; }
Glib::RefPtr<Gtk::Builder> VRGuiBuilder(bool standalone = false);

// TEXT
void setLabel(string l, string txt);
void setTextEntry(string entry, string text);
string getTextEntry(string entry);
void setEntryCallback(string e, sigc::slot<void> sig, bool onEveryChange = false);
void setEntrySensitivity(string e, bool b);

// BUTTONS
void setButtonCallback(string b, sigc::slot<void> sig );
void setToggleButtonCallback(string b, sigc::slot<void> sig );
void setToolButtonCallback(string b, sigc::slot<void> sig );
void setToolButtonSensitivity(string toolbutton, bool b);
void setCheckButton(string cb, bool b);
bool getCheckButtonState(string b);
void setCheckButtonCallback(string cb, sigc::slot<void> sig );
bool getRadioButtonState(string b);
bool getRadioToolButtonState(string b);
bool getToggleButtonState(string b);
void setRadioToolButtonCallback(string cb, sigc::slot<void> sig );
void setRadioButtonCallback(string cb, sigc::slot<void> sig );
void setRadioButton(string cb, bool b );
void setRadioToolButton(string cb, bool b );
void setButtonText(string cb, string txt );
void setButtonSensitivity(string b, bool s );

// COMBOBOX
void setComboboxCallback(string b, sigc::slot<void> sig);
void setComboboxLastActive(string cb);
void setCombobox(string cb, int i);
int getListStorePos(string ls, string s);
string getComboboxText(string cbn);
int getComboboxI(string cbn);
Gtk::TreeModel::Row getComboboxRow(string cbn);
Gtk::TreeModel::iterator getComboboxIter(string cbn);
void eraseComboboxActive(string cb);
void setComboboxSensitivity(string cb, bool b);

// SLIDER
void setSliderCallback(string s, sigc::slot< bool,int,double > sig);
float getSliderState(string s);

// LISTVIEWS
void fillStringListstore(string ls, vector<string> list);
void setCellRendererCallback(string renderer, void (* fkt)(GtkCellRendererText*, gchar*, gchar*, gpointer));
void setCellRendererCallback(string renderer, sigc::slot<void,const Glib::ustring&,const Glib::ustring& > sig, bool after = true);
void setCellRendererCombo(string treeviewcolumn, string combolist, Gtk::TreeModelColumnBase& col, void (* fkt)(GtkCellRendererCombo*, gchar*, GtkTreeIter*, gpointer));
void setCellRendererCombo(string treeviewcolumn, string combolist, Gtk::TreeModelColumnBase& col, sigc::slot<void,const Glib::ustring&,const Gtk::TreeModel::iterator& > sig);
void setTreeviewSelectCallback(string treeview, sigc::slot<void> sig);
Gtk::TreeModel::iterator getTreeviewSelected(string treeview);
void selectTreestoreRow(string treeview, Gtk::TreeModel::iterator itr);
void focusTreeView(string treeview);

// STUFF
void setTableSensitivity(string table, bool b);
void setNotebookSensitivity(string nb, bool b);
void setVPanedSensitivity(string vp, bool b);
void setNoteBookCallback(string nb, void (* fkt)(GtkNotebook*, GtkNotebookPage*, guint, gpointer) , gpointer ptr = NULL);
void setNotebookPage(string nb, int p);
void setTooltip(string widget, string tp);

bool keySignalProxy(GdkEventKey* e, string k, sigc::slot<void> sig );
void setExpanderSensitivity(string exp, bool b);
bool askUser(string msg1, string msg2);
string askUserInput(string msg);
string askUserPass(string msg);
OSG::Color4f chooseColor(string drawable, OSG::Color4f current);
void setColorChooser(string drawable, sigc::slot<bool, GdkEventButton*> sig);
void setColorChooserColor(string drawable, OSG::Color3f col);

OSG::VRTexturePtr takeSnapshot();
void saveScene(string path = "", bool saveas = false);

void showDialog(string d);
void hideDialog(string d);

//OSG_END_NAMESPACE;

#endif // VRGUIUTILS_H_INCLUDED
