#include "VRTechnicalDrawing.h"
#include "core/objects/geometry/VRGeoData.h"
#include "core/objects/geometry/VRGeometry.h"
#include "core/objects/material/VRMaterial.h"
#include "core/tools/VRAnnotationEngine.h"
#include "core/utils/toString.h"

using namespace OSG;



VRTechnicalDrawing::Object::Object(string name, ObjectType type, string style) : name(name), type(type), style(style) {
    static size_t i = 0; i++;
    ID = i;
}

VRTechnicalDrawing::Parameter::Parameter(ParameterType type, void* src, size_t nBytes) : type(type) {
    data = string(reinterpret_cast<char*>(src), nBytes);
}

VRTechnicalDrawing::Layer::Layer(string name) : name(name) {
    root = VRTransform::create(name);
}

VRTechnicalDrawing::VRTechnicalDrawing() : VRTransform("drawing") {}
VRTechnicalDrawing::~VRTechnicalDrawing() {}

VRTechnicalDrawingPtr VRTechnicalDrawing::create() { return VRTechnicalDrawingPtr( new VRTechnicalDrawing() ); }
VRTechnicalDrawingPtr VRTechnicalDrawing::ptr() { return static_pointer_cast<VRTechnicalDrawing>(shared_from_this()); }

template<typename T> VRTechnicalDrawing::Parameter VRTechnicalDrawing::packParam(ParameterType type, T data) { return Parameter(type, &data, sizeof(data)); }
template<typename T> VRTechnicalDrawing::Parameter VRTechnicalDrawing::packParam(ParameterType type, vector<T> data) { return Parameter(type, &data[0], sizeof(data[0])*data.size()); }
template<> VRTechnicalDrawing::Parameter VRTechnicalDrawing::packParam(ParameterType type, string data) { Parameter p; p.type = type; p.data = data; return p; }
template<typename T> void VRTechnicalDrawing::unpackParam(Parameter p, T& t) { t = *(T*)&p.data[0]; }
template<> void VRTechnicalDrawing::unpackParam(Parameter p, string& t) { t = p.data; }

template<typename T> void VRTechnicalDrawing::unpackParam(Parameter p, vector<T>& t) {
    size_t N = p.data.size() / sizeof(T);
    t = vector<T>(N);
    if (N > 0) memcpy( &t[0], &p.data[0], N*sizeof(T) );
}

void VRTechnicalDrawing::setActiveLayer(string layer) { context.layer = layer; }
void VRTechnicalDrawing::setActiveTransform(Matrix4d transform) { context.transform = transform; }

void VRTechnicalDrawing::addLayer(string name) {
    layers[name] = Layer(name);
    addChild( layers[name].root );
}

bool VRTechnicalDrawing::hasMaterial(string mID) { return materials.count(mID); }
VRTechnicalDrawing::Material VRTechnicalDrawing::getMaterial(string mID) { return materials[mID]; }
void VRTechnicalDrawing::addMaterial(string mID) { materials[mID] = Material(); }
void VRTechnicalDrawing::setColor(string mID, Color3f c) { materials[mID].color = c; }

void VRTechnicalDrawing::addMarkup(string name, VRAnnotationEnginePtr style, string layer) { layers[layer].annotationStyles[name] = style; }

VRTechnicalDrawing::Object& VRTechnicalDrawing::addObject(string name, ObjectType type, string style) {
    Object o(name, type, style);
    o.transform = context.transform;
    if (!layers.count( context.layer) ) { cout << "Error in addObject, layer " << context.layer << " not found" << endl; }
    auto& layer = layers[context.layer];
    layer.objects[o.ID] = o;
    layer.changed = true;
    return layer.objects[o.ID];
}

void VRTechnicalDrawing::addPoint(string name, Pnt2d p, string style) {
    Object& o = addObject(name, POINT, style);
    o.parameters[POSITION] = packParam(POSITION, p);
}

void VRTechnicalDrawing::addLine(string name, Pnt2d p1, Pnt2d p2, string style) {
    Object& o = addObject(name, LINE, style);
    vector<Pnt2d> vIn({p1,p2});
    o.parameters[POSITIONS] = packParam(POSITIONS, vIn);
}


void VRTechnicalDrawing::addQuad(string name, Pnt2d p1, Pnt2d p2, Pnt2d p3, Pnt2d p4, string style) {
    Object& o = addObject(name, QUAD, style);
    vector<Pnt2d> vIn({p1,p2,p3,p4});
    o.parameters[POSITIONS] = packParam(POSITIONS, vIn);
}

void VRTechnicalDrawing::addArc(string name, Pnt2d c, double r, double b, double e, Vec2d s, string style) {
    Object& o = addObject(name, ARC, style);
    o.parameters[CENTER] = packParam(CENTER, c);
    o.parameters[RADIUS] = packParam(RADIUS, r);
    o.parameters[BEGIN] = packParam(BEGIN, b);
    o.parameters[END] = packParam(END, e);
    o.parameters[SCALE] = packParam(SCALE, s);
}

