#ifndef VRCODECOMPLETION_H_INCLUDED
#define VRCODECOMPLETION_H_INCLUDED

#include <gtk/gtk.h>

struct VRCodeCompletionPrivate;

struct VRCodeCompletion {
	GObject parent;
	VRCodeCompletionPrivate* priv = 0;
};

struct VRCodeCompletionClass {
	GObjectClass parent_class;
};

VRCodeCompletion* vr_code_completion_new();

#endif // VRCODECOMPLETION_H_INCLUDED
