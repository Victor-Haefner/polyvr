#include "VRMenu.h"

#include "core/objects/material/VRMaterial.h"
#include "core/utils/VRFunction.h"
#include "core/utils/toString.h"
#include "selection/VRSelector.h"

using namespace OSG;

VRMenu::VRMenu(string path) : VRGeometry("menu") {
    type = "Menu";
    group = getName();
    if (path == "") return;

    setLeafType(mtype, scale);

    VRMaterialPtr mat = VRMaterial::get(path);
    mat->setLit(false);
    mat->setTexture(path);
    setMaterial(mat);

    close();
}

VRMenuPtr VRMenu::create(string path) { return shared_ptr<VRMenu>(new VRMenu(path) ); }
VRMenuPtr VRMenu::ptr() { return static_pointer_cast<VRMenu>( shared_from_this() ); }

VRMenuPtr VRMenu::append(string path) {
    VRMenuPtr m = VRMenu::create(path);
    m->group = group;
    m->parent = ptr();
    m->setLeafType(mtype, scale);
    m->setLayout(layout, param);
    m->close();
    addChild(m);
    return m;
}

VRMenuPtr VRMenu::getParent() { return parent; }
VRMenuPtr VRMenu::getSelected() { return selected; }

void VRMenu::setCallback(VRFunction<VRMenuPtr>* cb) { callback = cb; }

void VRMenu::trigger() {
    if (callback) (*callback)(ptr());
    else open();
}

void VRMenu::move(int dir) { ; }

void VRMenu::setLinear() {
    auto children = getChildren(false, "Menu");
    int a = 0;
    for (unsigned int i=0; i<children.size(); i++) if (children[i] == selected) a = i;

    for (unsigned int i=0; i<children.size(); i++) {
        float x = (i-a)*(param+scale[0]);
        VRMenuPtr m = static_pointer_cast<VRMenu>(children[i]);
        m->setFrom(Vec3d(x,0,0));
        m->show();
        m->setMeshVisibility(true);
    }
}

void VRMenu::setLeafType(TYPE t, Vec2d s) {
    mtype = t;
    scale = s;
    if (t == SPRITE) setPrimitive("Plane " + toString(s) + " 1 1");
}

void VRMenu::setLayout(LAYOUT l, float p) {
    layout = l;
    param = p;
    show();
    setMeshVisibility(false);
    if (l == LINEAR) setLinear();
    //if (l == CIRCULAR) setCircular(); // TODO
}

void VRMenu::open() {
    VRMenuPtr a = getActive();
    if (a) a->close();
    setActive();
    setLayout(layout, param);
}

void VRMenu::close() {
    for (auto o : getChildren(false, "Menu")) {
        VRMenuPtr m = static_pointer_cast<VRMenu>(o);
        m->setMeshVisibility(false);
    }
}

VRMenuPtr VRMenu::getActive() { return getTopMenu()->active; }
void VRMenu::setActive() { getTopMenu()->active = ptr(); }