void VRTechnicalDrawing::addCircle(string name, Pnt2d cp, double r, string style) {}
void VRTechnicalDrawing::addEllipse(string name, Pnt2d cp, double a, double b, string style) {}
void VRTechnicalDrawing::addPolyLine(string name, vector<Pnt2d> v, string style) {}

void VRTechnicalDrawing::addLabel(string name, Pnt2d p, string text, string style) {
    Object& o = addObject(name, LABEL, style);
    o.parameters[POSITION] = packParam(POSITION, p);
    o.parameters[TEXT] = packParam(TEXT, text);
}

void VRTechnicalDrawing::updateGeometries() {
    mat = VRMaterial::create("wire");
    mat->setLit(false);
    mat->setPointSize(6);

    for (auto& l : layers) {
        if (!l.second.changed) continue;
        for (auto& o : l.second.objects) {
            if (!o.second.changed) continue;
            updateGeometry(l.second, o.second);
            o.second.changed = false;
        }
        l.second.changed = false;
    }
}

void VRTechnicalDrawing::updateGeometry(Layer& l, Object& o) {
    if (!l.root) { cout << "Error in updateGeometry, layer without root object! " << l.name << endl; return; }
    if (!l.geoAggregators.count(o.style)) l.geoAggregators[o.style] = VRGeoData::create();
    VRGeoDataPtr geo = l.geoAggregators[o.style];
    Color3f col = materials[o.style].color;

    if (o.type == LINE) {
        vector<Pnt2d> v;
        unpackParam( o.parameters[POSITIONS], v);

        Pnt3d p1 = Pnt3d( v[0] );
        Pnt3d p2 = Pnt3d( v[1] );
		p1 = o.transform * p1;
		p2 = o.transform * p2;
        geo->pushVert(p1);
        geo->pushVert(p2);
        geo->pushColor(col);
        geo->pushColor(col);
        geo->pushLine();
    } else if (o.type == QUAD) {
        vector<Pnt2d> v;
        unpackParam( o.parameters[POSITIONS], v);
        if (v.size() != 4) { cout << "ERROR: Quad needs 4 vertices, got " << v.size() << endl; return; }

        Pnt3d p1 = Pnt3d( v[0] );
        Pnt3d p2 = Pnt3d( v[1] );
        Pnt3d p3 = Pnt3d( v[2] );
        Pnt3d p4 = Pnt3d( v[3] );
		p1 = o.transform * p1;
		p2 = o.transform * p2;
		p3 = o.transform * p3;
		p4 = o.transform * p4;
        geo->pushVert(p1);
        geo->pushVert(p2);
        geo->pushVert(p3);
        geo->pushVert(p4);
        geo->pushColor(col);
        geo->pushColor(col);
        geo->pushColor(col);
        geo->pushColor(col);
        geo->pushQuad();
    } else if (o.type == ARC) {
        Pnt2d c;
        Vec2d s;
        double r, a1, a2;
        unpackParam( o.parameters[CENTER], c);
        unpackParam( o.parameters[RADIUS], r);
        unpackParam( o.parameters[BEGIN], a1);
        unpackParam( o.parameters[END],   a2);
        unpackParam( o.parameters[SCALE],  s);

        Pnt3d C = o.transform * Pnt3d(c);

        double da = 0.1;
        int N = max(int(abs(a2-a1)/da),1);
        da = (a2-a1)/N;
        for (int i=0; i<=N; i++) {
            double a = a1+da*i;
            Pnt3d p = C + Vec3d(cos(a)*s[0], sin(a)*s[1], 0)*r;
            geo->pushVert(p);
            geo->pushColor(col);
            if (i > 0) geo->pushLine();
        }
    } else if (o.type == POINT) {
        Pnt2d p;
        unpackParam( o.parameters[POSITION], p);
        Pnt3d P(p);
		P = o.transform * P;
        geo->pushVert(P);
        geo->pushColor(col);
        geo->pushPoint();
    } else if (o.type == LABEL) {
        Pnt2d p;
        unpackParam( o.parameters[POSITION], p);
        Pnt3d P(p);
		P = o.transform * P;
		string t;
        unpackParam( o.parameters[TEXT], t);

        if (!l.annotationStyles.count(o.style)) {
            auto a = VRAnnotationEngine::create("labels");
            auto v = splitString(o.style, '_');
            double h = toDouble(v[1]);
            Vec3d xDir;
            toValue(v[2], xDir);
            a->setSize(h);
            a->setUp( xDir.cross(Vec3d(0,0,-1)) );
            l.annotationStyles[o.style] = a;
            l.root->addChild( a );
        }

		auto a = l.annotationStyles[o.style];
		a->add(Vec3d(P), t);
    } else {
        cout << "Warning! type " << o.type << " not handled!" << endl;
    }

    if (!geo->size()) return;
    if (!l.geometries.count(o.style)) {
        auto g = geo->asGeometry(l.name);
        g->setMaterial(mat);
        l.geometries[o.style] = g;
        l.root->addChild( g );
    }
    geo->apply( l.geometries[o.style] );
}

