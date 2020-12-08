#ifndef VRGUIBUILDER_H_INCLUDED
#define VRGUIBUILDER_H_INCLUDED

#include <map>
#include <string>

using namespace std;

struct _GtkBuilder;
struct _GtkWidget;

#if GTK_MAJOR_VERSION == 2
struct _GtkObject;
#else
struct _GObject;
#define _GtkObject _GObject
#endif

class VRGuiBuilder {
    private:
        _GtkBuilder* builder = 0;
        map<string, _GtkWidget*> widgets;
        map<string, _GtkObject*> objects;

        void buildMinimalUI();
        void buildBaseUI();

    public:
        VRGuiBuilder();
        ~VRGuiBuilder();

        static VRGuiBuilder* get(bool standalone = false);

        void read(string path);

        bool has_object(string name);
        bool has_widget(string name);

        _GtkWidget* get_widget(string name);
        _GtkObject* get_object(string name);

        void reg_widget(_GtkWidget* w, string name);
        void reg_object(_GtkObject* o, string name);
};

#endif // VRGUIBUILDER_H_INCLUDED
