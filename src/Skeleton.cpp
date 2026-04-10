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
#include <asm-generic/errno.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <execution>
#include <thread>
#include <vector>

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

class CatmullRomSpline {
public:
// Nem Hermite-nak kéne lennie?
	Geometry<vec2>  draw_curve;	// Bezier görbe
	Geometry<vec2>  draw_cps;	// Bezier görbe kontrollpontjai
	vec2 a0, a1, a2, a3;    // polinom együtthatók
	float segment_tau;
	// int tauMax = 0;         // az aktív paramétertartomány

	std::vector<vec2> cps;
	std::vector<float> ts; // gondolom egyes csomópontoknál milyen t legyen

	void Hermite(vec2 p0, vec2 v0, float t0, vec2 p1, vec2 v1, float t1, float t){
		a0 = p0;
		a1 = v0;
		a2 = ( 3*(p1 - p0) / pow(t1-t0, 2) ) - ( (v1 + 2*v0) / (t1 - t0) );
		a3 = ( 2*(p0-p1) / pow(t1-t0, 3) ) + ( (v1+v0) / pow(t1-t0, 2) );
		segment_tau = t-t0;
	};
	void update_data(){

		cps.clear();

		// az első pontnak kell az utolsó, hogy lehessen sebességet számítani
		cps.push_back(draw_cps.Vtx()[draw_cps.Vtx().size()-1]);

		for(vec2 elem : draw_cps.Vtx()){
			cps.push_back(elem);
		}

		// Ez az utolsó pont, ami valójában az első
		cps.push_back(draw_cps.Vtx()[0]);

		// Ez az utolsó pont utáni, valójában a második, hogy lehessen sebességet számítani
		cps.push_back(draw_cps.Vtx()[1]);

		ts.clear();
		ts.push_back(0);
		for(size_t i = 1; i < cps.size(); i++){
			ts.push_back(ts[i-1] + length(cps[i] - cps[i-1]));
		}

		// printf("%d, %d", cps.size(), ts.size());
	}
	void update() {
		update_data();
		float tauMax = ts[ts.size()-2];
		float tauMin = ts[1];
		// draw_cps-t külön kéne kezelni, amikor a pontokat hozzáadom
		// mert így meg lehet oldani, hogy az eleje / vége duplán legyen tárolva a számításokhoz, de itt meg csak egyszer legyenek
		draw_curve.Vtx().clear();
		if (draw_cps.Vtx().size() >= 3) {
			const int nVertices = 100; // A közelítő töröttvonal csúcsszáma
			// printf("Tau values:\n");
			for (int i = 0; i < nVertices; i++) {	// Tesszelláció
				float tau = tauMin + (float)i * (tauMax-tauMin) / nVertices;
				// printf("%.2f, ", tau);
				draw_curve.Vtx().push_back(r(tau));
			}
			// draw_curve.Vtx().push_back(r(tauMax - 0.0001f));
			draw_curve.updateGPU();
		}
	}
	// float paramMax() { return tauMax; }
	// megkeresi a megfelelő polynomrészletet, és kitölteti a Hermite-al az értékeket
	void Polynom(float tau){
		// mik a határok?
		static bool asd = false;
		// if(!asd){
		// 	std::vector<vec2> speedvec;
		// 	for(size_t i = 1; i < cps.size()-1; i++){
		// 		vec2 vi = 0.5f * ( ( (cps[i+1] - cps[i]) / (ts[i+1] - ts[i]) ) + ( (cps[i] - cps[i-1]) / (ts[i] - ts[i-1]) ) );
		// 		asd = true;
		// 	}
		// }

		for(size_t i = 1; i<cps.size() - 2; i++){
			if(ts[i] <= tau && tau <= ts[i+1]){
				vec2 v0 = 0.5f * ( ( (cps[i+1] - cps[i]) / (ts[i+1] - ts[i]) ) + ( (cps[i] - cps[i-1]) / (ts[i] - ts[i-1]) ) );
				i++;
				vec2 v1 = 0.5f * ( ( (cps[i+1] - cps[i]) / (ts[i+1] - ts[i]) ) + ( (cps[i] - cps[i-1]) / (ts[i] - ts[i-1]) ) );
				i--;

				Hermite(cps[i], v0, ts[i], cps[i+1], v1, ts[i+1], tau);
				return;
			}
		}
		printf("Wrong tau");
	}
	vec2 r(float tau) { // Catmull-Rom spline
		Polynom(tau);
		// MAJD VISSZACSERÉLNI
		return a3*pow(segment_tau, 3) + a2*pow(segment_tau, 2) + a1*segment_tau + a0;
		// return ((a3*segment_tau + a2)*segment_tau + a1)*segment_tau + a0;
	}
	// ez kell
	vec2 r1d(float tau) { // Első derivált
		Polynom(tau);
		return (3.0f * a3 * segment_tau + 2.0f * a2) * segment_tau + a1;
	}
	void Draw(GPUProgram* gpuProgram, Camera2D& camera) {
		mat4 MVP = camera.P() * camera.V();
		gpuProgram->setUniform(MVP, "MVP");
		draw_curve.Draw(gpuProgram, GL_LINE_LOOP, vec3(1, 1, 1)); // g�rbe
		// draw_curve.Draw(gpuProgram, GL_POINTS, vec3(1, 0, 1)); // g�rbe
		// draw_cps.Draw(gpuProgram, GL_LINES, vec3(1, 1, 0)); // g�rbe
		draw_cps.Draw(gpuProgram, GL_POINTS, vec3(1, 0, 0));	// kontrollpontok
	}
	// TODO: Itt adja hozzá jól a knot value-t.
	void addControlPoint(vec2 p){
		printf("Added control point %f, %f\n", p.x, p.y);

		draw_cps.Vtx().push_back(p);
		draw_cps.updateGPU();

		if(draw_cps.Vtx().size() >= 3)
			update();
	}
};

