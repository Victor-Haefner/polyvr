#ifndef VRCOCOAWINDOW_H_INCLUDED
#define VRCOCOAWINDOW_H_INCLUDED

#include "VRWindow.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class VRCocoaWindow : public VRWindow {
	private:
		void init();
		void cleanup();

	public:
		VRCocoaWindow();
		~VRCocoaWindow();

		static VRCocoaWindowPtr create();
		VRCocoaWindowPtr ptr();

        void render(bool fromThread = false) override;

        void save(XMLElementPtr node) override;
        void load(XMLElementPtr node) override;

        void onDisplay();
        void onMouse(int b, int s, int x, int y);
        void onMotion(int x, int y);
        void onKeyboard(int k, int s, int x, int y);
        void onKeyboard_special(int k, int s, int x, int y);
};

OSG_END_NAMESPACE;

#endif //VRCOCOAWINDOW_H_INCLUDED
