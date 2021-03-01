#include "VRScenegraphInterface.h"
#include "core/networking/VRSocket.h"
#include "core/scene/VRSceneManager.h"
#include "core/scene/VRScene.h"
#include "core/networking/VRNetworkManager.h"
#include "core/utils/VRFunction.h"
#include "core/utils/toString.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/material/VRMaterial.h"
#include "core/tools/selection/VRSelector.h"
#include "core/math/kinematics/VRConstraint.h"
#include "core/objects/object/VRObjectT.h"

#include <OpenSG/OSGMatrix.h>
#include <OpenSG/OSGColor.h>
#include <OpenSG/OSGGeoProperties.h>

using namespace OSG;

namespace OSG {
    struct VRScenegraphInterface::Mate {
        VRTransformPtr a0;
        VRTransformPtr b0;
        vector<int> DoF0;
        pair<PosePtr, PosePtr> C_AandB0;
        PosePtr C0;
        Vec3d IjkA0;
        string TypeC0;
        vector<float> MinMax;
    };
}

VRScenegraphInterface::VRScenegraphInterface(string name) : VRObject(name) {
    resetWebsocket();
}

VRScenegraphInterface::~VRScenegraphInterface() {}

VRScenegraphInterfacePtr VRScenegraphInterface::ptr() { return static_pointer_cast<VRScenegraphInterface>( shared_from_this() ); }
VRScenegraphInterfacePtr VRScenegraphInterface::create(string name)  { return VRScenegraphInterfacePtr( new VRScenegraphInterface(name) ); }

void VRScenegraphInterface::enableTransparency(bool b) { handleTransparency = b; }

void VRScenegraphInterface::clear() {
    clearChildren();
    materials.clear();
    objects.clear();
    objectIDs.clear();
	meshes.clear();
	transforms.clear();
	Mate_dictionary.clear();
	//CADVRselection.clear();
}

void VRScenegraphInterface::setPort(int p) { if (p == port) return; port = p; resetWebsocket(); }

void VRScenegraphInterface::send(string msg) { socket->answerWebSocket(clientID, msg); }

void VRScenegraphInterface::resetWebsocket() {
    if (socket) VRSceneManager::get()->remSocket(socket->getName());
    socket = VRSceneManager::get()->getSocket(port);
    cb = new VRHTTP_cb( "scenegraph interface callback", bind(&VRScenegraphInterface::ws_callback, this, _1) );
    socket->setHTTPCallback(cb);
    socket->setType("http receive");
}

void VRScenegraphInterface::addCallback(VRMessageCbPtr cb) { customHandlers.push_back(cb); }

void VRScenegraphInterface::ws_callback(void* _args) {
	HTTP_args* args = (HTTP_args*)_args;
    if (!args->websocket) return;

    clientID = args->ws_id;
    string msg = args->ws_data;
    if (args->ws_data.size() == 0) return;

    handle(msg);
    //auto job = VRUpdateCb::create("sgi_handler", bind(&VRScenegraphInterface::handle, this, msg));
    //VRScene::getCurrent()->queueJob(job);
}

void VRScenegraphInterface::loadStream(string path) {
    clear();
    ifstream f(path);
    string line;
    if (!f.is_open()) return;
    while ( getline (f,line) ) handle(line);
	f.close();
}

template<class T>
vector<T> parseVec(string& data) {
    vector<T> res;
    stringstream ss(data);
    T f;
    while (ss >> f) res.push_back(f);
    return res;
}

template<class T, typename V>
void parseOSGVec(string& data, V& v) {
    stringstream ss(data);
    T f;
    while (ss >> f) v->addValue(f);
}

// T2 allows to pass type double because stringstreams may fail to convert scientific notations to float
template<class T, class U, class T2, typename V>
void parseOSGVec2(string& data, V& v) {
    int N = sizeof(U)/sizeof(T);
    stringstream ss(data);
    T2 f;
    U u;
    int i=0;
    while (ss >> f) {
        u[i] = f;
        i++;
        if (i == N) {
            i = 0;
            v->addValue(u);
        }
    }

	if (ss.fail()) {
		cout << " parseOSGVec2 failed!" << endl;
	}
}

VRObjectPtr VRScenegraphInterface::getObject(string objID) {
    if (objects.count(objID)) return objects[objID];
    return 0;
}

string VRScenegraphInterface::getObjectID(VRObjectPtr obj) {
    if (objectIDs.count(obj.get())) return objectIDs[obj.get()];
    return "";
}

