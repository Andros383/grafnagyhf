//=============================================================================================
// Mintaprogram: Zold h�romszog. Ervenyes 2025.-tol
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni, ide�rtve ChatGPT-t �s t�rsait is
// - felesleges programsorokat a beadott programban hagyni
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL es GLM fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Falucskai Andras
// Neptun : A2PT9J
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================//=============================================================================================

// VIGYÁZZ!!! Includeolgat a zed
#include "framework.h"

// diasorról másolva
class Camera2D {
	vec2 wCenter; // center in world coords
	vec2 wSize; // width and height in world coords
public:
	Camera2D(vec2 wCenter, vec2 wSize){
		this->wCenter = wCenter;
		this->wSize = wSize;
	}
	mat4 V() {
		return translate(vec3(-wCenter.x, -wCenter.y, 0));
	}
	mat4 P() { // projection matrix
		return scale(vec3(2/wSize.x, 2/wSize.y, 1));
	}
	mat4 Vinv() { // inverse view matrix
		return translate(vec3(wCenter.x, wCenter.y, 0));
	}
	mat4 Pinv() { // inverse projection matrix
		return scale(vec3(wSize.x/2, wSize.y/2, 1));
	}
	// Ezek asszem nem kellenek
	void Zoom(float s) { wSize = wSize * s; }
	void Pan(vec2 t) { wCenter = wCenter + t; }
};

const char* vertSource = R"(
	#version 330
    precision highp float;

	layout(location = 0) in vec2 vertexPosition;

	uniform mat4 MVP;

	void main() {
		gl_Position = MVP * vec4(vertexPosition, 0, 1);
	}
)";

const char* fragSource = R"(
	#version 330
    precision highp float;

	uniform vec3 color;
	out vec4 fragmentColor;

	void main() {
		fragmentColor = vec4(color, 1);
	}
)";


const int winWidth = 600, winHeight = 600;

// class Object : public Geometry<vec2> {
// protected:
// 	vec2 scaling = vec2(1, 1), pos = vec2(0, 0);
// 	float phi = 0; // rotation
// public:
// 	// vectorization, ear clipping, etc.
// 	virtual std::vector<vec2> GenVertexData() = 0;
// 	void update() {
// 		Vtx() = GenVertexData();
// 		updateGPU();
// 	}
// 	void Draw(GPUProgram* gpuProgram, int type, vec3 color, Camera2D& camera) {
// 		mat4 M = translate(pos) * rotate(phi, vec3(0, 0, 1)) * scale(scaling);
// 		mat4 MVP = camera.P() * camera.V() * M;
// 		gpuProgram->setUniform(MVP, "MVP");
// 		Geometry<vec2>::Draw(gpuProgram, type, color);
// 	}
// };

