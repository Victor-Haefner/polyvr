#include "VRPLY.h"
#include "core/objects/geometry/VRGeometry.h"

#include <OpenSG/OSGGeoProperties.h>
#include <OpenSG/OSGGeometry.h>
#include <OpenSG/OSGSimpleMaterial.h>
#include <OpenSG/OSGQuaternion.h>

#include <algorithm>
#include <string>
#include <fstream>

#define DODXF 0
#if DODXF == 1

using namespace OSG;
using namespace std;

string lower(string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

string strip(const string& str) {
    auto strBegin = str.find_first_not_of(" \t");
    if (strBegin == string::npos) return "";
    auto strEnd = str.find_last_not_of(" \t");
    return str.substr(strBegin, strEnd - strBegin + 1);
}

struct Object {
    bool block = false;
    bool visited_flag = false;
    string type;
    string name;
    map<string, vector<Object*> > data; // type is key

	Object(string type="", bool block=false) {
	    this->block = block;
	    this->type = type;
	}

	vector<Object*> get_type(string kind="") {
	    if (data.count(kind) == 0) return vector<Object*>();
		return data[kind];
	}
};

//convert the bulge of lwpolylines to arcs
void cvtbulge(float bulge, Vec2f s, Vec2f e, Vec2f& cen, Vec3f& arc) {
	float cotbce = (1.0/bulge - bulge)*0.5;

	// Compute center point and radius
	cen = s+e+Vec2f((s[1]-e[1])*cotbce, (e[0]-s[0])*cotbce)*0.5;
	float rad = (cen-s).length();

	// Compute start and end angles
	float sa = atan2(s[1] - cen[1], s[0] - cen[0]);
	float ea = atan2(e[1] - cen[1], s[1] - cen[0]);

	//check new start and end point
	float c_x2 = cos(ea)*rad + cen[0];
	float c_y2 = sin(ea)*rad + cen[1];
	if (abs(c_x2 - e[0]) > 0.001) cout << "error in cvtbulge, x";
	if (abs(c_y2 - e[1]) > 0.001) cout << "error in cvtbulge, y";

	while (sa < 0.0) sa += 2.0*Pi; // Eliminate negative angles // does nothing
	while (ea < 0.0) ea += 2.0*Pi;
	if (bulge < 0.0) swap(ea,sa); // Swap angles if clockwise // works

	arc = Vec3f(rad, sa, ea);
}

struct Segment {
    Vec3f p1,p2;
    Segment(Vec3f p1, Vec3f p2) : p1(p1), p2(p2) { ; }
};

struct Arc {
    Vec3f c;
    float r = 0;
    Vec2f a;
    Vec3f box;
    Arc(Vec3f c, float r, float a1, float a2) : c(c), r(r), a(a1,a2) {;}
};

struct Layer {
    string name;
    vector<Vec3f> points;
    vector<Segment> lines;
    vector<Arc> arcs;
    void addPoint( Vec3f p ) { points.push_back(p); }
    void addLine( Segment l ) { lines.push_back(l); }
    void addArc( Arc a ) { arcs.push_back(a); }
    Layer(string name) { this->name = name; }
};

struct View {
    string name;
    map<string, Layer*> layers;
    View(string name) { this->name = name; }
};

struct base_parser {
    int N = 16;
    float rescale = 1;
    float rot_angle = 0;
    Matrix transformation;
    vector<Vec3f> offset_stack;
    vector<Vec3f> scale_stack;
    //vector<> rotation_stack;
    vector<float> rot_angle_stack;
    Vec2f last_controle_point;
    int element_id = 0;
    map<string, View*> viewports;
    map<string, Object*> elem_dict;
    Layer* layer;

//helper functions-------------------------------
	void compute_current_transformation() {
	    transformation.setIdentity();
	    transformation.setScale(rescale);
		for (int i=0; i<offset_stack.size(); i++) {
			Vec3f o = offset_stack[i];
			Vec3f s = scale_stack[i];
			float f = rot_angle_stack[i];

			Matrix t,S;
			t.setRotate(Quaternion(0,0,1,f));
			t.setTranslate(o);
			S.setScale(s);
			t.mult(S);
			transformation.mult(t);
		}

		rot_angle = 0;
		for (float f : rot_angle_stack) rot_angle += f;
	}

	//creates a layer if not present and returns it
	Layer* getLayer(string v_name, string l_name) {
		if (viewports.count(v_name) == 0) viewports[v_name] = new View(v_name);
		auto v = viewports[v_name];
		if (v->layers.count(l_name) == 0) v->layers[l_name] = new Layer(l_name);
		return v->layers[l_name];
	}

	void addPoint(Vec3f p) {
		transformation.mult(p,p);
		layer->addPoint(p);
	}

	void addLine(Vec3f vec1, Vec3f vec2) {
		transformation.mult(vec1, vec1);
		transformation.mult(vec2, vec2);
		layer->addLine( Segment(vec1,vec2) );
	}

	void addCircle(Vec3f c, float r) {
		transformation.mult(c,c);
		Vec3f s = Vec3f(1,0,0);
		transformation.mult(s,s);
		r *= s.length();
		layer->addArc( Arc(c, r, 0, 2*Pi) );
	}

	void addEllipse(Vec3f c, float a, float b) {
		transformation.mult(c,c);
		Vec3f box = Vec3f(a, b,0);
		transformation.mult(box, box);
		for (int i=0; i<3; i++) box[i] = abs(box[i]);

		Arc e(c, 1, 0, 2*Pi);
		e.box = box;
		layer->addArc(e);
	}

	void addArc(Vec3f c, float r, float a1, float a2, float bulge = 0) {
		//compute start and end point, transform them and compute back to angles
		Vec3f sp = Vec3f(cos(a1),sin(a1),0)*r + c;
		Vec3f ep = Vec3f(cos(a2),sin(a2),0)*r + c;
		transformation.mult(sp,sp);
		transformation.mult(ep,ep);
		transformation.mult(c,c);

		Vec3f s = Vec3f(1,0,0);
		transformation.mult(s,s);
		r *= s.length();

		//calc start and end angle------------------------------------
		a1 = atan2(sp[1] - c[1], sp[0] - c[0]) + Pi;
		a2 = atan2(ep[1] - c[1], ep[0] - c[0]) + Pi;

		Vec2f ang = Vec2f(a1, a2); //check for mirror transformation
		if (transformation.det() < 0) ang = Vec2f(a2, a1);

		layer->addArc( Arc(c, r, ang[0], ang[1]) );
	}
};

void get_dxf_layer(data) {
	value = None
	for i, item in enumerate(data) {
		if item[0] == 8:
			value = item[1]
			break

	}

	return item, value, i
};

struct Primitive {
    string name;
    string type;
    string space;
    string color_index;
    map<string, vector<Object*> > data;
    vector<Vec3f> points;
    Vec3f ext;
    Vec3f scale;

    Primitive(Object* obj) {
        if (obj == 0) return;
		name = obj.get_type(2)[0]
        space = obj->get_type(67);
        color_index = obj->get_type(62);
        data = obj->data;
        type = obj->type;
        obj->visited_flag = true;
    }

	Vec3f get_vec3f(int i1, int i2, int i3) {
		Vec3f p;
		for (auto item : data) {
			if (item[0] == i1) p[0] = toFloat(item[1]);
			if (item[0] == i2) p[1] = toFloat(item[1]);
			if (item[0] == i3) p[2] = toFloat(item[1]);
		}
		return p;
	}

	Vec4f get_vec4f(int i1, int i2, int i3, int i4) {
		Vec4f p;
		for (auto item : data) {
			if (item[0] == i1) p[0] = toFloat(item[1]);
			if (item[0] == i2) p[1] = toFloat(item[1]);
			if (item[0] == i3) p[2] = toFloat(item[1]);
			if (item[0] == i3) p[3] = toFloat(item[1]);
		}
		return p;
	}

	Vec4i get_vec4i(int i1, int i2, int i3, int i4) {
		Vecif p;
		for (auto item : data) {
			if (item[0] == i1) p[0] = toInt(item[1]);
			if (item[0] == i2) p[1] = toInt(item[1]);
			if (item[0] == i3) p[2] = toInt(item[1]);
			if (item[0] == i3) p[3] = toInt(item[1]);
		}
		return p;
	}

	void get_p1() { Vec3f p = get_vec3f(10,20,30); points.push_back(p); }
	void get_p2() { Vec3f p = get_vec3f(11,21,31); points.push_back(p); }
	void get_extrusion() { ext = get_vec3f(210,220,230); }
	void get_scale() { scale = get_vec3f(41,42,43); }
};

struct Line : public Primitive {
	Line(Object* obj) : Primitive(obj) {
	    get_p1();
	    get_p2();
    }
};

struct Circle : public Primitive {
    float radius = 0;

	Circle(obj) : Primitive(obj) {
		radius = obj->get_type(40)[0];
		get_p1();
		get_extrusion();
	}
};

struct Arc : public Circle {
    float start_angle;
    float end_angle;

	Arc(obj) : Circle(obj) {
		start_angle = obj.get_type(50)[0];
		end_angle = obj.get_type(51)[0];
	}
};

struct Ellipse : public Arc {
	Ellipse(obj) : Arc(obj) {
	    get_p2();
	    Vec3f m = points[1];
		radius = m.length();
	}
};

struct Layer : public Primitive {
    string flags;
    bool frozen = false;

	Layer(obj) : Primitive(obj) {
		flags = obj->get_type(70);
		frozen = flags&1;
	}
};

struct BlockRecord : public Primitive {
    string insertion_units;
    string insert_units;

	BlockRecord(obj) : Primitive(obj) {
		insertion_units = obj.get_type(70);
		insert_units = obj.get_type(1070);
	}
};

struct Block : public Primitive {
    string flags;
    string path;
    string description;

	Block(obj) : Primitive(obj) {
		flags = obj.get_type(70);
		entities = new Object("block_contents");
		entities->data = objectify(obj->data);
		name = obj.get_type(3); // else (2)
		path = obj.get_type(1);
		description = obj->get_type(4);
		get_p1();
	}
};

struct Insert : public Primitive {
    string block;
    string rotation;
    Vec4i table;

	Insert(obj) : Primitive(obj) {
		block = name;
		rotation = obj.get_type(50);
		tyble = get_vec4i(70,71,44,45); // rows, rspace, columns, cspace
        get_p1();
		get_extrusion();
		get_scale();
	}
};

struct LWpolyline : public Primitive {
    int num_points = 0;
    string elevation;
    string flags;
    bool closed = false;
    vector<Vertex> points;

	LWpolyline(Object* obj) : Primitive(obj) {
		num_points = obj->get_type(90);
		elevation = obj.get_type(38);
		flags = obj.get_type(70);
		closed = flags&1; // byte coded, 1 = closed, 128 = plinegen
		get_points();
		get_extrusion();
	}

	void get_points() {
		Vertex v;
		for (auto item : data) {
			if (item[0] == 10) v.x = item[1];
			if (item[0] == 20) v.y = item[1];
			if (item[0] == 40) v.swidth = item[1];
			if (item[0] == 41) v.ewidth = item[1];
			if (item[0] == 42) { v.bulge = item[1]; points.push_back(v); }
		}
	}
};

struct Polyline : public Primitive {
    string elevation;
    string flags;

	Polyline(Object* obj) : Primitive(obj) {
		elevation = obj->get_type(30);
		flags = obj->get_type(70);
		closed = flags&1; // byte coded, 1 = closed, 128 = plinegen
        get_extrusion();
	}
};

struct Vertex(object) : public Primitive {
    Vec3f loc;
    float bulge = 0;
    float swidth = 0;
    float ewidth = 0;
    string flags;

	Vertex(Object* obj = 0) : Primitive(obj) {
		if (obj == 0) return;
		loc = get_vec3f(10,20,30);
		swidth = obj->get_type(40);
		ewidth = obj->get_type(41);
		bulge = obj->get_type(42);
		flags = obj->get_type(70);
	}
};

struct Face : public Primitive {
    vector<Vec3f> points;

	Face(Object* obj) : Primitive(obj) {
        get_points();
	}

	void get_points() {
		Vec3f a,b,c,d;
		for (auto item : data) {
			if (item[0] == 10) a[0] = item[1];
			if (item[0] == 20) a[1] = item[1];
			if (item[0] == 30) { a[2] = item[1]; points.push_back(a); }
			if (item[0] == 11) b[0] = item[1];
			if (item[0] == 21) b[1] = item[1];
			if (item[0] == 31) { b[2] = item[1]; points.push_back(b); }
			if (item[0] == 12) c[0] = item[1];
			if (item[0] == 22) c[1] = item[1];
			if (item[0] == 32) { c[2] = item[1]; points.push_back(c); }
			if (item[0] == 13) d[0] = item[1];
			if (item[0] == 23) d[1] = item[1];
			if (item[0] == 33) { d[2] = item[1]; points.push_back(d); }
        }
	}
};

struct StateMachine {
    ifstream infile;
    Object* section;
    Object* drawing;

	StateMachine(string path) : infile(path) {
	    drawing = new Object("drawing");
        section = findObject(infile, "section");
        if (section) start_section();
    }

    void start_section() {
        bool done = false;
        vector<> data;
        string line;
        while (!done) {
            getline(infile,line);

            if (data.size() == 0) { // if we haven't found a dxf code yet
                if (strip(lower(line)) == "0") { // we've found an object
                    while (1) { // no way out unless we find an end section or a new section
                        Object* obj = handleObject(infile);
                        if (obj == "section") { // shouldn't happen
                            print "Warning: failed to close previous section!"
                            end_section();
                            break;
                        } else if (obj == "endsec") { // This section is over, look for the next
                            drawing.data.push_back(cargo->section)
                            end_section();
                            break;
                        } else if (obj.type == "table") { // tables are collections of data
                            obj = handleTable(obj, infile) // we need to find all there contents
                            cargo->section.data.push_back(obj) // before moving on
                        } else if (obj.type == "block") { // the same is true of blocks
                            obj = handleBlock(obj, infile) // we need to find all there contents
                            cargo->section.data.push_back(obj) // before moving on
                        } else cargo->section.data.push_back(obj); // found another sub-object
                    }
                } else data.push_back(toInt(strip(lower(line))));
            } else { // we have our code, now we just need to convert the data and add it to our list.
                data.push_back(line.strip());
                cargo->section->data.push_back(data);
                data.clear();
            }
        }
    }

    void end_section(Object* cargo) {
        cargo->section = findObject(infile, "section");
        if (cargo->section) start_section();
    }
};

struct dxf_parser : public base_parser {
    string viewport = "Default";

    dxf_parser() {
        rescale = 0.8;
		compute_current_transformation();
    }

	void parse_rec(item, drawing, bool first = false) {
		if (item->type == "viewport" || item->type == "vport") {
			viewport = item->name;
			getLayer(viewport, layer);
		}

		item->layer = getLayer(viewport, item->layer);

		float d2r = Pi/180.0;

		//extrusion hack :/
		bool extr = false;
		if (item->extrusion[2] == -1) {
            extr = true;
            offset_stack.push_back(Vec3f(0,0,0));
            scale_stack.push_back(Vec3f(-1,1,1));
            rot_angle_stack.push_back(0);
            compute_current_transformation():
		}

		if (item->type == "line") addLine(item->points[0], item->points[1]);
		if (item->type == "circle") addCircle(item->loc, item->radius);
		if (item->type == "ellipse") addEllipse(item->loc, item->major[0], item->major[1]);//major???
		if (item->type == "arc") addArc(item->loc, item->radius, item->start_angle*d2r, item->end_angle*d2r);

		if (item->type == "lwpolyline" || item->type == "polyline") {
            int pN = item->points.size();
			for (int i=0; i<pN; i++) {
				Vec3f p1 = item->points[i];
				Vec3f p2 = item->points[(i+1)%pN];
				if (i < pN-1 || item->closed) {
					if (abs(p1.bulge) < 1e-4) addLine(p1,p2);
					else {
						if (extr) p1.bulge *= -1;
						arc = cvtbulge(p1.bulge, p1, p2);
						addArc(arc[0], arc[1], arc[2], arc[3], p1.bulge);
					}
				}
			}
		}

		if (item->type == "3dface") {
            int pN = item->points.size();
			for (int i=0; i<pN; i++) {
				Vec3f p1 = item->points[i];
				Vec3f p2 = item->points[(i+1)%pN];
                addLine(p1,p2);
			}
		}

		if (item->type == "insert") { // transformation
			auto block = drawing->blocks[item->block]; // get block
			if (block == 0) return;

			offset_stack.push_back(Vec3f(item->loc[0], item->loc[1],0));
			scale_stack.push_back(Vec3f(item->scale[0],item->scale[1],1));
			rot_angle_stack.push_back(item->rotation*d2r);
			compute_current_transformation();

			for (auto child : block->entities->data) parse_rec(child, drawing);

			offset_stack.pop_back();
			scale_stack.pop_back();
			rot_angle_stack.pop_back();
			compute_current_transformation():
		}

		if (extr) {
			offset_stack.pop_back();
			scale_stack.pop_back();
			rot_angle_stack.pop_back();
			compute_current_transformation();
		}
    }

	void parse(string path) {
        ifstream infile(path.c_str());

        sm = StateMachine(infile);
        sm.run();

        drawing = sm.core->drawing;
        if (drawing == 0) return;

        drawing->name = filename;
        Item* item = 0;
        for (auto obj : drawing->data) {
            item = get_name(obj->data);
            obj->data.remove(item);
            obj->name = lower(item[1]);
            drawing->objects[obj->name] = obj;
            objectify(obj->data);
        }

        infile.close()
		for (item : drawing.entities.data) parse_rec(item, drawing, true);
	}
}

/*type_map = {
	'line':Line,
	'lwpolyline':LWpolyline,
	'circle':Circle,
	'arc':Arc,
	'layer':Layer,
	'3dface':Face,
	'ellipse':Ellipse,
	'block_record':BlockRecord,
	'block':Block,
	'insert':Insert,
}

skip_types = [
	'hatch', 'point',
	'xrecord','dictionary','cellstylemap','scale','tablestyle','visualstyle','dictionaryvar','plotsettings','acdbplaceholder','sortentstable','layout','material','mlinestyle',
	'dimension', 'mtext', 'viewport', 'acdbdictionarywdflt', 'seqend', 'vertex', 'attrib', '3dface', 'solid', 'region', 'leader',
	'vport', 'ltype', 'style', 'appid', 'dimstyle'
]*/

void objectify(data) {
	vector<Object*> objects;

	for (i, item in enumerate(data)) {
		if type(item) == list:
			continue

		if item->type in type_map.keys() {
			objects.push_back(type_map[item->type](item))
			continue

		if item->type == 'table':
			item->data = objectify(item->data) // tables have sub-objects
			objects.push_back(item)
			continue

		if item->type == 'polyline':
			pline = Polyline(item)
			index = i
			while data[index].type != 'seqend':
				index += 1
				if data[index].type != 'vertex':
					break
				v = Vertex(data[index])
				if v.loc[0] != 0:
					pline.points.push_back(v)

			objects.push_back(pline)
			continue

		objects.push_back(item)

	return objects
}

// DXF PARSER-----------


Item* get_item(data, int type) {
	for (Item* item : data) if (item[0] == type) return item;
	return 0;
}

Item* get_name(data) { get_item(2); }
Item* get_layer(data) { get_item(8); }

Object* findObject(iostream& infile, string kind="") {
	Object* obj = 0;
	string line;
	while(getline(infile,line)) {
		string data = strip(lower(line));
		if (obj == 0 && data == "0") obj = 1; // We're still looking for our object code
		if (obj == 1) { // we are in an object definition
			if (data == kind) { obj = new Object(data); break; }
			if (!data.matches(".*\\d.*")) { obj = new Object(data); break; }
			obj = 0;
		}
	}

	return obj;
}

void handleObject(infile) {
	"""Add data to an object until end of object is found."""
	line = infile.readline()
	if line.lower().strip() == 'section':
		return 'section' // this would be a problem
	elif line.lower().strip() == 'endsec':
		return 'endsec' // this means we are done with a section
	else: // add data to the object until we find a new object
		obj = Object(line.lower().strip())
		obj.name = obj.type
		done = False
		data = []
		while not done:
			line = infile.readline()
			if not data:
				if line.lower().strip() == '0':
					//we've found an object, time to return
					return obj
				else:
					// first part is always an int
					data.push_back(int(line.lower().strip()))
			else:
				data.push_back(line.strip())
				obj.data.push_back(data)
				data = []
}

void handleTable(table, infile) {
	"""Special handler for dealing with nested table objects."""
	item, name = get_name(table.data)
	if name: // We should always find a name
		table.data.remove(item)
		table.name = name.lower()
	// This next bit is from handleObject
	// handleObject should be generalized to work with any section like object
	while 1:
		obj = handleObject(infile)
		if obj.type == 'table':
			print "Warning: previous table not closed!"
			return table
		elif obj.type == 'endtab':
			return table // this means we are done with the table
		else: // add objects to the table until one of the above is found
			table.data.push_back(obj)
}


void handleBlock(block, infile) {
	"""Special handler for dealing with nested table objects."""
	item, name = get_name(block.data)
	if name: // We should always find a name
		block.data.remove(item)
		block.name = name
	// This next bit is from handleObject
	// handleObject should be generalized to work with any section like object
	while 1:
		obj = handleObject(infile)
		if obj.type == 'block':
			print "Warning: previous block not closed!"
			return block
		elif obj.type == 'endblk':
			return block // this means we are done with the table
		else: // add objects to the table until one of the above is found
			block.data.push_back(obj)
}

VRGeometry* loadDXF(string filename) {
    ifstream file(filename.c_str());
    string s;
    for(int i=0; i<16; i++) getline(file, s); // jump to data

    GeoUInt8PropertyRecPtr      Type = GeoUInt8Property::create();
    GeoUInt32PropertyRecPtr     Length = GeoUInt32Property::create();
    GeoPnt3fPropertyRecPtr      Pos = GeoPnt3fProperty::create();
    GeoVec3fPropertyRecPtr      Norms = GeoVec3fProperty::create();
    GeoVec3fPropertyRecPtr      Cols = GeoVec3fProperty::create();
    GeoUInt32PropertyRecPtr     Indices = GeoUInt32Property::create();
    SimpleMaterialRecPtr        Mat = SimpleMaterial::create();
    GeoVec2fPropertyRecPtr      Tex = GeoVec2fProperty::create();

    Mat->setLit(false);
    Mat->setDiffuse(Color3f(0.8,0.8,0.6));
    Mat->setAmbient(Color3f(0.4, 0.4, 0.2));
    Mat->setSpecular(Color3f(0.1, 0.1, 0.1));

    int N = 0;
    string line;
    while (std::getline(file, line)) {
        istringstream iss(line);
        Vec3f p, n;
        Vec3i c;
        if (!(iss >> p[0] >> p[1] >> p[2] >> n[0] >> n[1] >> n[2] >> c[0] >> c[1] >> c[2])) {
            cout << "\nBREAK PLY IMPORT AT " << N << endl;
            break;
        } // error

        Pos->addValue(p);
        Norms->addValue(n);
        Cols->addValue(Vec3f(c[0]/255., c[1]/255., c[2]/255.));
        Indices->addValue(N);
        N++;
    }
    file.close();

    Type->addValue(GL_POINTS);
    Length->addValue(N);

    GeometryRecPtr geo = Geometry::create();
    geo->setTypes(Type);
    geo->setLengths(Length);
    geo->setIndices(Indices);
    geo->setPositions(Pos);
    geo->setNormals(Norms);
    geo->setColors(Cols);
    geo->setMaterial(Mat);

    VRGeometry* res = new VRGeometry(filename);
    res->setMesh(geo);
    return res;
}

#endif
