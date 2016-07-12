#include "VRGuiRecWidget.h"
#include "VRGuiUtils.h"

#include "core/tools/VRRecorder.h"

#include <gtkmm/filechooser.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/button.h>
#include <gtkmm/dialog.h>
#include <gtkmm/label.h>
#include <gtkmm/builder.h>
#include <boost/filesystem.hpp>

using namespace OSG;

VRGuiRecWidget::VRGuiRecWidget() {
    rec = VRRecorderPtr( new VRRecorder() );
    rec->setView(0);

    diag = new Gtk::Dialog();
    diag->set_title("Recorder");
    diag->add_button(Gtk::StockID("gtk-media-record"),1);
    diag->add_button(Gtk::StockID("gtk-media-pause"),2);
    diag->add_button(Gtk::StockID("gtk-floppy"),3);
    diag->add_button(Gtk::StockID("gtk-refresh"),3);

    Gtk::Label lbl;
    lbl.set_text("HELOOOO");
    diag->get_vbox()->pack_start(lbl);
    diag->set_decorated(false);
    diag->set_resizable(false);
}

VRGuiRecWidget::~VRGuiRecWidget() {
    delete diag;
}

void VRGuiRecWidget::setVisible(bool b) {
    if (b) diag->show_all();
    else diag->hide();
}