class CatmullRomSpline {
protected:
// Nem Hermite-nak kéne lennie?
	Geometry<vec2>  curve;	// Bezier görbe
	Geometry<vec2>  cps;	// Bezier görbe kontrollpontjai
	vec2 a0, a1, a2, a3;    // polinom együtthatók
	int tauMax = 0;         // az aktív paramétertartomány
	void Polynom(float tau) {
		for (unsigned int i = 0; i < tauMax; i++) {
			if (i <= tau && tau < i + 1) {
				vec2 p0 = cps.Vtx()[i], p1 = cps.Vtx()[i + 1];
				vec2 vPrev = (i > 0) ? (p0 - cps.Vtx()[i - 1]) : vec2(0, 0);
				vec2 vCur = (p1 - p0);
				vec2 vNext = (i < cps.Vtx().size() - 2) ? (cps.Vtx()[i + 2] - p1) : vec2(0, 0);
				vec2 v0 = (vPrev + vCur) / 2.0f;
				vec2 v1 = (vCur + vNext) / 2.0f;
				a0 = p0;
				a1 = v0;
				a2 = (p1 - p0) * 3.0f - (v1 + v0 * 2.0f);
				a3 = (p0 - p1) * 2.0f + (v1 + v0);
				return;
			}
		}
	}
public:
	void update() {
		tauMax = cps.Vtx().size() - 1;
		cps.updateGPU();
		curve.Vtx().clear();
		if (cps.Vtx().size() > 0) {
			const int nVertices = 100; // A k�zel�t� t�r�ttvonal cs�cssz�ma
			for (int i = 0; i < nVertices; i++) {	// Tesszell�ci�
				float tau = (float)i * tauMax / nVertices;
				curve.Vtx().push_back(r(tau));
			}
			curve.Vtx().push_back(r(tauMax - 0.0001f));
			curve.updateGPU();
		}
	}
	float paramMax() { return tauMax; }
	vec2 r(float tau) { // Catmull-Rom spline
		Polynom(tau);
		tau = tau - (int)tau;
		return ((a3 * tau + a2) * tau + a1) * tau + a0;
	}
	vec2 r1d(float tau) { // Els� deriv�lt
		Polynom(tau);
		tau = tau - (int)tau;
		return (3.0f * a3 * tau + 2.0f * a2) * tau + a1;
	}
	// ez asszem nem is kell
	vec2 r2d(float tau) { // M�sodik deriv�lt
		Polynom(tau);
		tau = tau - (int)tau;
		return 6.0f * a3 * tau + 2.0f * a2;
	}
	void Draw(GPUProgram* gpuProgram, Camera2D camera) {
		mat4 MVP = camera.P() * camera.V();
//		mat4 MVP = scale(vec3(1.0f, 1.0f, 1));

		gpuProgram->setUniform(MVP, "MVP");
		curve.Draw(gpuProgram, GL_LINE_STRIP, vec3(1, 1, 1)); // g�rbe
		cps.Draw(gpuProgram, GL_POINTS, vec3(1, 0, 0));	// kontrollpontok
	}
	void addControlPoint(vec2 p){
		cps.Vtx().push_back(p);
		update();
	}
};



// vec3 mert azzal lehet konstruálni a mat4-eket? Vagy nem tudom milyen dimenziójú vektor kéne.
// szerintem ez is geometry, nem kell obj, majd állítjuk az MVP-t a spline-ban, vagy csak egyszer az elején
// obj egyáltalán nem kell, mert nem rajzolom ki ugyan azt többször
// class CatmullRom: public Geometry<vec2>{
// 	std::vector<vec2> control_points;
// 	std::vector<float> knot_values;
// public:
// 	void addControlPoint(vec2 p){
// 		Vtx().push_back(p);
// 		updateGPU();
// 	}
// 	void Draw(GPUProgram *prog){
// 		// mat4 MVP = camera.P()*camera.V();
// 		// prog->setUniform(MVP, "MVP");
// 		// ős hívása
// 		Geometry<vec2>::Draw(prog, GL_LINE_STRIP, vec3(1, 1, 0));
// 	}
// };

// szerintem majd ő csak egyszer fogja állítani az MVP-t, mint egyszer rajzolandó objektum
// mert a pontokat és a görbét egybefogja, azaz egyben tartja azt is, hogy hogyan kell ezeket eltranszformálni
class Spline{
	CatmullRomSpline spline;
	// TODO felcserélgetni, hogy minden OK-e?
	// csak érdekel mi van, ha megfordítom a mátrixszorzás sorrendjét.
public:
	// TODO knot érték számítása
	void addControlPoint(vec2 p){
		printf("Control point %.2f %.2f added\n", p.x, p.y);
		spline.addControlPoint(p);
	}
	void Draw(GPUProgram *prog, Camera2D& camera){
		prog->setUniform(camera.P()*camera.V(), "MVP");
		spline.Draw(prog, camera);
	}
};

class Ball: Geometry<vec2>{
	vec2 pos;
	vec2 startpos, v0;
	const vec2 g = vec2(0, -5);
	// kell parabola
	// mátrixos reprezentáció???

