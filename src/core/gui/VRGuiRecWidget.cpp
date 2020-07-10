#include "VRGuiRecWidget.h"
#include "VRGuiUtils.h"

#include "core/tools/VRRecorder.h"
#include "core/scene/VRSceneManager.h"
#include "core/utils/toString.h"
#include "core/utils/VRFunction.h"
#include "VRGuiUtils.h"

#include <gtk/gtkdialog.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkbbox.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkimage.h>
#include <gtk/gtkstock.h>

using namespace OSG;

VRGuiRecWidget::VRGuiRecWidget() {
    rec = VRRecorderPtr( new VRRecorder() );
    rec->setView(0);

    diag = (GtkDialog*)getGUIBuilder()->get_widget("recorder");
    lbl = (GtkLabel*)getGUIBuilder()->get_widget("label149");
    disableDestroyDiag("recorder");

    gtk_window_set_resizable((GtkWindow*)diag, false);
    gtk_window_set_type_hint((GtkWindow*)diag, GDK_WINDOW_TYPE_HINT_MENU);
    gtk_window_set_title((GtkWindow*)diag, "Recorder");

    GtkButtonBox* box = (GtkButtonBox*)gtk_dialog_get_action_area(diag);
    gtk_button_box_set_child_size(box, 20, -1);

    auto addButton = [&](const char* icon, int signal) {
        //GtkButton* b = (GtkButton*)gtk_dialog_add_button(diag, NULL, signal);
        //GtkButton* b = (GtkButton*)gtk_dialog_add_button(diag, GTK_STOCK_MEDIA_PLAY, signal);
        //auto img = gtk_image_new_from_stock(GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_BUTTON);
        //gtk_image_set_from_stock(imgPlay, stock_id, GTK_ICON_SIZE_BUTTON);
        //gtk_button_set_image(but, img);

        auto but = (GtkButton*)gtk_button_new();
        GtkWidget* img = gtk_image_new_from_stock(icon, GTK_ICON_SIZE_BUTTON);
        gtk_container_add((GtkContainer*)but, img);
        gtk_container_add((GtkContainer*)box, (GtkWidget*)but);
        function<void()> sig = bind(&VRGuiRecWidget::buttonHandler, this, signal);
        connect_signal((GtkWidget*)but, sig, "clicked");
    };

    addButton(GTK_STOCK_MEDIA_RECORD, 1);
    addButton(GTK_STOCK_MEDIA_PAUSE, 2);
    addButton(GTK_STOCK_FLOPPY, 3);
    addButton(GTK_STOCK_REFRESH, 4);

    fillStringListstore("codecList", VRRecorder::getCodecList());
    setCombobox("codecs", getListStorePos("codecList", rec->getCodec()));
    setTextEntry("entry27", toString(rec->getBitrate()));

    setComboboxCallback("codecs", bind(&VRGuiRecWidget::on_codec_changed, this));
    setEntryCallback("entry27", bind(&VRGuiRecWidget::on_bitrate_changed, this));

    gtk_widget_show_all((GtkWidget*)box);
    updateCb = VRUpdateCb::create("recorder widget", bind(&VRGuiRecWidget::update, this) );
    VRSceneManager::get()->addUpdateFkt( updateCb );

    setVisible(false);
}

VRGuiRecWidget::~VRGuiRecWidget() {}

void VRGuiRecWidget::on_codec_changed() { rec->setCodec( getComboboxText("codecs") ); }
void VRGuiRecWidget::on_bitrate_changed() { rec->setBitrate( toInt( getTextEntry("entry27") ) ); }

void VRGuiRecWidget::setVisible(bool b) {
    if (b) gtk_widget_show((GtkWidget*)diag);
    else gtk_widget_hide((GtkWidget*)diag);
}

void VRGuiRecWidget::update() {
    if (!rec->isRunning()) return;
    string T = "Recording: " + toString(rec->getRecordingLength(), 2);
    gtk_label_set_text(lbl, T.c_str());
}

void VRGuiRecWidget::buttonHandler(int i) {
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
}

