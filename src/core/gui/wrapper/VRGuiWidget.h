#ifndef VRGUIWIDGET_H_INCLUDED
#define VRGUIWIDGET_H_INCLUDED

#include <string>

using namespace std;

struct _GtkWidget;

class VRGuiWidget {
    private:
        _GtkWidget* widget = 0;

    public:
        VRGuiWidget(string name);
        VRGuiWidget(_GtkWidget* widget);
        ~VRGuiWidget();

        void setVisibility(bool b, bool all);
        void setSensitivity(bool b);
        void grabFocus();
};

#endif // VRGUIWIDGET_H_INCLUDED
