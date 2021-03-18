#include "VRMachiningCode.h"
#include "core/utils/toString.h"

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
	if (T * (skippedSteps + 1) > 1.0 / 60) {
		Instruction i;
		i.G = 0;
		i.d = vecf;
		i.p0 = vec_0;
		i.T = T;
		instructions.push_back(i);
		skippedSteps = 0;
	} else skippedSteps += 1;
}

void VRMachiningCode::rotate(Vec3d vec_0, Vec3d vec_1, Vec3d m_0, double v_0, int mode) { // G02 and G03 rotation
	double a = 0;
	if (mode == 1) a = 1;
	else a = -1;

	// vector
	Vec3d r0 = vec_0 - m_0;
	Vec3d r1 = vec_1 - m_0;

	// theta
	double theta = 0;
	if ( (r0.dot(r1) / (r0.length() * r1.length())) > 1 ) theta = acos(1);
	else theta = acos(r0.dot(r1) / (r0.length() * r1.length()));
	double dtheta = a * theta / arcPrecision;

	// matrix of rotation
	Matrix4d m = {	cos(dtheta), -sin(dtheta), 0, 0,
					sin(dtheta),  cos(dtheta), 0, 0,
					0, 0, 0, 0,
					0, 0, 0, 0 };

	// move
	for (int q = 0; q < arcPrecision; q++) {
		// rotate vector 0 around dtheta degrees
		Vec3d r;
		m.mult(r0, r);
		translate(r0 + m_0, r + m_0, v_0);
		r0 = r;
	}
}

void VRMachiningCode::readGCode(string path, double speedMultiplier) {
	cout << "VRMachiningCode::readGCode " << path << " " << speedMultiplier << endl;
	Vec3d vec0, vec1;

	ifstream file;
	file.open(path);
	if (!file.is_open()) { cout << "ERROR in VRMachiningCode::readGCode : file '" << path << "' not found!" << endl; return; }

	string code;
	while (getline(file, code)) {
		map<char, double> params;
		for (auto i : splitString(code)) {
			params[i[0]] = toFloat(subString(i, 1, i.size()-1));
		}

		if (!params.count('V')) params['V'] = 50; // standard speed
		if (!params.count('G')) params['G'] = 1; // standard movement

		double v = 1.0;

		if (params['G'] < 4) { // get speed
			vec1 = vec0;
			v = params['V']*speedMultiplier;
			if (params.count('X')) vec1[0] = params['X'];
			if (params.count('Y')) vec1[1] = params['Y'];
			if (params.count('Z')) vec1[2] = params['Z'];
		}

		if (params['G'] == 0 || params['G'] == 1) { // translate
			translate(vec0, vec1, v);
			vec0 = vec1;
		}

		if (params['G'] == 2) { // rotate clockwise
			Vec3d m = Vec3d(params['I'], params['J'], 0.0);
			rotate(vec0,vec1,m,v,1);
			vec0 = vec1;
		}

		if (params['G'] == 3) { // rotate counterclockwise
			Vec3d m = Vec3d(params['I'], params['J'], 0.0);
			m *= 0.001;
			rotate(vec0, vec1, m, v, -1);
			vec0 = vec1;
		}
	}

	cout << " read " << instructions.size() << " instructions" << endl;
}