void VRScenegraphInterface::buildKinematics(vector<string> m) {
    //cout << "VRScenegraphInterface::buildKinematics " << m[0] << endl;
    auto toFloat2 = [&](string data) {
        replace( data.begin(), data.end(), ',', '.');
        return toFloat(data);
    };

	auto parse = [&](string data) {
	    Vec3d res;
	    int i=0;
	    for (string s : splitString(data, ';')) {
            res[i] = toFloat2(s);
            i++;
            cout << " buildKinematics::parse " << s << " " << i << " " << res[i] << endl;
	    }
	    cout << "buildKinematics::parse " << data << " -> " << res << endl;
		return res;
	};

	string Type, Alignment, CanBeFlipped;
	VRTransformPtr a, b, c;
	string a_Name, b_Name, c_Name;
	string Mates, Mates2;
	string IsFixedA, IsFixedB, IsFixedC;
	string MateEntityTypeA, MateEntityTypeB, MateEntityTypeC;
	string Radius1A, Radius1B, Radius1C;
	string Radius2A, Radius2B, Radius2C;
	Vec3d XyzA, XyzB, XyzC;
	Vec3d IjkA, IjkB, IjkC;
	float MaximumVariation = 0, MinimumVariation = 0;
	PosePtr newC;

	for (unsigned int i = 0; i< m.size(); i++) {
		if (m[i] == "Type") Type = m[i+1];
		else if (m[i] == "Alignment") Alignment = m[i+1];
		else if (m[i] == "Can be flipped") CanBeFlipped = m[i+1];
		else if (m[i] == "A: Component") {
			auto o = dynamic_pointer_cast<VRTransform>(getObject(m[i+1]));
			if (o) a = o;
			a_Name = m[i+1];
		}
		else if (m[i] == "B: Component") {
			auto o = dynamic_pointer_cast<VRTransform>(getObject(m[i+1]));
			if (o) b = o;
			b_Name = m[i+1];
			if (a) a->setPickable(true);
			if (b) b->setPickable(true);
			Mates = a_Name + " + " + b_Name;
			Mates2 = b_Name + " + " + a_Name; //alternativer Name
		}
		else if (m[i] == "C: Component") {
			if (b_Name == a_Name) { // Type 11
				auto o = dynamic_pointer_cast<VRTransform>(getObject(m[i+1]));
                if (o) b = o;
				b_Name = m[i+1];
				if (a) a->setPickable(true);
				if (b) b->setPickable(true);
				Mates = a_Name + " + " + b_Name;
				Mates2 = b_Name + " + " + a_Name; //alternativer Name
			}
		}
		else if (m[i] == "A: IsFixed") IsFixedA = m[i+1];
		else if (m[i] == "B: IsFixed") IsFixedB = m[i+1];
		else if (m[i] == "C: IsFixed") {
			IsFixedC = m[i+1];
			if (b_Name == a_Name) IsFixedB = IsFixedC;
		}

		else if (m[i] == "A: Mate entity type") MateEntityTypeA = m[i+1];
		else if (m[i] == "B: Mate entity type") MateEntityTypeB = m[i+1];
		else if (m[i] == "C: Mate entity type") MateEntityTypeC = m[i+1];

		else if (m[i] == "A: (x,y,z)") XyzA = parse(m[i+1]);
		else if (m[i] == "B: (x,y,z)") XyzB = parse(m[i+1]);
		else if (m[i] == "C: (x,y,z)") XyzC = parse(m[i+1]);

		else if (m[i] == "A: (i,j,k)") IjkA = parse(m[i+1]);
		else if (m[i] == "B: (i,j,k)") IjkB = parse(m[i+1]);
		else if (m[i] == "C: (i,j,k)") IjkC = parse(m[i+1]);

		else if (m[i] == "A: Radius 1") Radius1A = m[i+1];
		else if (m[i] == "B: Radius 1") Radius1B = m[i+1];
		else if (m[i] == "C: Radius 1") Radius1C = m[i+1];

		else if (m[i] == "A: Radius 2") Radius2A = m[i+1];
		else if (m[i] == "B: Radius 2") Radius2B = m[i+1];
		else if (m[i] == "C: Radius 2") Radius2C = m[i+1];

		else if (m[i] == "MaximumVariation") MaximumVariation = toFloat2(m[i+1]);
		else if (m[i] == "MinimumVariation") MinimumVariation = toFloat2(m[i+1]);
	}

	/**
	Variabeln
	*/
	string Joint = "unknown"; //build variable
	string TypeC = ""; //C variable
    Mate Values0;
	Values0.MinMax = {false,0,0,0,false,1,0,0,false,2,0,0,false,3,0,0,false,4,0,0,false,5,0,0};
	//MinMax variable

	/**
	minmax[] True/False  Translation x/y/z + Rotation x,y,z
	minmax[] Dof
	minmax[] Min
	minmax[] Max
	0(tx),4(ty),8(tz),12(rx),16(ry),20(rz)
	*/
	Vec2i dof1(1,5);
	Vec2i dof2(2,5);

	/**
	2: Mate bib
	*/

	if (IsFixedB == "True") {
		swap(Mates, Mates2);
		swap(a, b);
		swap(MateEntityTypeA, MateEntityTypeB);
		swap(XyzA, XyzB);
		swap(IjkA, IjkB);
		swap(Radius1A, Radius1B);
		swap(Radius2A, Radius2B);
	}

	if (Mate_dictionary.count(Mates) || Mate_dictionary.count(Mates2)) {
		if (Mate_dictionary.count(Mates)) Values0 = Mate_dictionary[Mates];  //get Values
		if (Mate_dictionary.count(Mates2)) {
			Values0 = Mate_dictionary[Mates2];
            swap(Mates, Mates2);
            swap(a, b);
            swap(MateEntityTypeA, MateEntityTypeB);
            swap(XyzA, XyzB);
            swap(IjkA, IjkB);
            swap(Radius1A, Radius1B);
            swap(Radius2A, Radius2B);
		}
	}

	auto check = [&]() {
		if (Mate_dictionary.count(Mates) || Mate_dictionary.count(Mates2)) return "edit";
		else return "build";
	};

	/**
	3: Mathe Funktionen
	*/

	auto buildnewUp = [&](Vec3d Dir) {
		Vec3d newUp = Vec3d(0,1,0);
		if (abs(Dir[1]) > abs(Dir[0]) && abs(Dir[1]) > abs(Dir[2])) newUp = Vec3d(0,0,1);
		return newUp;
	};

	auto pnt2line = [&](Vec3d v, Vec3d g, Vec3d p) { //v=vector g=geradenstart p=teilursprung
	    cout << "pnt2line " << v << " " << g << " " << p << " " << endl;
		Vec3d x = (p.dot(v) - g.dot(v)) /v.dot(v); //kürzester abstand
		return g + v*x;
	};

	auto pnt2plane = [&](Vec3d v, Vec3d g, Vec3d p) { //v=normalenvector g= punkt auf ebene p=teilursprung
		Vec3d x = (g.dot(v) - p.dot(v)) /v.dot(v); //kürzester abstand
		return p + v*x;
	};

	auto intersect_line2line = [&](PosePtr C, PosePtr C0) -> string {
	    newC = C0;
		Vec3d pnt1 = C->pos();
		Vec3d pnt2 = C0->pos();
		Vec3d g = pnt2-pnt1;
		Vec3d h = C0->dir().cross(g);
		Vec3d k = C->dir().cross(g);
		Vec3d Para = C->dir().cross(C0->dir());
		if (Para.length() < 1e-6) { //gleich/parallel
			Vec3d d = C->dir();
			float t;
			if      (abs(d[0]) > 1e-6) t = g[0]/d[0];
			else if (abs(d[1]) > 1e-6) t = g[1]/d[1];
			else if (abs(d[2]) > 1e-6) t = g[2]/d[2];
			else {
				//print "no vector?";
				t = 0;
			}

			if ((pnt1 + d*t) == pnt2) { TypeC = "1"; return TypeC; } //!!!!

			else {
				//print "not possible: parallel lines"
                TypeC = "1";
                return TypeC;
			}
		} else if (h.length()< 1e-6 || k.length()< 1e-6) {
			//print "not possible: skew lines";
			TypeC = "1";
			return TypeC;
		} else {
			Vec3d cross1 = C0->dir().cross(C->dir());
			Vec3d x;
			if (cross1.dot(h) > 0) x = h.length()/k.length();
			else x = -h.length()/k.length();
			TypeC = "0";
			C0->setPos(pnt1 + C->dir()*x);
			return TypeC;
		}
	};

	auto intersect_line2plane = [&](PosePtr pose1, PosePtr pose2) { // pose2 ist ebene
	    newC = pose1;
		Vec3d g = pose1->pos();
		Vec3d v = pose1->dir();
		Vec3d p = pose2->pos();
		Vec3d n = pose2->dir();
		if (abs(v.dot(n)) < 1e-6 && abs(g.dot(n)-p.dot(n)) < 1e-6) {
			TypeC = "1";
		} else if (abs(v.dot(n)) < 1e-6 && abs(g.dot(n)-p.dot(n)) > 1e-6)  {
			//print "not possible: parallel line/plane";
			TypeC = "1";
		} else {
			Vec3d x = (p.dot(n)-g.dot(n)) / v.dot(n);
			pose1->setPos(g + v*x);
			TypeC = "0";
        }
		return TypeC;
	};

	auto intersect_plane2plane = [&](PosePtr C, PosePtr C0) {
	    newC = C;
		Vec3d g = C->pos();
		Vec3d v = C->dir();
		Vec3d p = C0->pos();
		Vec3d n = C0->dir();
		Vec3d s = v.cross(n);
		if (s.length()< 1e-6) {
			if (abs(g.dot(n)-p.dot(n)) < 1e-6) {
				TypeC = "3";
			} else {
				TypeC = "3";
				//print "not possible: parallel planes";
			}
		} else {
			Vec3d d1 = g.dot(v);
			Vec3d d2 = p.dot(n);
			Vec3d s1 = (d1*n.dot(n)-d2*v.dot(n))/(v.dot(v)*n.dot(n)-v.dot(n)*v.dot(n));
			Vec3d s2 = (d2*v.dot(v)-d1*v.dot(n))/(v.dot(v)*n.dot(n)-v.dot(n)*v.dot(n));
			Vec3d x = v*s1 +n*s2;
			C->set(x,s,v);
			TypeC = "1";
		}

		return TypeC;
	};

	auto buildC_AandB = [&](PosePtr C, VRTransformPtr a, VRTransformPtr b) {
		PosePtr A = a->getPose();
		PosePtr B = b->getPose();
		A->invert();
		B->invert();
		if (!C) return make_pair(A,B);
		PosePtr C_A = A->multRight(C); // A⁻¹*C
		PosePtr C_B = B->multRight(C); // B⁻¹*C
		return make_pair(C_A,C_B);
	};

	auto Find_new_C = [&](string TypeA, string TypeB, PosePtr Pose1, PosePtr Pose2) { //vollständigkeit?
		if (TypeA == "1" && TypeB == "1")  {
			return intersect_line2line(Pose1,Pose2);
		}

		else if (TypeA == "1" && TypeB == "3")  {
			return intersect_line2plane(Pose1,Pose2);
		}

		else if (TypeA == "3" && TypeB == "1")  {
			return intersect_line2plane(Pose2,Pose1);
		}

		else if (TypeA == "3" && TypeB == "3")  {
			return intersect_plane2plane(Pose1,Pose2);
		}

		else if (TypeA == "unknown") { newC = Pose2; return TypeB; }
		else if (TypeB == "unknown") { newC = Pose1; return TypeA; }

		else {
			//print "No new C possible!";
			//print TypeA,TypeB;
			newC = 0;
			return TypeB;
		}
	};

	/*auto toDeg = [&](float a) {
		return a*180/3.14159265359;
	};*/

	auto findMinMax = [&](float Min, float Max, PosePtr C) {
		Vec3d z = C->dir();
		Vec3d y = C->up();
		Vec3d x = -z.cross(y);
		int m = 0;
		if (Type == "5") {
			if (abs(IjkA.cross(x).length()) < 1e-6) {
				m = 0;
			} else if (abs(IjkA.cross(y).length()) < 1e-6) {
				m = 4;
			} else if (abs(IjkA.cross(z).length()) < 1e-6) {
				m = 8;
			} else {
				m = 8;
				//print "Error: Limit(advanced) wrong directionvector"
			}
        }
		if (Type == "6") {
			Vec3d v = IjkA + IjkB;
			if (abs(v.dot(x)) < 1e-6) {
				m = 12;
			} else if (abs(v.dot(y)) < 1e-6) {
				m = 16;
			} else if (abs(v.dot(z)) < 1e-6) {
				m = 20;
			} else {
				m = 20;
				//print "Error: Limit(advanced) wrong rotationvector"
			}
        }
		Values0.MinMax[m] = true;
		Values0.MinMax[m+2] = Min; //Min
		Values0.MinMax[m+3] = Max; //Max

		return Values0.MinMax;
	};


	/**
	4: Fall-Unterscheidung
	*/
	/**
	4.1: Standart
	(Coincident,parallel,perpendicular,tangent,concentric,lock,distance,angle)
	momentan sind viele typen(coincident/parallel,perpendicular etc. größtenteils gleich,
	was an der Aufbauweiße von Joints : polyvr liegt. Später (mit zwei Joints) wird sich das jedoch ändern
	darum jetzt schon die unterscheidung
	*/

    vector<int> DoF;
    PosePtr C = Pose::create();
    pair<PosePtr, PosePtr> C_AandB = make_pair(Pose::create(), Pose::create());
	if (Type == "0") { //coincident
		if (MateEntityTypeA == "0" && a && b) { //Point
			DoF = {3,4,5}; //nur Rotation (xyz)
			C = Pose::create(XyzA,IjkA,buildnewUp(IjkA));
			C_AandB = buildC_AandB(C,a,b);
			TypeC = "0";
			Joint = check();
		}

		else if (MateEntityTypeA == "1" && a && b) {  //Line
			DoF = {2,5};			//Translation : z Rotation um z
			C = Pose::create(pnt2line(IjkA,XyzA,a->getFrom()),IjkA,buildnewUp(IjkA));
			C_AandB = buildC_AandB(C,a,b);
			TypeC = "1";
			Joint = check();
		}

		else if (MateEntityTypeA == "2" && a && b) { //Circle
			DoF = {5}; //Rotation um z
			C = Pose::create(XyzA,IjkA,buildnewUp(IjkA));
			C_AandB = buildC_AandB(C,a,b);
			TypeC = "0";
			Joint = check();
		}

		else if (MateEntityTypeA == "3" && a && b) {  //Plane
			DoF = {0,1,5};  // Translation : x&y Rotation um z
			C = Pose::create(pnt2plane(IjkA,XyzA,a->getFrom()),IjkA,buildnewUp(IjkA));
			C_AandB = buildC_AandB(C,a,b);
			TypeC = "3";
			Joint = check();
		}

		else if (MateEntityTypeA == "4" && a && b) {  //Cylinder
			DoF = {2,5};			//Translation : z Rotation um z
			C = Pose::create(pnt2line(IjkA,XyzA,a->getFrom()),IjkA,buildnewUp(IjkA));
			C_AandB = buildC_AandB(C,a,b);
			TypeC = "1";
			Joint = check();
		}

		else if (MateEntityTypeA == "5" && a && b) {  //Sphere
			DoF = {3,4,5}; //nur Rotation (xyz)
			C = Pose::create(XyzA,IjkA,buildnewUp(IjkA));
			C_AandB = buildC_AandB(C,a,b);
			TypeC = "0";
			Joint = check();
		}

		/*else if (MateEntityTypeA == "6")  //Set
			//print "todo: 0.6:Set?"
		else if (MateEntityTypeA == "7")  //Cone
			//print "not possible yet: 0.7:Cone"
		else if (MateEntityTypeA == "8")  //SweptSurface
			//print "not possible yet: 0.8: SweptSurface"
		else if (MateEntityTypeA == "9")  //MultipleSurface
			//print "not possible yet: 0.9: MultipleSurface"
		else if (MateEntityTypeA == "10") //GenSurface
			//print "not possible yet: 0.10: GenSurface"
		else if (MateEntityTypeA == "11") //Ellipse
			//print "not possible yet: 0.11: Ellipse"
		else if (MateEntityTypeA == "12") //GeneralCurve
			//print "not possible yet: 0.12: GeneralCurve"
		else if (MateEntityTypeA == "13") //UNKNOWN
			//print "not possible: 0.13: UNKNOWN"
			*/
	}

	else if (Type == "1") { //concentric

		if (MateEntityTypeA == "2" && a && b) {  //Circle
			DoF = {2,5}; //Rotation um z
			C = Pose::create(XyzA,IjkA,buildnewUp(IjkA));
			C_AandB = buildC_AandB(C,a,b);
			TypeC = "1";
			Joint = check();
        }

		else if (MateEntityTypeA == "4" && a && b) {  //Cylinder
			DoF = {2,5};			//Translation : z Rotation um z
			C = Pose::create(pnt2line(IjkA,XyzA,a->getFrom()),IjkA,buildnewUp(IjkA));
			C_AandB = buildC_AandB(C,a,b);
			TypeC = "1";
			Joint = check();
        }

		else if (MateEntityTypeA == "5" && a && b) {  //Sphere
			//print "work : progress: 1.5 (2 joints needed)";
			DoF = {2,3,4,5}; //Rotationfrei, translation nur : ausrichtung (geht noch nicht)
			C = Pose::create(XyzA,IjkA,buildnewUp(IjkA));
			C_AandB = buildC_AandB(C,a,b);
			TypeC = "0";
			Joint = check();
        }

		else if (MateEntityTypeA == "7" && a && b) {  //Cone
			DoF = {2,5};			//Translation : z Rotation um z
			C = Pose::create(pnt2line(IjkA,XyzA,a->getFrom()),IjkA,buildnewUp(IjkA));
			C_AandB = buildC_AandB(C,a,b);
			TypeC = "1";
			Joint = check();
        }
	}

	else if (Type == "2") { //perpendicular

		if (MateEntityTypeA == "1") {    //Line
			//print "work : progress: 2.1 (2 joints needed)";
			//DoF = {0,2,5] //Rotation um z, Translation : z und : ausrichtung(geht noch nicht: Dof 0,1)
        }

		else if (MateEntityTypeA == "3" && a && b) {  //Plane
			if (MateEntityTypeB == "1" || MateEntityTypeB == "4" || MateEntityTypeB == "7")  { //Line,Cylinder,Cone
				DoF = {0,1,2,5};
				C = Pose::create(pnt2plane(IjkA,XyzA,a->getFrom()),IjkA,buildnewUp(IjkA));//?
				C_AandB = buildC_AandB(C,a,b);
				TypeC = "unknown";
				Joint = check();
				//ausrichtung ijkA= ebenen normale, ijkB = senkrecht dazu (bei ebene zu ebene)
            }
			else if (MateEntityTypeB == "3" && a && b) {
				DoF = {0,1,2,4,5};
				C = Pose::create(pnt2plane(IjkA,XyzA,a->getFrom()),IjkA,buildnewUp(IjkA));
				C_AandB = buildC_AandB(C,a,b);
				TypeC = "unknown";
				Joint = check();
				// ijkB bei Line = gleiche ausrichtung wie ebenen normale
            }
			else {
				//print "Error_2.3.",MateEntityTypeB;
            }
		}

		else if (MateEntityTypeA == "4") {  //Cylinder
			//print "work : progress: 2.4 (2 joints needed)";
			//siehe Line
		}

		else if (MateEntityTypeA == "7") {  //Cone
			//print "work : progress: 2.7 (2 joints needed)";
			//siehe Line
		}
    }

	else if (Type == "3") { //parallel

		if (MateEntityTypeA == "1" && a && b) {    //Line
			if (MateEntityTypeB == "1" || MateEntityTypeB == "4" || MateEntityTypeB == "7")  {
				DoF = {0,1,2,5};
				C = Pose::create(pnt2line(IjkA,XyzA,a->getFrom()),IjkA,buildnewUp(IjkA));
            }
			else if (MateEntityTypeB == "3") {
				DoF = {0,1,2,4,5};
				C = Pose::create(pnt2line(IjkA,XyzA,a->getFrom()),IjkA,buildnewUp(IjkA));
            }
			C_AandB = buildC_AandB(C,a,b);
			TypeC = "unknown";
			Joint = check();
		}

		else if (MateEntityTypeA == "3" && a && b) {    //Plane
			if (MateEntityTypeB == "1") {
				DoF = {0,1,2,4,5};
				C = Pose::create(pnt2plane(IjkA,XyzA,a->getFrom()),IjkB,buildnewUp(IjkB));
            }
			else if (MateEntityTypeB == "3") {
				DoF = {0,1,2,5};
				C = Pose::create(pnt2plane(IjkA,XyzA,a->getFrom()),IjkA,buildnewUp(IjkA));
            }
			C_AandB = buildC_AandB(C,a,b);
			TypeC = "unknown";
			Joint = check();
		}

		else if (MateEntityTypeA == "4" && a && b) {    //Cylinder
			DoF = {0,1,2,5};
			C = Pose::create(pnt2line(IjkA,XyzA,a->getFrom()),IjkA,buildnewUp(IjkA));
			C_AandB = buildC_AandB(C,a,b);
			TypeC = "unknown";
			Joint = check();
		}

		else if (MateEntityTypeA == "7" && a && b) {    //Cone
			DoF = {0,1,2,5};
			C = Pose::create(pnt2line(IjkA,XyzA,a->getFrom()),IjkA,buildnewUp(IjkA));
			C_AandB = buildC_AandB(C,a,b);
			TypeC = "unknown";
			Joint = check();
		}
	}

	else if (Type == "4") { //tangent
		//print "not possible: 4: tangent (2 joints needed)"
		/*if MateEntityTypeA == "1")    //Line
		else if (MateEntityTypeA == "3")    //Plane
		else if (MateEntityTypeA == "4")    //Cylinder
		else if (MateEntityTypeA == "5")    //Sphere
		else if (MateEntityTypeA == "7")    //Cone
		else if (MateEntityTypeA == "8")  //SweptSurface
			//print "not possible yet: 0.8: SweptSurface"
		else if (MateEntityTypeA == "9")  //MultipleSurface
			//print "not possible yet: 0.9: MultipleSurface"
		else if (MateEntityTypeA == "10") //GenSurface
			//print "not possible yet: 0.10: GenSurface"
        */

	} else if (Type == "5" && MaximumVariation == 0 && MinimumVariation == 0)  {  //distance
		if (MateEntityTypeA == "0" && a && b) {   //Point
			DoF = {3,4,5}; //nur Rotation (xyz)
			C = Pose::create(XyzA,IjkA,buildnewUp(IjkA));
			C_AandB = buildC_AandB(C,a,b);
			TypeC = "0";
			Joint = check();
		}
		else if (MateEntityTypeA == "1" && a && b) {    //Line
			DoF = {2,5};			//Translation : z Rotation um z
			C = Pose::create(pnt2line(IjkA,XyzA,a->getFrom()),IjkA,buildnewUp(IjkA));
			C_AandB = buildC_AandB(C,a,b);
			TypeC = "1";
			Joint = check();
		}
		else if (MateEntityTypeA == "3" && a && b) {    //Plane
			DoF = {0,1,5};  // Translation : x&y Rotation um z
			C = Pose::create(pnt2plane(IjkA,XyzA,a->getFrom()),IjkA,buildnewUp(IjkA));
			C_AandB = buildC_AandB(C,a,b);
			TypeC = "3";
			Joint = check();
		}
		else if (MateEntityTypeA == "4" && a && b) {    //Cylinder
			DoF = {2,5};			//Translation : z Rotation um z
			C = Pose::create(pnt2line(IjkA,XyzA,a->getFrom()),IjkA,buildnewUp(IjkA));
			C_AandB = buildC_AandB(C,a,b);
			TypeC = "1";
			Joint = check();
		}
		else if (MateEntityTypeA == "5" && a && b) {    //Sphere
			DoF = {3,4,5}; //nur Rotation (xyz)
			C = Pose::create(XyzA,IjkA,buildnewUp(IjkA));
			C_AandB = buildC_AandB(C,a,b);
			TypeC = "0";
			Joint = check();
		}
		else if (MateEntityTypeA == "7" && a && b) {    //Cone
			DoF = {5}; //nur Rotation um z
			C = Pose::create(pnt2line(IjkA,XyzA,a->getFrom()),IjkA,buildnewUp(IjkA));
			C_AandB = buildC_AandB(C,a,b);
			TypeC = "0";
			Joint = check();
		}
		else if (MateEntityTypeA == "12" && a && b) {    //GeneralCurce
			//print "not possible yet: 5.12 Distance to Curve"
		}
	}

	else if (Type == "6" && MaximumVariation == 0 && MinimumVariation == 0 && a && b)  {  //angle

		if (MateEntityTypeA == "1" || MateEntityTypeA == "4" || MateEntityTypeA == "7")  {    //Line,Cylinder,Cone
			DoF = {2,5};
			//print "work : progress: 6.1/4/7 (2 joints needed)" //2 joints DoF=[2,5] dir()1=IjkA; dir()2=IjkB
			C = Pose::create(pnt2line(IjkA,XyzA,a->getFrom()),IjkA,buildnewUp(IjkA));
			C_AandB = buildC_AandB(C,a,b);
			TypeC = "1";
			Joint = check();
		}

		else if (MateEntityTypeA == "3") {    //Plane
			DoF = {0,1,2,5};
			C = Pose::create(pnt2plane(IjkA,XyzA,a->getFrom()),IjkB,buildnewUp(IjkB));
			C_AandB = buildC_AandB(C,a,b);
			TypeC = "unknown";
			Joint = check();
		}
    }


	else if (Type == "16" && a && b) { //Lock
        DoF = {}; //0
        C = Pose::create(XyzA,IjkA,buildnewUp(IjkA));
        TypeC = "0";
        C_AandB = buildC_AandB(C,a,b);
        Joint = check();
    }

	/**
	4.2: Advanced p-->possible, n-->not possible yet
	(limit(p), linear/linear coupler(n), path(n), profile center(?), symmetry(n),width(?))
	*/

    PosePtr D, E;
	if (Type == "5" && (abs(MaximumVariation) > 1e-6 || abs(MinimumVariation) > 1e-6))  { //limit test
		if (Mate_dictionary.count(Mates)  || Mate_dictionary.count(Mates2))  {
			DoF = Values0.DoF0;
			C = Values0.C0;
			Values0.MinMax = findMinMax(MinimumVariation,MaximumVariation,C);
			Joint = check();
		} else {
			//print "todo: 5(advanced)  {Limit (new)"								////////?
		}
    } else if (Type == "6" && (abs(MaximumVariation) > 1e-6 || abs(MinimumVariation) > 1e-6))  { //limit test
		if (Mate_dictionary.count(Mates)  || Mate_dictionary.count(Mates2))  {
			DoF = Values0.DoF0;
			C = Values0.C0;
			Values0.MinMax = findMinMax(MinimumVariation,MaximumVariation,C);
			Joint = check();
        }
		else {
			if (MateEntityTypeA == "3" && MateEntityTypeB == "3")  {
				DoF = {2,5};
				D = Pose::create(XyzA,IjkA,buildnewUp(IjkA));
				E = Pose::create(XyzB,IjkB,buildnewUp(IjkB));
				MateEntityTypeA = "1";
				intersect_plane2plane(D,E);
				C = newC;
				Values0.MinMax = findMinMax(MinimumVariation,MaximumVariation,C);
				Joint = check();
			}
			else {
				DoF = {2,5};
				D = Pose::create(XyzA,IjkA,buildnewUp(IjkA));
				E = Pose::create(XyzB,IjkB,buildnewUp(IjkB));
				MateEntityTypeA = "1";
				intersect_line2line(D,E);
				C = newC;
				Values0.MinMax = findMinMax(MinimumVariation,MaximumVariation,C);
				Joint = check();
			}
        }
    }

	else if (Type == "11") { //Width
		//print "todo: Width"
		/*DoF = {0,1,5]
		C = Pose::create(XyzA,IjkA,buildnewUp(IjkA))
		// XyzA finden? => gleicher abstand zu beiden ebenen?
		C_AandB = buildC_AandB(C,a,b)
		TypeC = "3"
		Joint = check()*/
    }

	else if (Type == "24" && a && b) {//Profil Center
		DoF = {}; //0
		C = Pose::create(XyzA,IjkA,buildnewUp(IjkA));
		TypeC = "0";
		C_AandB = buildC_AandB(C,a,b);
		Joint = check();
    }

	/**
	4.3 Mechanical p-->possible, n-->not possible yet
	(cam-follower(n), gear(n), hinge(?), rack && pinion(n), screw(n), slot(n),universal joint(n))
	*/

	if (Type == "22" && a && b) {//Hinge
		DoF = {5}; //rotation
		PosePtr C1 = Pose::create(pnt2line(IjkA,XyzA,a->getFrom()),IjkA,buildnewUp(IjkA)); //cylinder
		PosePtr C2 = Pose::create(pnt2plane(IjkC,XyzC,a->getFrom()),IjkC,buildnewUp(IjkC)); //plane
		Find_new_C("1","3",C1,C2);
		C = newC;
		TypeC = "0";
		if ((abs(MaximumVariation) > 1e-6 || abs(MinimumVariation) > 1e-6))  {
			Type == "6";
			Values0.MinMax = findMinMax(MinimumVariation,MaximumVariation,C);
		}
		C_AandB = buildC_AandB(C,a,b);
		Joint = check();
    }

	/**
	5: Joint Funktionen
	*/

	auto setupJoint = [&](VRTransformPtr a, VRTransformPtr b, vector<int> DoF, pair<PosePtr, PosePtr> C_AandB, vector<float> MinMax) {
		if (!a || !b) return;

		PosePtr C_A = C_AandB.first;
		PosePtr C_B = C_AandB.second;

		//print "a:",a->getFrom(),a->getDir(),a->getUp()
		//print "b:",b->getFrom(),b->getDir(),b->getUp()
		//print "DoF:",DoF
		//print "C_A:",C_A->pos(),C_A->dir(),C_A->up()
		//print "C_B:",C_B->pos(),C_B->dir(),C_B->up()
		//print "C:",C->pos(),C->dir(),C->up()

		auto c = VRConstraint::create();
		c->free(DoF);
		if (C_A) c->setReferenceA(C_A);
		if (C_B) c->setReferenceB(C_B);
		int m = 0;
		while (m < 21) {
			if (MinMax[m] == true) {
				//print "MinMax","Dof:",MinMax[m+1],"Degree:",toDeg(MinMax[m+2]),toDeg(MinMax[m+3])
				c->setMinMax(MinMax[m+1],MinMax[m+2],MinMax[m+3]); //winkel : radian(0-2pi)
			}
			m = m + 4;
		}

		a->attach(b, c);
	};

	auto buildnewDoF = [&](vector<int> DoF, vector<int> DoF0) {
	    vector<int> newDoF;
	    for (int i : DoF) {
            for (int j : DoF0) {
                if (i == j) { newDoF.push_back(i); continue; }
            }
	    }
		return newDoF;
	};

	/**
	6: Joint bilden
	*/

	if (Joint == "build") {
		//print Joint + ") " + Mates;
		setupJoint(a,b,DoF,C_AandB,Values0.MinMax);
		Mate Values;
		Values.a0 = a;
		Values.b0 = b;
		Values.DoF0 = DoF;
		Values.C_AandB0 = C_AandB;
		Values.C0 = C;
		Values.IjkA0 = C->dir();
		Values.TypeC0 = TypeC;
		Values.MinMax = Values0.MinMax;
		Mate_dictionary[Mates] = Values;
	}

	else if (Joint == "edit") {
		//print Joint + ") " + Mates
		Vec3d Cross = C->dir().cross(Values0.C0->dir());

		if (Cross.length()< 1e-6) {
			//print "edit Joint"
			vector<int> newDoF = buildnewDoF(DoF,Values0.DoF0);

			if (C != Values0.C0) {
				if (TypeC == "0") {
					setupJoint(Values0.a0,Values0.b0,newDoF,Values0.C_AandB0,Values0.MinMax);
				}

				else if (MateEntityTypeA == "0") {
					setupJoint(a,b,newDoF,C_AandB,Values0.MinMax);
				}

				else {
					Find_new_C(MateEntityTypeA,Values0.TypeC0,C,Values0.C0);
					Values0.TypeC0 = TypeC;
					if (newC) Values0.C0 = newC;
					Values0.C_AandB0 = buildC_AandB(Values0.C0,a,b);
					setupJoint(Values0.a0,Values0.b0,newDoF,Values0.C_AandB0,Values0.MinMax);
				}
            }

			else setupJoint(a,b,newDoF,Values0.C_AandB0,Values0.MinMax);

			Mate Values;
            Values.a0 = Values0.a0;
            Values.b0 = Values0.b0;
            Values.DoF0 = newDoF;
            Values.C_AandB0 = Values0.C_AandB0;
            Values.C0 = Values0.C0;
            Values.IjkA0 = Values0.C0->dir();
            Values.TypeC0 = Values0.TypeC0;
            Values.MinMax = Values0.MinMax;

			if (Mate_dictionary.count(Mates))  { Mate_dictionary.erase(Mates); }
			else if (Mate_dictionary.count(Mates2))  { Mate_dictionary.erase(Mates2); }
			Mate_dictionary[Mates] = Values;
		} else {
			//print "Error: Ausrichtung falsch"
		}
    }


	else if (Joint == "unknown") {
		//print "Todo: Case unknown "," Type: " +Type," MateEntityTypeA: "+MateEntityTypeA
	}
	/**
	Test
	*/
	/**
		testPlane1 = Geometry("testPlane1")
		testPlane1.setPrimitive('Plane 0.5 0.5 1 1')
		testPlane2 = Geometry('testPlane2')
		testPlane2.setPrimitive('Plane 1 1 1 1')
		scene.addChild(testPlane1)
		scene.addChild(testPlane2)

		testPlane1.setEuler([0,math.pi*0.5,Angle1])
		testPlane2.setEuler([0,math.pi*0.5,Angle2])
		testPlane1.setFrom(SP)
		testPlane2.setFrom(SP)
	*/
	/**
		AngleMax = math.pi*2-math.acos(u.dot(nT)/(nT.length()*u.length()))
		u_flip = Vec3d([u[0],-u[1],u[2]])
		AngleMin = -math.acos(u_flip.dot(nT)/(nT.length()*u_flip.length()))
	*/
	/**
		if (MateEntityA == '0' && MateEntityB == '0')  {
			if Type == '5':
				DoF = {3,4,5]
				t = Transform('joint')
				t.setPose::create(XyzB,[0,0,-1],[0,1,0])
				C = Pose::create(XyzA,[0,0,-1],[0,1,0])
				C2 = Pose::create(XyzB,[0,0,-1],[0,1,0])
				C_AandT = buildC_AandB(C,a,t)
				C_TandB = buildC_AandB(C2,t,b)

				Joint = 'extra'

		if Joint == 'extra':
			setupJoint(a,t,DoF,C_AandT,MinMax)
			setupJoint(t,b,DoF,C_TandB,MinMax)
			//C = Pose::create(XyzA,[0,0,-1],[0,1,0])
			//C_AandB = buildC_AandB(C,a,b)
			TypeC = '0'
			//Joint = check()
			t = Transform('joint')
	*/
	/**
		'0':  //Point
		'1':  //Line
		'2':  //Circle
		'3':  //Plane
		'4':  //Cylinder
		'5':  //Sphere
		'6':  //Set
		'7':  //Cone
		'8':  //SweptSurface
		'9':  //MultipleSurface
		'10': //GenSurface
		'11': //Ellipse
		'12': //GeneralCurve
		'13': //UNKNOWN
	*/
}