class Ball: Geometry<vec2>{
	vec2 pos, v0;
	const vec2 g = vec2(0, -5);

	float collision_time;

	constexpr static float radius = 1.0;
public:
	Ball(vec2 pos, vec2 v0){
		this->pos = pos;
		this->v0 = v0;

		const int nVertices = 100;
		for(int i = 0; i < nVertices; i++){
			float phi = i * 2.0f * (float)M_PI / nVertices;
			Vtx().push_back(vec2(radius*cosf(phi), radius*sinf(phi)));
		}
		updateGPU();
	}
	void Draw(GPUProgram *prog, Camera2D& camera){
		mat4 MVP = translate(vec3(pos.x, pos.y, 0));
		MVP = camera.P()*camera.V()*MVP;
		prog->setUniform(MVP, "MVP");
		// lehet más a szín?
		Geometry<vec2>::Draw(prog, GL_TRIANGLE_FAN, vec3(0, 0, 1));
	}
	// Lehet nem így kell, mert nem az elejétől kéne számítani, hanem a deltákból?
	void updateFromTime(float maxT, std::vector<vec2>* spline, Geometry<vec2>* collisionPoints){
		// TODO NEM STATIC HANEM A BALL-NAK TAGVÁLTOZÓJA
		static int fordulasos_updateok = 0;
		bool voltfordulas = false;
		static int perv_collision_id = -1;
		while (true) {
			int collision_id = -1;
			collision_time = maxT;

			if(!spline->empty()){
				// ide a collision checkek
				for(size_t i = 0; i < spline->size()-1; i++){
					// kétszer ne ütközzön ugyan azzal
					if(i == perv_collision_id) continue;

					vec2 line_start = (*spline)[i];
					vec2 line_end = (*spline)[i+1];
					if(this->collideWithLineSegment(line_start, line_end, maxT, collisionPoints)){
						collision_id = i;
					}
				}

				if(perv_collision_id != spline->size()){
					if(this->collideWithLineSegment((*spline)[spline->size()-1], (*spline)[0], maxT, collisionPoints)){
						collision_id = spline->size();
					}
				}

			}
			// felzárkóztatni collision time-ig
			pos = pos + collision_time*v0 + collision_time*collision_time*g/2;
			v0 = v0 + collision_time*g;

			// megtörtént az ütközés, max time csökken
			maxT = maxT - collision_time;

			// ha nem volt ütközés, akkor a collision time dt volt, OK
			if(collision_id == -1) break;

			// elmentjük, valójában kivel ütközött
			perv_collision_id = collision_id;

			vec2 collision_dirvec = vec2(NAN, NAN);

			if(collision_id == spline->size()){
				collision_dirvec = (*spline)[spline->size()-1] - (*spline)[0];
			}else {
				collision_dirvec = (*spline)[collision_id] - (*spline)[collision_id+1];
			}

			printf("Fennmaradó maxT: %f\n", maxT);
			printf("FORDUL %.2f, %.2f\n", v0.x, v0.y);
			printf("%f, %f\n", collision_dirvec.x, collision_dirvec.y);
			// volt ütközés, a frissített v0-t át kell forgatni
			vec2 paralell = (dot(v0, collision_dirvec) / dot(collision_dirvec, collision_dirvec)) * collision_dirvec;
			vec2 perpend = v0 - paralell;

			v0 = paralell - perpend;
			printf("FORDUL2 %.2f, %.2f\n", v0.x, v0.y);
			voltfordulas = true;
		}

		if(voltfordulas){
			printf("\n");
			fordulasos_updateok++;
		}

	}
	// kiszámítja a metszéspontot, beállítja a közös változóban, hogy milyen t-vel ütközik, és ha ütközött, igazat ad vissza
	bool collideWithLineSegment(vec2 line_start, vec2 line_end, float maxT, Geometry<vec2>* collisionPoints){
		// MAJD MINDENKÉPP LEGYEN BENNE, HOGY LINE_LOOP ÉS KELL FELVENNI ITT KÜLÖN AZ UTOLSÓT, AMIKOR EZT A FÜGGVÉNYT MEGHÍVJUK
		vec2 dirvec = line_end-line_start;
		vec2 normvec = vec2(-dirvec.y, dirvec.x);

		// egyenes egyenlete Ax + By + C = 0
		// ax = normvec.x;
		// by = normvec.y;
		// constant = dot(-1 * normvec, vec3(p.x, p.y, 0));
		float lineA = normvec.x;
		float lineB = normvec.y;
		float lineC = dot(-1*normvec, line_start);

		float A = 0.5*g.y*lineB;
		float B = v0.y*lineB + v0.x*lineA;
		float C = lineA*pos.x + lineB*pos.y + lineC;

		std::vector<float> collision_times;

		float D = B * B - 4 * A * C;
		collisionPoints->Vtx().clear();

		bool had_collision = false;
		if (D < 0){
			// nincs collision point
		}else if (D == 0) {
			float t = (-B) / (2 * A);
			collision_times.push_back(t);

			if(0 < t && t<=collision_time){
				collision_time = t;
				// collision_dirvec = dirvec;
				had_collision = true;
				// vec2 coll = pos + t*v0 + t*t*g/2;
				// printf("Collision with %.2f, %.2f\n", coll.x, coll.y);
				// collisionPoints->Vtx().push_back(coll);
			}
		}
		else
		{
			float t1 = (-B + sqrt(D)) / (2 * A);
			float t2 = (-B - sqrt(D)) / (2 * A);
			vec2 coll1 = pos + t1*v0 + t1*t1*g/2;
			vec2 coll2 = pos + t2*v0 + t2*t2*g/2;

			float xmin = fmin(line_start.x, line_end.x);
			float xmax = fmax(line_start.x, line_end.x);
			if(xmin <= coll1.x && coll1.x <= xmax){
				if(0 < t1 && t1 <= maxT){
					collision_time = t1;
					// collision_dirvec = dirvec;
					had_collision = true;
					// printf("Collision with %.2f, %.2f\n", coll1.x, coll1.y);
					// collisionPoints->Vtx().push_back(coll1);
					// collision_times.push_back(t1);
				}
			}
			if(xmin <= coll2.x && coll2.x <= xmax){
				if(0 < t2 && t2 <= maxT){
					collision_time = t2;
					// collision_dirvec = dirvec;
					had_collision = true;
					// printf("Collision with %.2f, %.2f\n", coll2.x, coll2.y);
					// collisionPoints->Vtx().push_back(coll2);
					// collision_times.push_back(t2);
				}
			}
		}

		return had_collision;
		// collisionPoints->updateGPU();
	}
};

