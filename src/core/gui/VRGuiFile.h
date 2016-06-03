#ifndef VRGUIFILE_H_INCLUDED
#define VRGUIFILE_H_INCLUDED

#include <string>
#include <vector>
#include <sigc++/functors/slot.h>

namespace Gtk { class FileChooserDialog; class Table; }
using namespace std;

class VRGuiFile {
    private:
        static Gtk::FileChooserDialog* dialog;
        static Gtk::Table* addon;
        typedef sigc::slot<void> sig;
        static sig sigApply;
        static sig sigClose;
        static sig sigSelect;
        static bool cache_override;
        static float scale;
        static void init();

        static void on_toggle_cache_override();
        static void on_edit_import_scale();

    public:
        static void open(string button, int action, string title);
        static void close();
        static void apply();
        static void select();
        static string getPath();
        static string getRelativePath_toOrigin();
        static string getRelativePath_toWorkdir();

        static void gotoPath(string path);
        static void setFile(string file);
        static void addFilter(string name, int N, ...);
        static void clearFilter();

        static void setCallbacks(sig sa = sig(), sig sc = sig(), sig ss = sig());
        static void setWidget(Gtk::Table* table);

        static bool exists(string path);
        static bool isDir(string path);
        static bool isFile(string path);
        static vector<string> listDir(string dir);

        static float getScale() { return scale; }
};

#endif // VRGUIFILE_H_INCLUDED