VRMenuPtr VRMenu::getTopMenu() {
    VRMenuPtr p = ptr();
    while(p->parent) p = p->parent;
    return p;
}
/*

	class menu:
		def __init__(self, name):
			self.obj = VR.Object('menu')
			self.obj.setVisible(False)
			self.visible = False
			self.sprites = []
			self.menues = []
			self.active = 0
			self.current = None
			self.name = name
			self.tool = None
			self.param = None
			self.doClose = False
			self.onClose = [None, None]

		def hide(self):
			self.visible = False
			self.obj.setVisible(False)

		def toggle(self, dev):
			mc = VR.fmenu.current
			if mc.visible:
				mc.hide()
				if mc.onClose[0]:
					mc.onClose[0](mc.onClose[1])
				return

			self.visible = not self.visible
			self.obj.setVisible(self.visible)
			if self.visible:
				VR.fmenu.current = self
				self.select(0)
			dev.getBeacon().addChild(self.obj)

		def add(self, img, tool, param, isImg, doClose):
			# sprites
			s = VR.Sprite('menu_item')
			s.setSize(0.1,0.07)
			mt = VR.Material('mat')
			mt.setLit(False)
			s.setMaterial(mt)
			s.setDir(0,0,-1)
			if isImg:
				mt.setTexture(img)
			else:
				s.setText(img)
			self.sprites.append(s)
			self.obj.addChild(s)

			# menu
			m = menu(img)
			self.menues.append(m)
			self.layout()
			m.tool = tool
			m.param = param
			m.doClose = doClose
			return m

		def enter(self, dev):
			if not self.visible:
				return
			if not 0 <= self.active < len(self.menues):
				return
			m = self.menues[self.active]
			if len(m.menues) == 0:
				if m.tool:
					m.tool(dev, m.param)
					if not m.doClose:
						return
			self.toggle(dev)
			m.toggle(dev)
			m.layout()

		def select(self, d):
			self.active += d
			self.active = max(0, self.active)
			self.active = min(len(self.menues)-1, self.active)
			VR.fmenu_sel.deselect()
			if 0 <= self.active < len(self.sprites):
				VR.fmenu_sel.select(self.sprites[self.active])
			self.layout()

		def layout(self):
			N = len(self.sprites)
			for i,s in enumerate(self.sprites):
				x = (i - self.active)*0.15
				s.setFrom([x,0.2,-0.5])

		def setOnClose(self, params):
			self.onClose = params


	def setSpeed(dev, s):
		if dev:
			dev.setSpeed(s, 0.5*s)

	def goto(dev, i):
		b = VR.bookmarks
		c = VR.getRoot().find('Default')
		p = VR.Path()
		p.addPoint(c.getFrom(), c.getDir(), [1,0,0], c.getUp())
		p.addPoint(b.getFrom(i), b.getDir(i), [1,0,0], b.getUp(i))
		p.compute(80)
		c.animate(p, 1, 0, True)

	def bookmark(dev, n):
		if not hasattr(VR, 'bookmarks'):
			VR.bookmarks = VR.Recorder()
			VR.bookmarks.setView(0)
		i = VR.bookmarks.capture()
		VR.bm_menu.add(VR.bookmarks.get(i), goto, i, True, True)

	def camfly(dev, n):
		if not hasattr(VR, 'flying'):
			VR.flying = False
		c = VR.getRoot().find('Default')
		if VR.flying:
			c.animationStop()
			VR.flying = False
			return
		p = VR.Path()
		bm = VR.bookmarks
		N = bm.getRecordingSize()
		for i in range(N):
			p.addPoint( bm.getFrom(i), bm.getDir(i), [1,0,0], bm.getUp(i) )
		p.close()
		p.compute(20)
		c.animate(p, 10*N, 0, True, True)
		VR.flying = True

	def toggleClip(dev, param):
		dev.getBeacon().addChild(VR.clip)
		VR.clip.setTree(VR.getRoot())
		VR.clip.setActive(not VR.clip.isActive())
		VR.clip.setFrom(0,0,-0.5)

	def closeClip(param):
		print 'close clip'
		VR.clip.setActive(False)

	def setPoster(dev, data):
		iP = dev.getIntersected()
		if data[2] in iP.getName():
			iP.destroy() #segfault???
			#print i, 'kill'
			return

		l = VR.getRoot().find('Headlight')
		p = VR.Sprite(data[2])
		p.setMaterial(VR.Material(data[0]))
		s = 2
		p.setSize(s*1,s*data[1])
		p.setTexture(data[0])
		p.getMaterial().setLit(False)
		l.addChild(p)
		hp = dev.getIntersection()
		hn = dev.getIntersectionNormal()
		hpd = hp
		hpd[0] += hn[0]*0.2
		hpd[1] += hn[1]*0.2
		hpd[2] += hn[2]*0.2
		p.setFrom(hp)
		n = hn
		n[0] *= -1
		n[1] *= -1
		n[2] *= -1
		p.setDir(hn)
		print p, 'new'

	if not hasattr(VR, 'fmenu') or True:
		VR.fmenu = menu('main')
		VR.fmenu.current = VR.fmenu
		VR.fmenu_sel = VR.Selector()

		# menu.add( name/path/texture, callback, parameters, isTexture, closeOnToggle)
		# clip
		mc = VR.fmenu.add('gui/clip.png', None, None, True, True) # cliping
		mc.add('gui/clip.png', toggleClip, None, True, False)
		mc.onClose = [closeClip, None]

		# bookmarks
		VR.bm_menu = VR.fmenu.add('gui/bookmark.png', None, None, True, True) # bookmarks
		VR.bm_menu.add('gui/fly.png', camfly, None, True, True)
		VR.bm_menu.add('gui/bookmark.png', bookmark, None, True, False)

		# speed
		ms = VR.fmenu.add('gui/speed.png', None, None, True, True) # speed
		ms.add('gui/slow.png', setSpeed, 0.3, True, True)
		ms.add('gui/fast.png', setSpeed, 1.5, True, True)

		# poster
		mp = VR.fmenu.add('gui/paths.png', None, None, True, True) # poster
		for p in listdir('poster'):
			sy = 2.4
			if p[0] in ['W', 'B', 'K']:
				sy = 1.3578
			name = p[:-4]
			mp.add(name, setPoster, ['poster/'+p,sy,name], False, False)

*/