class BallApp : public glApp {

	CatmullRomSpline* spline;
	Ball* ball;
	GPUProgram* gpuProgram = nullptr;
	Geometry<vec2>* testpoints;
	Geometry<vec2>* collisionpoints;
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

		spline = new CatmullRomSpline();
		// a segítőkész sarkok
		// spline->addControlPoint(vec2(20.0, 10.0));
		// spline->addControlPoint(vec2(10.0, 15.0));
		// spline->addControlPoint(vec2(-15.0, 15.0));
		// spline->addControlPoint(vec2(-20.0, -15.0));
		// spline->addControlPoint(vec2(10, 10));

		ball = new Ball(vec2(-10, 10), vec2(3.5, 10));
		testpoints = new Geometry<vec2>();
		collisionpoints = new Geometry<vec2>();


		spline->addControlPoint(vec2(-10, 10));
		spline->addControlPoint(vec2(-6.5, 20));
		spline->addControlPoint(vec2(-6.5, 10));


		testpoints->Vtx().push_back(vec2(10, 10));
		testpoints->Vtx().push_back(vec2(-10, -15));

		// vec2 a = testpoints->Vtx()[0];
		// vec2 b = testpoints->Vtx()[1];
		ball->collideWithLineSegment(testpoints->Vtx()[0], testpoints->Vtx()[1], 1.0, collisionpoints);

		testpoints->updateGPU();
		collisionpoints->updateGPU();
	}

	void onDisplay() {
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);

		spline->Draw(gpuProgram, camera);
		ball->Draw(gpuProgram, camera);

		gpuProgram->setUniform(camera.P()*camera.V(), "MVP");
		testpoints->Draw(gpuProgram, GL_POINTS, vec3(0, 1, 1));
		collisionpoints->Draw(gpuProgram, GL_POINTS, vec3(1, 0, 0));
		Geometry<vec2> test;
		for(auto e : spline->draw_cps.Vtx()){
			test.Vtx().push_back(e);
		}
		test.updateGPU();
		test.Draw(gpuProgram, GL_POINTS, vec3(1, 0, 1));
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
		// printf("start: %f\nend: %f\ndelta: %f\n", startTime, endTime, endTime-startTime);
		ball->updateFromTime(endTime-startTime, &spline->draw_curve.Vtx(), collisionpoints);
		refreshScreen();
	}
};

BallApp app;
