#include "VRScenegraphInterface.h"
#include "core/networking/VRSocket.h"
#include "core/scene/VRSceneManager.h"
#include "core/networking/VRNetworkManager.h"
#include "core/utils/VRFunction.h"
#include "core/utils/toString.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/tools/selection/VRSelector.h"

#include <boost/bind.hpp>
#include <OpenSG/OSGMatrix.h>
#include <OpenSG/OSGColor.h>
#include <OpenSG/OSGGeoProperties.h>

using namespace OSG;

VRScenegraphInterface::VRScenegraphInterface(string name) : VRObject(name) {
    resetWebsocket();
}

VRScenegraphInterface::~VRScenegraphInterface() {}

VRScenegraphInterfacePtr VRScenegraphInterface::ptr() { return static_pointer_cast<VRScenegraphInterface>( shared_from_this() ); }
VRScenegraphInterfacePtr VRScenegraphInterface::create(string name)  { return VRScenegraphInterfacePtr( new VRScenegraphInterface(name) ); }


void VRScenegraphInterface::clear() {
    clearChildren();
    materials.clear();
	meshes.clear();
	transforms.clear();
	//CADVRselection.clear();
}

void VRScenegraphInterface::setPort(int p) { if (p == port) return; port = p; resetWebsocket(); }

void VRScenegraphInterface::send(string msg) { socket->answerWebSocket(clientID, msg); }

void VRScenegraphInterface::resetWebsocket() {
    if (socket) VRSceneManager::get()->remSocket(socket->getName());
    socket = VRSceneManager::get()->getSocket(port);
    cb = new VRHTTP_cb( "scenegraph interface callback", boost::bind(&VRScenegraphInterface::ws_callback, this, _1) );
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

template<class T, class U, typename V>
void parseOSGVec2(string& data, V& v) {
    stringstream ss(data);
    T f;
    U u;
    int i=0;
    while (ss >> f) {
        u[i] = f;
        i++;
        if (i == 3) {
            i = 0;
            v->addValue(u);
        }
    }
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

	for (auto handler : customHandlers) (*handler)(msg);

	auto m = splitString(msg, '|');
	if (m.size() == 0) return;

	auto toMatrix = [](const vector<double>& d) {
		Matrix4d m;
		if (d.size() > 11) m = Matrix4d(d[0], d[3], d[6], d[9], d[1], d[4], d[7], d[10], d[2], d[5], d[8], d[11], 0,0,0,1);
		return m;
	};

	if (m[0] == "set" && m.size() > 2) {
		string name = m[2];
		VRGeometryPtr geo;
		VRTransformPtr trans;
		if (meshes.count(name)) geo = meshes[name];
		if (transforms.count(name)) trans = meshes[name];

		if (m[1] == "transform") {
			if (trans && m.size() > 3) {
                replace( m[3].begin(), m[3].end(), ',', '.');
                trans->setWorldMatrix(toMatrix(parseVec<double>(m[3])));
			}
		}

		if (m[1] == "positions") {
            if (geo && m.size() > 3) {
                GeoPnt3fPropertyMTRecPtr pos = GeoPnt3fProperty::create();
                replace( m[3].begin(), m[3].end(), ',', '.');
                parseOSGVec2<float, Pnt3f>(m[3], pos);
                geo->setPositions(pos);
                cout << "set geo positions " << geo->getName() << "  " << pos->size() << endl;
            }
		}

		if (m[1] == "normals") {
            if (geo && m.size() > 3) {
                GeoVec3fPropertyMTRecPtr norms = GeoVec3fProperty::create();
                replace( m[3].begin(), m[3].end(), ',', '.');
                parseOSGVec2<float, Vec3f>(m[3], norms);
                geo->setNormals(norms);
                cout << "set geo normals " << geo->getName() << "  " << norms->size() << endl;
            }
		}

		if (m[1] == "indices") {
            if (geo && m.size() > 3) {
                cout << "set geo indices " << geo->getName() << endl;
                GeoUInt8PropertyMTRecPtr types = GeoUInt8Property::create();;
                GeoUInt32PropertyMTRecPtr lengths = GeoUInt32Property::create();;
                GeoUInt32PropertyMTRecPtr indices = GeoUInt32Property::create();
                parseOSGVec<int>(m[3], indices);
                types->addValue(GL_TRIANGLES);
                lengths->addValue(indices->size());
                geo->setTypes(types);
                geo->setLengths(lengths);
                geo->setIndices(indices);
                geo->setMeshVisibility(1);
            }
		}

		if (m[1] == "material" && m.size() > 3) {
			string mat = m[3];
			if (mat == "") mat = "__default__";
            if (geo && materials.count(mat)) {
                geo->setMaterial( materials[mat] );
                cout << "set material " << mat << " to " << geo->getName() << endl;
            }
		}

		if (m[1] == "visible") {
			if (trans) trans->setVisible(toInt(m[3]));
		}

		if (m[1] == "Material") {
            if (name == "") name = "__default__";
            if (materials.count(name) && m.size() > 3) {
                // format: [red, green, blue, ambient, diffuse, specular, shininess, transparency, emission]
                replace( m[3].begin(), m[3].end(), ',', '.');
                auto matData = parseVec<float>(m[3]);
                Color3f rgb = Color3f(1,0,1);
                Color3f ads = Color3f(1,1,1);
                if (matData.size() > 2) rgb = Color3f(matData[0], matData[1], matData[2]); // r,g,b = mat[:3]
                if (matData.size() > 5) ads = Color3f(matData[3], matData[4], matData[5]); // a,d,s = mat[3:6]
                //print obj, mat
                //materials[obj].setAmbient([r*a,g*a,b*a])
                materials[name]->setDiffuse(rgb * ads[1]);
                //materials[obj].setSpecular([r*s,g*s,b*s])
                //materials[obj].setTransparency(1-mat[7])
            }
		}

		if (m[1] == "kinematic") {
			// TODO
			//VR.buildKinematics(obj1, obj2, params)
			//print "hi"
		}
	}

	if (m[0] == "clear") {
		cout << "clear scene" << endl;
		clear();
	}

	if (m[0] == "new") {
		string obj = m[2];

		VRObjectPtr o;
		if (m[1] == "Object") o = VRObject::create(obj);

		if (m[1] == "Transform") {
            auto t = VRTransform::create(obj);
            transforms[obj] = t;
            o = t;
        }

		if (m[1] == "Geometry") {
            VRGeometryPtr g = 0;
            if (meshes.count(obj)) {
                g = dynamic_pointer_cast<VRGeometry>(meshes[obj]->duplicate());
            } else {
                g = VRGeometry::create(obj);
                g->setMeshVisibility(0);
                transforms[obj] = g;
                meshes[obj] = g;
            }
            o = g;
        }

		if (m[1] == "Material") {
			if (obj == "") obj = "__default__";
			materials[obj] = VRMaterial::create(obj);
		}

		objects[obj] = o;

		if (!o) { cout << "bad type:" << m[1] << endl; return; }

		VRObjectPtr p;
		if (m.size() > 3) {
            if (objects.count(m[3])) p = objects[m[3]];
		}
		if (!p) p = ptr();
		p->addChild(o);
		cout << "created new object:" << m[2] << endl;
	}
}