	// lehet más
	// radiust beleépíteni az MVP-be? Elég krézi lenne
	// mivel nincs változó labda, jó ez így
	constexpr static float radius = 1.0;
public:
	Ball(vec2 pos, vec2 v0){
		this->startpos = pos;
		this->v0 = v0;
		this->updateFromTime(0);

		const int nVertices = 100;
		for(int i = 0; i < nVertices; i++){
			float phi = i * 2.0f * (float)M_PI / nVertices;
			Vtx().push_back(vec2(radius*cosf(phi), radius*sinf(phi)));
		}
		updateGPU();
	}
	// Lehet nem így kell, mert nem az elejétől kéne számítani, hanem a deltákból?
	void updateFromTime(float dt){
		startpos = startpos + dt*v0 + dt*dt*g/2;
		v0 = v0 + dt*g;
		pos = startpos;
		// pos = startpos + dt*v0 + dt*dt*g/2;
	}
	void Draw(GPUProgram *prog, Camera2D& camera){
		mat4 MVP = translate(vec3(pos.x, pos.y, 0));
		MVP = camera.P()*camera.V()*MVP;
		prog->setUniform(MVP, "MVP");
		// lehet más a szín?
		Geometry<vec2>::Draw(prog, GL_TRIANGLE_FAN, vec3(0, 0, 1));
	}
};

class BallApp : public glApp {

	Spline* spline;
	Ball* ball;
	GPUProgram* gpuProgram = nullptr;

	// nincs benne OpenGL hívás
	Camera2D camera = Camera2D(vec2(0, 0), vec2(50, 50));

	// ezzel mi lesz?
	int vpX = 0, vpY = 0, vpWidth = winWidth, vpHeight = winHeight;

public:
	// ezt asszem át kell írni majd hogy a MVP-vel dolgozzon?
	vec2 PixelToNDC(int pX, int pY) {
		pY = winHeight - pY;
		return vec2(2.0f * (pX - vpX) / vpWidth - 1, 2.0f * (pY - vpY) / vpHeight - 1);
	}
	void onMousePressed(MouseButton button, int pX, int pY) {
		if (button != MOUSE_LEFT) return;

		vec2 clicked = PixelToNDC(pX, pY);
		vec4 translated = camera.Vinv() * camera.Pinv() * vec4(clicked.x, clicked.y, 0, 1);
		spline->addControlPoint(vec2(translated.x, translated.y));

		refreshScreen();
	}
	BallApp() : glApp("Ball App") {}

	void onInitialization() {
		gpuProgram = new GPUProgram(vertSource, fragSource);

		// Ki kell venni a *2-t
		glViewport(0, 0, winHeight*2, winWidth*2);

		// ez nem volt specifikálva, megnézni!
		glLineWidth(3);
		glPointSize(10);

		// elég itt beállítani, mert úgy se változik a program alatt
		mat4 MVP = camera.P() * camera.V();
		gpuProgram->setUniform(MVP, "MVP");

		spline = new Spline();
		// a segítőkész sarkok
		spline->addControlPoint(vec2(25.0, 25.0));
		spline->addControlPoint(vec2(25.0, -25.0));
		spline->addControlPoint(vec2(-25.0, 25.0));
		spline->addControlPoint(vec2(-25.0, -25.0));

		ball = new Ball(vec2(-20, 10), vec2(2, 10));
	}

	void onDisplay() {
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);

		spline->Draw(gpuProgram, camera);
		ball->Draw(gpuProgram, camera);
	}

	void onKeyboard(int key) {
		switch (key) {
		case 'p':
			break;
		default:
			return;
		}
	}

	void onTimeElapsed(float startTime, float endTime){
		printf("start: %f\nend: %f\ndelta: %f\n", startTime, endTime, endTime-startTime);
		ball->updateFromTime(endTime-startTime);
		refreshScreen();
	}
};

BallApp app;
