#include "VRMachiningCode.h"
#include "core/utils/toString.h"
#include "core/objects/material/VRMaterial.h"

#include <OpenSG/OSGMatrix.h>

using namespace OSG;

VRMachiningCode::VRMachiningCode() {}
VRMachiningCode::~VRMachiningCode() {}

VRMachiningCodePtr VRMachiningCode::create() { return VRMachiningCodePtr( new VRMachiningCode() ); }
VRMachiningCodePtr VRMachiningCode::ptr() { return static_pointer_cast<VRMachiningCode>(shared_from_this()); }

void VRMachiningCode::reset() { pointer = 0; }
size_t VRMachiningCode::length() { return instructions.size() - pointer; }

void VRMachiningCode::clear() {
    instructions.clear();
    reset();
}

VRMachiningCode::Instruction VRMachiningCode::next() {
	if (pointer <= instructions.size()) return instructions[pointer++];
	return Instruction();
}

void VRMachiningCode::translate(Vec3d vec_0, Vec3d vec_1, double v) { // G01 translation
	vec_0 *= 0.001;
	vec_1 *= 0.001;
	Vec3d vecf = vec_1 - vec_0;

	double L = vecf.length();
	double T = L / v * 1000;
	if (T * (skippedSteps + 1) > 1.0 / 60 || true) {
		Instruction i;
		i.G = 0;
		i.d = vecf;
		i.p0 = vec_0;
		i.T = T;
		instructions.push_back(i);
		skippedSteps = 0;
	} else skippedSteps += 1;
}

void VRMachiningCode::rotate(Vec3d start, Vec3d end, Vec3d center, Vec3d axis, double v_0, int mode) { // G02 and G03 rotation
	double a = 0;
	if (mode == 1) a = 1;
	else a = -1;

	// vector
	Vec3d r0 = start - center;
	Vec3d r1 = end - center;

	// theta
	double theta = 0;
	if ( (r0.dot(r1) / (r0.length() * r1.length())) > 1 ) theta = acos(1);
	else theta = acos(r0.dot(r1) / (r0.length() * r1.length()));
	int N = max(1.0, theta / (2*Pi/arcPrecision) );
	double dtheta = a * theta / N;
    Quaterniond Q(axis, dtheta);

    cout << "rotate theta " << theta << ", N " << N << " -> dtheta " << dtheta << endl;

	// move
	double A = 0;
	for (int q = 0; q < N; q++) {
		// rotate vector 0 around dtheta degrees
		Vec3d r;
		Q.multVec(r0, r);
		translate(r0 + center, r + center, v_0);
		r0 = r;
		A += dtheta;
        cout << " rotate " << q << ", A " << A << endl;
	}
}

// https://ncviewer.com/

void VRMachiningCode::readGCode(string path, double speedMultiplier) {
	cout << "VRMachiningCode::readGCode " << path << " " << speedMultiplier << endl;
	Vec3d vec0, vec1;

	ifstream file;
	file.open(path);
	if (!file.is_open()) { cout << "ERROR in VRMachiningCode::readGCode : file '" << path << "' not found!" << endl; return; }

	// state variables
	int motionMode = 1; // G0, G1, G2, G3
	float speed = 50;
	Vec3d rotationAxis = Vec3d(0,-1,0);
	Vec3d cursor;
	Vec3d target;
	Vec3d rotationCenter;

	struct Command {
        char code = 'G';
        float value = 0;
	};

	string line;
	while (getline(file, line)) {
		// parse commands
		vector<Command> commands;
		for (auto i : splitString(line)) {
            if (i.size() <= 1) { cout << " Warning! empty line: '" << line << "'" << endl; continue; }
            commands.push_back( { i[0], toFloat(subString(i, 1)) } );
		}

		// apply commands to current state
		rotationCenter = cursor;
		for (auto& cmd : commands) {
            if (cmd.code == 'V') speed = cmd.value * speedMultiplier;

            if (cmd.code == 'G') {
                if (cmd.value > -1 && cmd.value < 4 ) motionMode = cmd.value;
                if (cmd.value > 16 && cmd.value < 20) {  // G17 (Y), G18 (Z), G19 (X)
                    if (cmd.value == 17) rotationAxis = Vec3d(0,-1,0);
                    if (cmd.value == 18) rotationAxis = Vec3d(0,0,1);
                    if (cmd.value == 19) rotationAxis = Vec3d(1,0,0);
                }
            }

            if (cmd.code == 'X') target[0] =  cmd.value;
            if (cmd.code == 'Y') target[2] = -cmd.value;
            if (cmd.code == 'Z') target[1] =  cmd.value;

            if (cmd.code == 'I') rotationCenter[0] = cursor[0] + cmd.value;
            if (cmd.code == 'J') rotationCenter[2] = cursor[2] - cmd.value;
            if (cmd.code == 'K') rotationCenter[1] = cursor[1] + cmd.value;
		}

		if (motionMode == 0 || motionMode == 1) { // translate
			translate(cursor, target, speed);
		}

		if (motionMode == 2) { // rotate clockwise
			rotate(cursor, target, rotationCenter, rotationAxis, speed, 1);
		}

		if (motionMode == 3) { // rotate counterclockwise
			rotate(cursor, target, rotationCenter, rotationAxis, speed, -1);
		}

		cursor = target;

		//implementation of the other G Code commands!
	}

	cout << " read " << instructions.size() << " instructions" << endl;
}

//Implement here to plot the gcode path

VRGeometryPtr VRMachiningCode::asGeometry() {
    VRGeoData data;

    // Print all elements of the vector
    for (const auto& element : instructions) {

        //cout << "G: " << element.G << endl;
        //cout << "d: " << element.d << endl;
        //cout << "p0: " << element.p0 << endl;
        //cout << "T: " << element.T << endl;

        data.pushVert(element.p0, Vec3d(0,1,0));
        data.pushLine();
    }

    auto geo = data.asGeometry("ncCode");
    auto m = VRMaterial::create("ncCode");
    m->setLit(0);
    m->setLineWidth(2);
    geo->setMaterial(m);
    return geo;
}




