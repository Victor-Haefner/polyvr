#version 400 compatibility
#define M_PI 3.1415926535897932384626433832795

// gen
in vec3 norm;
in vec4 pos;
in vec2 tcs;
in mat3 miN;
in mat4 miP;
vec3 fragDir;
vec3 PCam;
vec3 axU;
vec3 axV;
vec3 worldVec;
vec4 color;
bool debugB = false;
float disBD = 0.04;
float radius = 0.3;

uniform float scale;
uniform int pass;

uniform vec2 OSGViewportSize;
uniform float tnow;
uniform float offset;

uniform bool isRaining;
uniform bool isWiping;
uniform float wiperSpeed;
uniform float tWiperstart;
uniform float durationWiper;

uniform vec3 windshieldPos;
uniform vec3 windshieldDir;
uniform vec3 windshieldUp;

void computeDirection() {
	fragDir = normalize( miN * (miP * pos).xyz );
}

float hash(vec2 co) {
    	return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void computePCam() {
	mat4 m = inverse(gl_ModelViewMatrix);
	PCam = (m*vec4(0,0,0,1)).xyz;
}

void computeDepth(vec4 position) {
	float d = position.z / position.w;
	gl_FragDepth = d*0.5 + 0.5;
}

vec3 computeDropOnWS() {
	float x = dot((windshieldPos - PCam),windshieldUp)/(dot(fragDir,windshieldUp));
	vec3 p = x * fragDir + PCam;
	computeDepth(gl_ModelViewProjectionMatrix*vec4(p,1));
	return p;
}

vec3 localToWorld(vec2 inVec) {
	vec3 outVec;
	outVec = windshieldPos + axU*inVec.x + axV*inVec.y;
	return outVec;
}

vec2 worldToLocal(vec3 inV) {
	vec2 outV;
	outV.x = dot((inV-windshieldPos),axU);
	outV.y = dot((inV-windshieldPos),axV);
	return outV;
}

vec2 genDropOffset(vec2 uv, float disBD, float maxDispl) {
	vec2 seed = vec2( floor(uv.x/disBD), floor(uv.y/disBD) );
	vec2 o = vec2(hash(seed.xy),hash(seed.yx));
	o = normalize(o)*disBD*2;
	o.x = clamp(o.x, -maxDispl, maxDispl);
	o.y = clamp(o.y, -maxDispl, maxDispl);
	return o;
}

vec2 genPattern1Offset(vec2 uvC) {
	float X = floor(uvC.x/disBD);
	float K = mod(X,2);
	vec2 disp = vec2(0.3,0.0)*disBD;
	if (K > 0.5) disp *= -1;
	return disp;
}

vec2 genPattern2Offset(vec2 uvC) {
	float X = floor(uvC.x/disBD);
	float Y = floor(uvC.y/disBD);
	//float a = Y*X*50*disBD;
	float a = hash(vec2(X, Y))*7;
	if (pass == 1) a = hash(vec2(Y, X))*21;
	return vec2(cos(a), sin(a))*disBD*radius;
}

//calcs angles of wipers A and B in dependence of current time - [90|120] maxDegree
vec2 angles() {
	float period = durationWiper/(wiperSpeed);
	float time = (tnow-tWiperstart)/period;
	float angA = 0;
	float angB = 0;	
	if (time<0.5) {
		angA = 2*  90/180*M_PI * time;
		angB = 4.8*time; 
	} 
	if (time>0.5) {
		angA = 2*  90/180*M_PI-2*  90/180*M_PI * time;
		angB = 4.8-4.8*time;
	}
	return vec2(angA,angB);
}

float calcDeltaTime(float angle, int whichWiper) {
	float period = durationWiper/(wiperSpeed);
	float time = (tnow-tWiperstart)/period;
	float timeWiperLocal;
	if (whichWiper == 0) {
		if (time<0.5) {
			timeWiperLocal = angle/3.1416;
		} 
		if (time>0.5) {
			timeWiperLocal = -(angle-3.1416)/3.1416;
		}
	}
	if (whichWiper == 1) {
		if (time<0.5) {
			timeWiperLocal = angle/4.8; 
		} 
		if (time>0.5) {
			timeWiperLocal = -(angle-4.8)/4.8; 
		}	
	}	
	return timeWiperLocal*period;
}

//calcs time the wiper travels to angle, from 0
float calcTravelTime(float angle, int whichWiper) { 	
	float period = durationWiper/wiperSpeed;
	float travelTime;
	if (whichWiper == 0) {
		travelTime = angle/3.1416 * period;	
	}
	if (whichWiper == 1) {
		travelTime = angle/4.8 * period;
	}
	return travelTime;
}

//alpha = angle between 0 and drop, phi = angle between 0 and wiper, calcs deltaTime the wiper passed last
float calcLocalDeltaTime(float alpha, float phi, int whichWiper) { 
	float period = durationWiper/(wiperSpeed);
	float time = (tnow-tWiperstart)/period;
	float deltaTime;
	if (phi < alpha) {
		if (time < 0.5) { deltaTime = calcTravelTime(alpha,whichWiper) + calcTravelTime(phi,whichWiper); } 
		else { deltaTime = calcTravelTime(alpha,whichWiper) - calcTravelTime(phi,whichWiper); }		
	} else {
		if (time < 0.5) { deltaTime = calcTravelTime(phi,whichWiper) - calcTravelTime(alpha,whichWiper); } 
		else { deltaTime = period-calcTravelTime(alpha,whichWiper)-calcTravelTime(phi,whichWiper); }
	}
	return deltaTime;
}

bool calcTime(vec2 uvC) {
	vec2 uvG = worldToLocal(worldVec);
	float xAC = -floor((uvC.x-0.6)/disBD);
	float yAC = floor((uvC.y)/disBD);
	float xBC = -floor(uvC.x/disBD);
	float yBC = floor((uvC.y)/disBD);

	float xA = -(uvG.x-0.6);
	float yA = uvG.y;
	float xB = -(uvG.x);
	float yB = uvG.y;
	float hashTime = hash(vec2(xBC,yBC));
	if (hashTime<0.1) hashTime=0.1;

	bool ctrlAbounds = (atan(yA,xA)>0 && atan(yA,xA)<3.1416/2 && distance(uvG,vec2(0+0.6,0))<0.5);
	bool ctrlAWiper = (hashTime*20/scale > calcLocalDeltaTime(atan(yA,xA), angles().x, 0));
	bool ctrlBbounds = (atan(yB,xB)>0 && atan(yB,xB)<4.8/2 && distance(uvG,vec2(0,0))<0.5);
	bool ctrlBWiper = (hashTime*20/scale > calcLocalDeltaTime(atan(yB,xB), angles().y, 1));
	
	if (ctrlAbounds) if (ctrlAWiper) return false;
	if (ctrlBbounds) if (ctrlBWiper) return false;
	return true;
}

bool drawWipers(){
	vec2 uvG = worldToLocal(worldVec);
	float xA = -(uvG.x-0.6);
	float yA = uvG.y;
	float xB = -(uvG.x);
	float yB = uvG.y;
	
	if (distance(uvG,vec2(0+0.6,0))<0.5) if ((atan(yA,xA)-angles().x>0.0) && (atan(yA,xA)-angles().x<0.01)) return true; 
	if (distance(uvG,vec2(0,0))<0.5) if ((atan(yB,xB)-angles().y>0.0) && (atan(yB,xB)-angles().y<0.02)) return true;
	return false;
}

vec4 locateDrop() {
	vec2 uv = worldToLocal(worldVec) + vec2(0.5,0.5)*disBD*pass*3.54;
	if(distance(uv,vec2(0,0))>1.5) return vec4(-10,-10,0,0);
	uv= uv + vec2(0,1)*tnow/20*(1+0.2*pass+0.2*scale); //TODO: INSERT INERTIA DRIVEN OFFSET TO SIMULATE RAIN MOVEMENT ON GLASS
	vec2 uvUnchanged = uv;
	uv += genPattern2Offset(uv);

	float limitValue = disBD;

	float hsIn1 = floor(uv.x/disBD) + 50*floor((tnow+1.9)/4);
	float hsIn2 = floor(uv.y/disBD);
	float hs1 = hash(vec2(hsIn1,hsIn2));
	
	if (calcTime(uvUnchanged)) return vec4(uv.x,uv.y,mod(uv.x,disBD)/disBD,mod(uv.y,disBD)/disBD);
	
	return vec4(-10,-10,0,0);
}

vec4 returnColor(vec4 drop) {
	vec4 dropColor = vec4(0,0,0,0);
	float dir = dot(drop.zw-vec2(0.5,0.5), vec2(0,1+radius));
	float dist = distance(drop.zw,vec2(0.5,0.5));
	//float alph = dist*abs(dir);
	//float alph = smoothstep(1.0-radius+(radius-0.1)*(1-scale*0.1),0.9,1-dist)*(0.5-dist*1.5*dir);
	float alph = smoothstep(1.0-radius,0.95,1-dist)*(0.5-dist*1.5*dir);
	if (debugB) alph = 1;	
	vec4 check1 = vec4(0.2,0.2,0.3,0.7*alph);
	vec4 check2 = vec4(1,1,1,0.7*alph);
	dropColor = mix(check1, check2, -dir*32*dist);
	//dropColor = vec4(dir,dir,dir,1);

	//if (debugB) dropColor = vec4(0.6,0,0,0.9*alph);
	return dropColor;
}

//OverRideColor
vec4 orc(vec4 inCl1, vec4 inCl2) { 
	if (inCl2.x == -10) return inCl1;
	return inCl1;
}

vec4 drawBorder(vec4 check) {
	vec2 uvG = worldToLocal(worldVec);
	if (uvG.x<-0.7 && distance(windshieldPos,worldVec) < 1.5) return vec4(0,0,0,1);
	if (uvG.x>0.7 && distance(windshieldPos,worldVec) < 1.5) return vec4(0,0,0,1);
	if (uvG.y<-0.1 && distance(windshieldPos,worldVec) < 1.5) return vec4(0,0,0,1);
	if (uvG.y>0.6 && distance(windshieldPos,worldVec) < 1.5) return vec4(0,0,0,1);
	return check;
}

void main() {
	if (scale < 0.01) discard;
	radius *= scale*0.1;
	radius = clamp(radius, 0.0501, 0.3);

	computeDirection();
	computePCam();
	worldVec = computeDropOnWS();
	axU = cross(windshieldDir,windshieldUp);
	axV = windshieldDir;
	
	if (fragDir.y < -0.999) discard; //not sure if needed, but previous experiences showed conflicts with RAIN-MODULE's heightcam 
	vec4 dropColor = vec4(0,0,0,0);	
	if (!isRaining) discard;
	vec4 drop = locateDrop();
	dropColor = returnColor(drop);
	//dropColor = drawBorder(dropColor);
	if (drawWipers()) dropColor=vec4(0,0,0,1);
	
	if (dropColor == vec4(0,0,0,0)) discard;	
	if (dropColor.w < 0.2) discard;	
	gl_FragColor = dropColor;
}

/** NOT NEEDED RIGHT NOW */







