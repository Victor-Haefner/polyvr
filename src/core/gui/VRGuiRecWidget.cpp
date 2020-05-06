#include "VRGuiRecWidget.h"
#include "VRGuiUtils.h"

#include "core/tools/VRRecorder.h"
#include "core/scene/VRSceneManager.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "VRGuiUtils.h"

#include <gtkmm/dialog.h>
#include <gtkmm/image.h>
#include <gtkmm/stock.h>
#include <gtkmm/label.h>
#include <gtkmm/window.h>
#include <gtkmm/builder.h>
#include <boost/bind.hpp>

using namespace OSG;

VRGuiRecWidget::VRGuiRecWidget() {
    rec = VRRecorderPtr( new VRRecorder() );
    rec->setView(0);

    getGUIBuilder()->get_widget("recorder", diag);
    getGUIBuilder()->get_widget("label149", lbl);
    diag->set_deletable(false); // not working on most platforms
    diag->signal_delete_event().connect( sigc::mem_fun(*this, &VRGuiRecWidget::deleteHandler) );
    diag->set_resizable(false);
    diag->set_type_hint(Gdk::WINDOW_TYPE_HINT_MENU);
    diag->set_title("Recorder");

    Gtk::ButtonBox* box = diag->get_action_area();
    box->set_child_min_width(20);

    auto addButton = [&](Gtk::BuiltinStockID icon, int signal) {
        auto b = diag->add_button("",signal);
        Gtk::Image* img = Gtk::manage( new Gtk::Image(icon, Gtk::ICON_SIZE_BUTTON) );
        b->set_image(*img);
    };

    addButton(Gtk::Stock::MEDIA_RECORD, 1);
    addButton(Gtk::Stock::MEDIA_PAUSE, 2);
    addButton(Gtk::Stock::FLOPPY, 3);
    addButton(Gtk::Stock::REFRESH, 4);

    fillStringListstore("codecList", VRRecorder::getCodecList());
    setCombobox("codecs", getListStorePos("codecList", rec->getCodec()));
    setTextEntry("entry27", toString(rec->getBitrate()));

    setComboboxCallback("codecs", sigc::mem_fun(*this, &VRGuiRecWidget::on_codec_changed));
    setEntryCallback("entry27", sigc::mem_fun(*this, &VRGuiRecWidget::on_bitrate_changed));

    diag->show_all_children();
    diag->signal_response().connect( sigc::mem_fun(*this, &VRGuiRecWidget::buttonHandler) );
    updateCb = VRUpdateCb::create("recorder widget", boost::bind(&VRGuiRecWidget::update, this) );
    VRSceneManager::get()->addUpdateFkt( updateCb );
}

VRGuiRecWidget::~VRGuiRecWidget() {}

void VRGuiRecWidget::on_codec_changed() { rec->setCodec( getComboboxText("codecs") ); }
void VRGuiRecWidget::on_bitrate_changed() { rec->setBitrate( toInt( getTextEntry("entry27") ) ); }

bool VRGuiRecWidget::deleteHandler(GdkEventAny* e) { setVisible(false); return true; }

void VRGuiRecWidget::setVisible(bool b) {
    if (b) diag->show();
    else diag->hide();
}

void VRGuiRecWidget::update() {
    if (!rec->isRunning()) return;
    string T = toString(rec->getRecordingLength(), 2);
    lbl->set_text("Recording: " + T );
}

void VRGuiRecWidget::buttonHandler(int i) {
    if (i == 1) rec->setRecording(true);
    if (i == 2) rec->setRecording(false);
    if (i == 3) {
        string path = rec->getPath();
        lbl->set_text("Exporting to: " + path);
        rec->compile(path);
    }
    if (i == 4) {
        lbl->set_text("Idle");
        rec->clear();
    }
}

