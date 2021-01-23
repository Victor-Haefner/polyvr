#include "VRGuiRecWidget.h"
#include "VRGuiUtils.h"
#include "VRGuiBuilder.h"

#include "core/tools/VRRecorder.h"
#include "core/scene/VRSceneManager.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "VRGuiUtils.h"

#include <gtk/gtk.h>

using namespace OSG;

VRGuiRecWidget::VRGuiRecWidget() {
#ifndef WITHOUT_AV
    rec = VRRecorderPtr( new VRRecorder() );
    rec->setView(0);

    diag = (GtkDialog*)VRGuiBuilder::get()->get_widget("recorder");
    lbl = (GtkLabel*)VRGuiBuilder::get()->get_widget("label149");
    disableDestroyDiag("recorder");

    gtk_window_set_resizable((GtkWindow*)diag, false);
    gtk_window_set_type_hint((GtkWindow*)diag, GDK_WINDOW_TYPE_HINT_MENU);
    gtk_window_set_title((GtkWindow*)diag, "Recorder");

    GtkButtonBox* box = (GtkButtonBox*)gtk_dialog_get_action_area(diag);

    auto addButton = [&](const char* icon, int signal) {
        auto but = (GtkButton*)gtk_button_new();
		gtk_widget_set_size_request((GtkWidget*)but, 20, -1);
        GtkWidget* img = gtk_image_new_from_icon_name(icon, GTK_ICON_SIZE_BUTTON);
        gtk_container_add((GtkContainer*)but, img);
        gtk_container_add((GtkContainer*)box, (GtkWidget*)but);
        function<void()> sig = bind(&VRGuiRecWidget::buttonHandler, this, signal);
        connect_signal((GtkWidget*)but, sig, "clicked");
    };

    addButton("media-record", 1);
    addButton("media-playback-pause", 2);
    addButton("gtk-floppy", 3);
    addButton("view-refresh", 4);

    fillStringListstore("resList", VRRecorder::getResList());
    fillStringListstore("codecList", VRRecorder::getCodecList());
    setCombobox("resolutions", getListStorePos("resList", "custom"));
    setCombobox("codecs", getListStorePos("codecList", rec->getCodec()));
    setTextEntry("entry27", toString(rec->getBitrate()));

    setComboboxCallback("resolutions", bind(&VRGuiRecWidget::on_res_changed, this));
    setComboboxCallback("codecs", bind(&VRGuiRecWidget::on_codec_changed, this));
    setEntryCallback("entry27", bind(&VRGuiRecWidget::on_bitrate_changed, this));
    setCheckButtonCallback("doVSyncCB", bind(&VRGuiRecWidget::on_toggle_vsync, this));

    gtk_widget_show_all((GtkWidget*)box);
    updateCb = VRUpdateCb::create("recorder widget", bind(&VRGuiRecWidget::update, this) );
    VRSceneManager::get()->addUpdateFkt( updateCb );

    setVisible(false);
#endif
}

VRGuiRecWidget::~VRGuiRecWidget() {}

void VRGuiRecWidget::on_codec_changed() {
#ifndef WITHOUT_AV
	rec->setCodec( getComboboxText("codecs") );
#endif
}

void VRGuiRecWidget::on_toggle_vsync() {
#ifndef WITHOUT_AV
	rec->enableVSync( getCheckButtonState("doVSyncCB") );
#endif
}

void VRGuiRecWidget::on_res_changed() {
#ifndef WITHOUT_AV
	rec->setViewResolution( getComboboxText("resolutions") );
#endif
}

void VRGuiRecWidget::on_bitrate_changed() {
#ifndef WITHOUT_AV
	rec->setBitrate( toInt( getTextEntry("entry27") ) );
#endif
}

void VRGuiRecWidget::setVisible(bool b) {
    if (b) gtk_widget_show((GtkWidget*)diag);
    else gtk_widget_hide((GtkWidget*)diag);
}

void VRGuiRecWidget::update() {
#ifndef WITHOUT_AV
    if (!rec->isRunning()) return;
    string T = "Recording: " + toString(rec->getRecordingLength(), 2);
    gtk_label_set_text(lbl, T.c_str());
#endif
}

void VRGuiRecWidget::buttonHandler(int i) {
#ifndef WITHOUT_AV
    if (i == 1) rec->setRecording(true);
    if (i == 2) rec->setRecording(false);
    if (i == 3) {
        string path = rec->getPath();
        string T = "Exporting to: " + path;
        gtk_label_set_text(lbl, T.c_str());
        rec->compile(path);
    }
    if (i == 4) {
        gtk_label_set_text(lbl, "Idle");
        rec->clear();
    }
#endif
}