void VRScenegraphInterface::handle(string msg) {
	/** protocol

	action|option|object|data

	examples:
	'clear'
	'new|type|obj|parent'
	'set|transform|obj|1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1'
	'set|positions|obj|x y z x y z x y z x y z ...'
	'set|normals|obj|x y z x y z x y z x y z ...'
	'set|indices|obj|x y z x y z x y z x y z ...'

	"*/

	auto m = splitString(msg, '|');
	if (m.size() == 0) return;

	auto toMatrix = [](const vector<double>& d, int offset = 0) {
		Matrix4d m;
		int o = offset;
		if (d.size() > 11) m = Matrix4d(d[o+0], d[o+3], d[o+6], d[o+9], d[o+1], d[o+4], d[o+7], d[o+10], d[o+2], d[o+5], d[o+8], d[o+11], 0,0,0,1);
		return m;
	};

	if (m.size() > 1)   cout << "receive data, cmd: " << m[0] << " " << m[1] << endl;
	else                cout << "receive data, cmd: " << m[0] << endl;

	if (m[0] == "set" && m.size() > 2) {
		string objID = m[2];
		VRGeometryPtr geo;
		VRTransformPtr trans;
		VRObjectPtr obj;
		if (meshes.count(objID)) geo = meshes[objID];
		if (transforms.count(objID)) trans = transforms[objID];
		if (objects.count(objID)) obj = objects[objID];

		if (m[1] == "transform") {
			if (trans && m.size() > 3) {
                replace( m[3].begin(), m[3].end(), ',', '.');
                vector<double> M = parseVec<double>(m[3]);
                Matrix4d M2 = toMatrix(M, 0);
                trans->setWorldMatrix(M2);
			}
		}

		if (m[1] == "positions") {
			//cout << "set geo positions " << obj << " " << trans << " " << geo << endl;
            if (geo && m.size() > 3) {
                GeoPnt3fPropertyMTRecPtr pos = GeoPnt3fProperty::create();
                replace( m[3].begin(), m[3].end(), ',', '.');
                parseOSGVec2<float, Pnt3f, double>(m[3], pos);
                geo->setPositions(pos);
                //cout << "set geo positions " << geo->getName() << "  " << pos->size() << endl;
            }
		}

		if (m[1] == "normals") {
            if (geo && m.size() > 3) {
                GeoVec3fPropertyMTRecPtr norms = GeoVec3fProperty::create();
                replace( m[3].begin(), m[3].end(), ',', '.');
                parseOSGVec2<float, Vec3f, double>(m[3], norms);
                geo->setNormals(norms);
                //cout << "set geo normals " << geo->getName() << "  " << norms->size() << "  " << m[3].size() << endl;
            }
		}

		if (m[1] == "colors") {
            if (geo && m.size() > 3) {
                GeoColor4fPropertyMTRecPtr cols = GeoColor4fProperty::create();
                replace( m[3].begin(), m[3].end(), ',', '.');
                parseOSGVec2<float, Color4f, double>(m[3], cols);
                geo->setColors(cols);
                //cout << "set geo colors " << geo->getName() << "  " << cols->size() << endl;
            }
		}

		if (m[1] == "indices") {
            if (geo && m.size() > 3) {
                //cout << "set geo indices " << geo->getName() << endl;
                GeoUInt8PropertyMTRecPtr types = GeoUInt8Property::create();
                GeoUInt32PropertyMTRecPtr lengths = GeoUInt32Property::create();
                GeoUInt32PropertyMTRecPtr indices = GeoUInt32Property::create();
                parseOSGVec<int>(m[3], indices);
                types->addValue(GL_TRIANGLES);
                lengths->addValue(indices->size());
                geo->setTypes(types);
                geo->setLengths(lengths);
                geo->setIndices(indices);
                geo->setMeshVisibility(0);

				VRGeoData tester(geo);
				if (tester.valid() && tester.validIndices()) geo->setMeshVisibility(1);
            }
		}

		if (m[1] == "material" && m.size() > 3) {
			string mat = m[3];
			if (mat == "") mat = "__default__";
            if (geo && materials.count(mat)) {
                geo->setMaterial( materials[mat] );
                //cout << "set material " << mat << " to " << geo->getName() << endl;
            }
		}

		if (m[1] == "visible") {
            bool v = toInt(m[3]);
		    //if (!v) cout << " HIDE " << objID << " " << trans << " " << obj << endl;
			if (obj) {
                obj->setVisible(v);
                obj->addAttachment("CAD_visibility", v);
            }
		}

		if (m[1] == "Material") {
            if (objID == "") objID = "__default__";
            if (materials.count(objID) && m.size() > 3) {
                // format: [red, green, blue, ambient, diffuse, specular, shininess, transparency, emission]
                replace( m[3].begin(), m[3].end(), ',', '.');
                auto matData = parseVec<float>(m[3]);
                Color3f rgb = Color3f(1,0,1);
                Color3f ads = Color3f(1,1,1);
                if (matData.size() > 2) rgb = Color3f(matData[0], matData[1], matData[2]); // r,g,b = mat[:3]
                if (matData.size() > 5) ads = Color3f(matData[3], matData[4], matData[5]); // a,d,s = mat[3:6]
                //print obj, mat
                //materials[obj].setAmbient([r*a,g*a,b*a])
                materials[objID]->setDiffuse(rgb * ads[1]);
                if (matData.size() > 9 && matData[9] > 0.5) {
                    materials[objID]->ignoreMeshColors(true);
                }
                //materials[obj].setSpecular([r*s,g*s,b*s])

                // TODO: very prone to artifacts!
                if (handleTransparency) {
                    if (matData.size() > 7 && matData[7] > 0) {
                        cout << "VRScenegraphInterface::handle alpha " << 1-matData[7] << " of " << objID << endl;
                        materials[objID]->setTransparency(1-matData[7]);
                    }
                    if (matData.size() > 10 && matData[10] > 0.5) {
                        cout << "VRScenegraphInterface::handle enableTransparency " << matData[10] << " of " << objID << endl;
                        materials[objID]->enableTransparency();
                    }
                }
            }
		}

		if (m[1] == "kinematic") {
			buildKinematics(m);
		}
	}

	if (m[0] == "clear") {
		cout << "clear scene" << endl;
		clear();
	}

	if (m[0] == "new") {
		string objName = m[2];
		string objID = "NOID";
		if (m.size() > 3) objID = m[3];
		//if (!startsWith(obj, "SU_TIRE")) return;

		VRObjectPtr o;
		if (m[1] == "Object") o = VRObject::create(objName);

		if (m[1] == "Transform") {
            auto t = VRTransform::create(objName);
            cout << "create Transform: " << objID << endl;
            transforms[objID] = t;
            o = t;
        }

		if (m[1] == "Geometry") {
            VRGeometryPtr g = 0;
            if (meshes.count(objName)) {
                g = dynamic_pointer_cast<VRGeometry>(meshes[objName]->duplicate(false, false));
            } else {
                g = VRGeometry::create(objName);
                g->setMeshVisibility(0);
                meshes[objName] = g;
            }
            cout << "create Geometry: " << objID << endl;
            transforms[objID] = g;
            o = g;
        }

		if (m[1] == "Material") {
			if (objName == "") objName = "__default__";
			materials[objName] = VRMaterial::create(objName);
			return;
		}

		if (!o) { cout << "bad type:" << m[1] << endl; return; }

		objects[objID] = o;
		objectIDs[o.get()] = objID;

		VRObjectPtr p;
		if (m.size() > 4) {
            if (objects.count(m[4])) p = objects[m[4]];
		}
		if (!p) p = ptr();
		p->addChild(o);
		cout << "created new object:" << objName << " with ID " << objID << endl;
	}

	for (auto handler : customHandlers) (*handler)(msg); // TODO: pre and post handlers!
}





