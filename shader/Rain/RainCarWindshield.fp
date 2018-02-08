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

vec2 genPattern1Offset(vec2 uv) {
	float X = floor(uv.x/disBD);
	float K = mod(X,2);
	vec2 disp = vec2(0.3,0.0)*disBD;
	if (K > 0.5) disp *= -1;
	return disp;
}

vec2 genPattern2Offset(vec2 uv) {
	float X = floor(uv.x/disBD);
	float Y = floor(uv.y/disBD);
	//float a = Y*X*50*disBD;
	float a = hash(vec2(X, Y))*7;
	if (pass == 1) a = hash(vec2(Y, X))*21;
	return vec2(cos(a), sin(a))*disBD*radius;
}

vec2 angles() {
	float period = durationWiper/(wiperSpeed);
	float time = (tnow-tWiperstart)/period;
	float angA = 0;
	float angB = 0;	
	if (time<0.5) {
		angA = 2*  90/180*M_PI * time;
		angB = 4.8*time;//2* 150/180*M_PI * time; 
	} 
	if (time>0.5) {
		angA = 2*  90/180*M_PI-2*  90/180*M_PI * time;
		angB = 4.8-4.8*time;//2* 150/180*M_PI-2* 150/180*M_PI * time;
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
float calcTravelTime(float angle, int whichWiper) { 	float period = durationWiper/wiperSpeed;
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

bool calcTime(vec2 uv) {
	float xA = -floor((uv.x-0.6)/disBD);
	float yA = floor((uv.y)/disBD);
	float xB = -floor(uv.x/disBD);
	float yB = floor((uv.y)/disBD);
	float tLocal = tnow;// + hash(vec2(xB,yB));
	float hashTime = hash(vec2(xB,yB));
	if (atan(yB,xB)<0 || atan(yB,xB)>4.8/2 || distance(uv,vec2(0,0))>0.5) return true;
	if (hashTime*0.3*scale > calcLocalDeltaTime(atan(yB,xB), angles().y, 1)) return false;
	return true;
	//if (distance(uv,vec2(0.5,0))>0.05 && distance(uv,vec2(0.5,0))<0.5 && atan(yA,xA)-angles().x<0.0) return false;
	//if (distance(uv,vec2(0,0))>0.05 && distance(uv,vec2(0,0))<0.5 && atan(yB,xB)-angles().y<0.0) return false;
	
	float t = 3;
	//if (distance(uv,vec2(0.5,0))<0.5 && tnow-tWiperstart-calcDeltaTime(atan(yA,xA),0)>0.00 && tnow-tWiperstart-calcDeltaTime(atan(yA,xA),0)<0.1) return false;
	//if (distance(uv,vec2(0,0))<0.5 && tnow-tWiperstart-calcDeltaTime(atan(yB,xB),1)>0.00 && tnow-tWiperstart-calcDeltaTime(atan(yB,xB),1)<0.1) return false;
	//if (tnow-tWiperstart-calcDeltaTime(atan(yA,xA),0)>0) return true;
	if (distance(uv,vec2(0,0))>0.5 || uv.y<0) return true;
	if (atan(yB,xB)>2.4) return true; 
	if (tLocal-tWiperstart-calcDeltaTime(atan(yB,xB),1)>-0.05&&tLocal-tWiperstart-calcDeltaTime(atan(yB,xB),1)<0) return false;
	//if (tLocal-tWiperstart-calcDeltaTime(atan(yA,xA),1)>-0.05&&tLocal-tWiperstart-calcDeltaTime(atan(yA,yA),1)<0) return false;
	//if (distance(uv,vec2(0.5,0))<0.5 && tnow-tWiperstart-calcDeltaTime(atan(yA,xA),0)>0 && tnow-tWiperstart-calcDeltaTime(atan(yA,xA),0)<2) return true;
	//if (distance(uv,vec2(0,0))<0.5 && tnow-tWiperstart-calcDeltaTime(atan(yB,xB),1)>0 && tnow-tWiperstart-calcDeltaTime(atan(yB,xB),1)<2) return true;
	
	return true;
	
	float timefunction = tWiperstart;
	float localWiperTime = (tWiperstart);
	//if (tnow - localWiperTime>0.1) return true;
	return true;
	return false;
}

vec4 locateDrop() { //locate drop, if wipers active
	vec3 worldVec = computeDropOnWS();
	vec2 uv = worldToLocal(worldVec) + vec2(0.5,0.5)*disBD*pass;
	if(distance(uv,vec2(0,0))>1.5) return vec4(-10,-10,0,0);
	vec2 uvUnchanged = uv;
	uv += genPattern2Offset(uv);

	float limitValue = disBD;

	float hsIn1 = floor(uv.x/disBD) + 50*floor((tnow+1.9)/4);
	float hsIn2 = floor(uv.y/disBD);
	float hs1 = hash(vec2(hsIn1,hsIn2));
	
	if (calcTime(uvUnchanged)) return vec4(uv.x,uv.y,mod(uv.x,disBD)/disBD,mod(uv.y,disBD)/disBD);
	
	return vec4(-10,-10,0,0);
}

vec4 locateContDrop(float inputScale) {	//locate continuuos drop, if wipers non active
	vec3 worldVec = computeDropOnWS();
	vec2 uv = worldToLocal(worldVec) + vec2(0.5,0.5)*disBD*pass;
	float limitValue = disBD;
	float hsIn1 = floor(uv.x/disBD) + 50*floor((tnow)/400);
	float hsIn2 = floor(uv.y/disBD);
	float hs1 = hash(vec2(hsIn1,hsIn2));
	float hs2 = hash(vec2(hsIn2,hsIn1));
	
	//uv += offset;
	//uv += genDropOffset(uv, disBD, 0.5*disBD);
	uv += genPattern2Offset(uv);
	float asd = (-uv.y+6)*hs2;
	return vec4(uv.x,uv.y,mod(uv.x,disBD)/disBD,mod(uv.y,disBD)/disBD);
	if (mod(tnow,8)<4) {
		if (asd < mod(tnow,8)*0.6+2 && asd > (mod(tnow,8)-1)*0.6+2.3-0.1*scale){ 
			return vec4(uv.x,uv.y,mod(uv.x,disBD)/disBD,mod(uv.y,disBD)/disBD);
		}
	} else {
		if (asd < (8-mod(tnow,8))*0.6+2 && asd > ((8-mod(tnow,8))-1)*0.6+2.3-0.1*scale){ 
			return vec4(uv.x,uv.y,mod(uv.x,disBD)/disBD,mod(uv.y,disBD)/disBD);
		}
	}
	return vec4(-10,-10,0,0);
}

vec4 returnColor(vec4 drop) {
	vec4 dropColor = vec4(0,0,0,0);
	float dir = dot(drop.zw-vec2(0.5,0.5), vec2(0,1+radius));
	float dist = distance(drop.zw,vec2(0.5,0.5));
	//float alph = dist*abs(dir);
	//float alph = smoothstep(1.0-radius+(radius-0.1)*(1-scale*0.1),0.9,1-dist)*(0.5-dist*1.5*dir);
	float alph = smoothstep(1.0-radius,0.95,1-dist)*(0.5-dist*1.5*dir);
	vec4 check1 = vec4(0.2,0.2,0.3,0.7*alph);
	vec4 check2 = vec4(1,1,1,0.7*alph);
	dropColor = mix(check1, check2, -dir*32*dist);
	//dropColor = vec4(dir,dir,dir,1);

	if (debugB) dropColor = vec4(0.6,0,0,0.9*alph);
	return dropColor;
}

vec4 orc(vec4 inCl1, vec4 inCl2) { //OverRideColor
	if (inCl2.x == -10) return inCl1;
	return inCl1;
}

vec4 drawWiper(vec4 check) {
	vec3 worldVec = computeDropOnWS();
	vec2 uv = worldToLocal(worldVec);
	if (uv.x<-0.7 && distance(windshieldPos,worldVec) < 1.5) return vec4(0,0,0,1);
	if (uv.x>0.7 && distance(windshieldPos,worldVec) < 1.5) return vec4(0,0,0,1);
	if (uv.y<-0.3 && distance(windshieldPos,worldVec) < 1.5) return vec4(0,0,0,1);
	if (uv.y>0.6 && distance(windshieldPos,worldVec) < 1.5) return vec4(0,0,0,1);
	
	float xoff = 0.25;
	float yoff = -0.8;
	float xoff2 = -1.;
	float yoff2 = -0.8;
	if (mod(tnow,4)>2 && mod(tnow,4)<4) {
	//if ((uv.x-xoff2)*(uv.x-xoff2)+(uv.y-yoff2)*(uv.y-yoff2)<1.2 && (uv.x-xoff2)*(uv.x-xoff2)+(uv.y-yoff2)*(uv.y-yoff2)>0.05) return vec4(0,0,0,0);
	//if ((uv.x-xoff)*(uv.x-xoff)+(uv.y-yoff)*(uv.y-yoff)<1.2 && (uv.x-xoff)*(uv.x-xoff)+(uv.y-yoff)*(uv.y-yoff)>0.05) return vec4(0,0,0,0);
	
	}
	else {
	
	}
	return check;
}

void main() {
	if (scale < 0.01) discard;
	radius *= scale*0.1;
	radius = clamp(radius, 0.0501, 0.3);

	computeDirection();
	computePCam();
	axU = cross(windshieldDir,windshieldUp);
	axV = windshieldDir;
	
	if (fragDir.y < -0.999) discard; //not sure if needed, but previous experiences showed conflicts with RAIN-MODULE's heightcam 
	vec4 dropColor = vec4(0,0,0,0);	
	if (!isRaining) discard;
	vec4 drop = locateDrop();
	dropColor = returnColor(drop);
	dropColor = drawWiper(dropColor);
	
	if (dropColor == vec4(0,0,0,0)) discard;	
	gl_FragColor = dropColor;
}

/** NOT NEEDED RIGHT NOW */







