#ifndef VRGUIUTILS_H_INCLUDED
#define VRGUIUTILS_H_INCLUDED

#include <OpenSG/OSGConfig.h>
#include <OpenSG/OSGColor.h>
#include <gtkmm/builder.h>
#include <gtkmm/treemodelcolumn.h>
#include <gtkmm/treestore.h>

//OSG_BEGIN_NAMESPACE;
using namespace std;

Glib::RefPtr<Gtk::Builder> VRGuiBuilder();

// TEXT
void setLabel(string l, string txt);
void setTextEntry(string entry, string text);
string getTextEntry(string entry);
void setEntryCallback(string e, void (* fkt)(GtkEntry*, gpointer));
void setEntryCallback(string e, sigc::slot<void> sig);

// BUTTONS
void setButtonCallback(string b, void (* fkt)(GtkButton*, gpointer), gpointer data = NULL);
void setButtonCallback(string b, sigc::slot<void> sig );
void setToolButtonCallback(string b, void (* fkt)(GtkButton*, gpointer));
void setToolButtonCallback(string b, sigc::slot<void> sig );
void setToolButtonSensivity(string toolbutton, bool b);
void setCheckButton(string cb, bool b);
bool getCheckButtonState(string b);
void setCheckButtonCallback(string cb, void (* fkt)(GtkToggleButton*, gpointer) );
void setCheckButtonCallback(string cb, sigc::slot<void> sig );
bool getRadioButtonState(string b);
bool getToggleButtonState(string b);
void setRadioButtonCallback(string cb, sigc::slot<void> sig );
void setRadioButton(string cb, bool b );
void setButtonText(string cb, string txt );

// COMBOBOX
void setComboboxCallback(string b, void (* fkt)(GtkComboBox*, gpointer));
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

// LISTVIEWS
void fillStringListstore(string ls, vector<string> list);
void setCellRendererCallback(string renderer, void (* fkt)(GtkCellRendererText*, gchar*, gchar*, gpointer));
void setCellRendererCallback(string renderer, sigc::slot<void,const Glib::ustring&,const Glib::ustring& > sig, bool after = true);
void setCellRendererCombo(string treeviewcolumn, string combolist, Gtk::TreeModelColumnBase& col, void (* fkt)(GtkCellRendererCombo*, gchar*, GtkTreeIter*, gpointer));
void setCellRendererCombo(string treeviewcolumn, string combolist, Gtk::TreeModelColumnBase& col, sigc::slot<void,const Glib::ustring&,const Gtk::TreeModel::iterator& > sig);
void setTreeviewSelectCallback(string treeview, sigc::slot<void> sig);

// STUFF
void setTableSensivity(string table, bool b);
void setNotebookSensivity(string nb, bool b);
void setVPanedSensivity(string vp, bool b);
void setNoteBookCallback(string nb, void (* fkt)(GtkNotebook*, GtkNotebookPage*, guint, gpointer) , gpointer ptr = NULL);
void setNotebookPage(string nb, int p);

bool keySignalProxy(GdkEventKey* e, string k, sigc::slot<void> sig );
void setExpanderSensivity(string exp, bool b);
bool askUser(string msg1, string msg2);
OSG::Color4f chooseColor(string drawable, OSG::Color4f current);
void setColorChooser(string drawable, sigc::slot<bool, GdkEventButton*> sig);
void setColorChooserColor(string drawable, OSG::Color3f col);

void saveScene(string path = "");

void showDialog(string d);
void hideDialog(string d);

//OSG_END_NAMESPACE;

#endif // VRGUIUTILS_H_INCLUDED
