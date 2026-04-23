//=============================================================================================
// Textúra leképzés
//=============================================================================================
#include "framework.h"
<<<<<<< HEAD

class Camera2D {
	vec2 wCenter;
	vec2 wSize;
	int vpX, vpY, vpWidth, vpHeight;
public:
	Camera2D(vec2 wCenter, vec2 wSize, int vpX, int vpY, int vpWidth, int vpHeight) {
		this->wCenter = wCenter;
		this->wSize = wSize;
		this->vpX = vpX;
		this->vpY = vpY;
		this->vpWidth = vpWidth;
		this->vpHeight = vpHeight;
	}
	mat4 V() {
		return translate(vec3(-wCenter.x, -wCenter.y, 0));
	}
	mat4 P() {
		return scale(vec3(2 / wSize.x, 2 / wSize.y, 1));
	}
	mat4 Vinv() {
		return translate(vec3(wCenter.x, wCenter.y, 0));
	}
	mat4 Pinv() {
		return scale(vec3(wSize.x / 2, wSize.y / 2, 1));
	}

	vec2 convertClick(int pX, int pY) {
		pY = vpHeight - pY;
		vec2 clicked =  vec2(2.0f * (pX - vpX) / vpWidth - 1, 2.0f * (pY - vpY) / vpHeight - 1);
		vec4 translated = this->Vinv() * this->Pinv() * vec4(clicked.x, clicked.y, 0, 1);
		return vec2(translated.x, translated.y);
	}
};
=======
#include <cstdio>
#include <cstdlib>
>>>>>>> 7417295 (Harmadik házi kezdete, nem fogok új repót csinálni mert lusta vagyok.)

const char* vertSource = R"(
	#version 330
    precision highp float;

	layout(location = 0) in vec2 vertexPosition;

	void main() {
		gl_Position = vec4(vertexPosition, 0, 1);
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

// csúcspont árnyaló
const char * vertSourceText = R"(
	#version 330

	layout(location = 0) in vec2 vertexXY;	// Attrib Array 0
	layout(location = 1) in vec2 vertexUV;			// Attrib Array 1

<<<<<<< HEAD
class CatmullRomSpline {
public:
	Geometry<vec2>  draw_curve;
	Geometry<vec2>  draw_cps;
	vec2 a0, a1, a2, a3;
	float segment_tau = 0;

	std::vector<vec2> cps;
	std::vector<float> ts;

	void Hermite(vec2 p0, vec2 v0, float t0, vec2 p1, vec2 v1, float t1, float t) {
		a0 = p0;
		a1 = v0;
		a2 = (3 * (p1 - p0) / pow(t1 - t0, 2)) - ((v1 + 2 * v0) / (t1 - t0));
		a3 = (2 * (p0 - p1) / pow(t1 - t0, 3)) + ((v1 + v0) / pow(t1 - t0, 2));
		segment_tau = t - t0;
	};
	void update_data() {
		cps.clear();

		cps.push_back(draw_cps.Vtx()[draw_cps.Vtx().size() - 1]);

		for (vec2 elem : draw_cps.Vtx()) {
			cps.push_back(elem);
		}

		cps.push_back(draw_cps.Vtx()[0]);

		cps.push_back(draw_cps.Vtx()[1]);

		ts.clear();
		ts.push_back(0);
		for (size_t i = 1; i < cps.size(); i++) {
			ts.push_back(ts[i - 1] + length(cps[i] - cps[i - 1]));
		}
	}
	void update() {
		update_data();
		float tauMax = ts[ts.size() - 2];
		float tauMin = ts[1];

		draw_curve.Vtx().clear();
		if (draw_cps.Vtx().size() >= 3) {
			const int nVertices = 100;
			for (int i = 0; i < nVertices; i++) {
				float tau = tauMin + (float)i * (tauMax - tauMin) / nVertices;
				draw_curve.Vtx().push_back(r(tau));
			}
			draw_curve.Vtx().push_back(r(tauMax - 0.0001f));
			draw_curve.updateGPU();
		}
	}
	void Polynom(float tau) {
		for (size_t i = 1; i < cps.size() - 2; i++) {
			if (ts[i] <= tau && tau <= ts[i + 1]) {
				vec2 v0 = 0.5f * (((cps[i + 1] - cps[i]) / (ts[i + 1] - ts[i])) + ((cps[i] - cps[i - 1]) / (ts[i] - ts[i - 1])));
				i++;
				vec2 v1 = 0.5f * (((cps[i + 1] - cps[i]) / (ts[i + 1] - ts[i])) + ((cps[i] - cps[i - 1]) / (ts[i] - ts[i - 1])));
				i--;
				Hermite(cps[i], v0, ts[i], cps[i + 1], v1, ts[i + 1], tau);
				return;
			}
		}
	}
	vec2 r(float tau) {
		Polynom(tau);
		return ((a3 * segment_tau + a2) * segment_tau + a1) * segment_tau + a0;
	}
	void Draw(GPUProgram* gpuProgram, Camera2D& camera) {
		mat4 MVP = camera.P() * camera.V();
		gpuProgram->setUniform(MVP, "MVP");
		draw_curve.Draw(gpuProgram, GL_LINE_LOOP, vec3(1, 1, 0));
		draw_cps.Draw(gpuProgram, GL_POINTS, vec3(1, 0, 0));
	}
	void addControlPoint(vec2 p) {
		draw_cps.Vtx().push_back(p);
		draw_cps.updateGPU();

		if (draw_cps.Vtx().size() >= 3)
			update();
	}
};

class Ball : Geometry<vec2> {
public:
	vec2 pos, v0;
	const vec2 g = vec2(0, -5);

	float collision_time = -1;

	int perv_collision_id = -1;

	constexpr static float radius = 1.0;

	Ball(vec2 pos, vec2 v0) {
		this->pos = pos;
		this->v0 = v0;

		const int nVertices = 100;
		for (int i = 0; i < nVertices; i++) {
			float phi = i * 2.0f * (float)M_PI / nVertices;
			Vtx().push_back(vec2(radius * cosf(phi), radius * sinf(phi)));
		}
		updateGPU();
	}
	void Draw(GPUProgram* prog, Camera2D& camera) {
		mat4 MVP = translate(vec3(pos.x, pos.y, 0));
		MVP = camera.P() * camera.V() * MVP;

		prog->setUniform(MVP, "MVP");

		Geometry<vec2>::Draw(prog, GL_TRIANGLE_FAN, vec3(0, 0, 1));
	}
	void updateFromTime(float maxT, std::vector<vec2>* spline) {
		while (true) {
			int collision_id = -1;
			collision_time = maxT;

			if (!spline->empty()) {
				for (size_t i = 0; i < spline->size() - 1; i++) {
					if (i == perv_collision_id) continue;

					vec2 line_start = (*spline)[i];
					vec2 line_end = (*spline)[i + 1];
					if (this->collideWithLineSegment(line_start, line_end, maxT)) {
						collision_id = i;
					}
				}

				if (perv_collision_id != spline->size()) {
					if (this->collideWithLineSegment((*spline)[spline->size() - 1], (*spline)[0], maxT)) {
						collision_id = spline->size();
					}
				}

			}
			pos = pos + collision_time * v0 + collision_time * collision_time * g / 2;
			v0 = v0 + collision_time * g;

			maxT = maxT - collision_time;

			if (collision_id == -1) break;

			perv_collision_id = collision_id;

			vec2 collision_dirvec = vec2(NAN, NAN);

			if (collision_id == spline->size()) {
				collision_dirvec = (*spline)[spline->size() - 1] - (*spline)[0];
			}
			else {
				collision_dirvec = (*spline)[collision_id] - (*spline)[collision_id + 1];
			}

			vec2 paralell = (dot(v0, collision_dirvec) / dot(collision_dirvec, collision_dirvec)) * collision_dirvec;
			vec2 perpend = v0 - paralell;

			v0 = paralell - perpend;
		}
	}
	bool collideWithLineSegment(vec2 line_start, vec2 line_end, float maxT) {
		vec2 dirvec = line_end - line_start;
		vec2 normvec = vec2(-dirvec.y, dirvec.x);

		float lineA = normvec.x;
		float lineB = normvec.y;
		float lineC = -1 * dot(normvec, line_start);

		float A = 0.5 * g.y * lineB;
		float B = v0.y * lineB + v0.x * lineA;
		float C = lineA * pos.x + lineB * pos.y + lineC;

		float D = B * B - 4 * A * C;

		bool had_collision = false;
		if (D < 0) {

		}
		else if (D == 0) {
			float t = (-B) / (2 * A);
			if (0 < t && t <= collision_time) {
				collision_time = t;
				had_collision = true;
			}
		}
		else
		{
			float t1 = (-B + sqrt(D)) / (2 * A);
			float t2 = (-B - sqrt(D)) / (2 * A);
			vec2 coll1 = pos + t1 * v0 + t1 * t1 * g / 2;
			vec2 coll2 = pos + t2 * v0 + t2 * t2 * g / 2;

			float xmin = fmin(line_start.x, line_end.x);
			float xmax = fmax(line_start.x, line_end.x);
			if (xmin <= coll1.x && coll1.x <= xmax) {
				if (0 < t1 && t1 <= maxT) {
					collision_time = t1;
					had_collision = true;
				}
			}
			if (xmin <= coll2.x && coll2.x <= xmax) {
				if (0 < t2 && t2 <= maxT) {
					collision_time = t2;
					had_collision = true;
				}
			}
		}

		return had_collision;
	}
};

class BallApp : public glApp {

	CatmullRomSpline* spline = nullptr;
	std::vector<Ball*> balls;
	Ball* current_ball = nullptr;
	GPUProgram* gpuProgram = nullptr;

	int vpX = 0, vpY = 0, vpWidth = winWidth, vpHeight = winHeight;

	Camera2D camera = Camera2D(vec2(0, 0), vec2(50, 50), vpX, vpY, vpWidth, vpHeight);

public:
	void onMousePressed(MouseButton button, int pX, int pY) {

		if (button != MOUSE_LEFT && button != MOUSE_RIGHT) return;

		vec2 world_clicked = camera.convertClick(pX, pY);

		if (button == MOUSE_LEFT) {
			spline->addControlPoint(world_clicked);
		}
		if (button == MOUSE_RIGHT) {
			current_ball = new Ball(world_clicked, vec2(0, 0));
		}
		refreshScreen();
	}
	void onMouseReleased(MouseButton button, int pX, int pY) {
		if (button != MOUSE_RIGHT) return;

		vec2 world_clicked = camera.convertClick(pX, pY);

		current_ball->v0 = vec2(current_ball->pos - world_clicked);
		current_ball->pos = world_clicked;
		balls.push_back(current_ball);
		current_ball = nullptr;
	}
	BallApp() : glApp("Ball App") {}

=======
	out vec2 texCoord;								// output attribute

	void main() {
		texCoord = vertexUV;														// copy texture coordinates
		gl_Position = vec4(vertexXY, 0, 1);
	}
)";

// pixel árnyaló
const char * fragSourceText = R"(
	#version 330

	uniform sampler2D textureUnit;

	in vec2 texCoord;			// variable input: interpolated texture coordinates
	out vec4 outColor;		// output that goes to the raster memory as told by glBindFragDataLocation

	void main() { outColor = texture(textureUnit, texCoord); }
)";


const int winWidth = 960, winHeight = 540;

std::vector<int16_t> compressed_image = {2308, 13, 84, 5, 152, 17, 76, 17, 144, 29, 64, 25, 136, 45, 16, 5, 32, 29, 132, 65,
4, 21, 8, 33, 128, 85, 12, 41, 120, 93, 4, 73, 20, 9, 52, 101, 4, 81, 4, 29, 12, 5,
12, 1129, 14, 241, 18, 241, 18, 245, 14, 145, 6, 97, 18, 125, 6, 9, 10, 97, 18, 53,
10, 61, 30, 93, 22, 45, 14, 61, 30, 93, 22, 45, 18, 5, 6, 49, 30, 93, 30, 37, 22,
61, 18, 97, 30, 37, 22, 61, 14, 97, 22, 9, 6, 37, 22, 45, 6, 9, 6, 9, 10, 93, 26,
33, 6, 5, 26, 41, 6, 5, 10, 5, 6, 101, 26, 33, 42, 49, 6, 101, 14, 5, 6, 41, 46, 21,
6, 17, 6, 9, 6, 93, 14, 53, 50, 29, 14, 105, 6, 9, 10, 41, 54, 13, 34, 93, 14, 13,
6, 41, 54, 9, 42, 85, 30, 49, 26, 21, 58, 81, 34, 45, 26, 21, 62, 5, 6, 65, 42, 69,
6, 13, 62, 73, 50, 41, 10, 5, 14, 13, 66, 69, 42, 9, 6, 37, 106, 61, 58, 37, 106,
61, 34, 5, 18, 37, 6, 13, 90, 13, 6, 25, 10, 17, 30, 9, 10, 57, 98, 33, 30, 5, 26,
33, 4, 33, 102, 9, 18, 13, 50, 5, 6, 9, 10, 9, 6, 4, 17, 4, 17, 122, 5, 6, 5, 46, 5,
8, 9, 10, 9, 6, 8, 37, 82, 5, 10, 5, 10, 5, 14, 9, 38, 5, 24, 6, 13, 6, 16, 29, 18,
9, 34, 5, 58, 17, 10, 13, 6, 5, 36, 17, 12, 41, 6, 17, 6, 5, 74, 53, 32, 17, 20, 57,
6, 5, 50, 5, 6, 5, 6, 57, 32, 21, 20, 61, 18, 5, 22, 5, 18, 5, 6, 61, 28, 17, 24,
61, 4, 6, 5, 6, 21, 4, 21, 6, 65, 4, 9, 20, 9, 28, 29, 4, 45, 6, 121, 24, 5, 32, 29,
4, 37, 6, 5, 6, 121, 20, 9, 28, 205, 20, 5, 28, 81, 4, 125, 16, 5, 24, 5, 4, 209,
12, 5, 4, 5, 16, 217, 8, 17, 16, 913};

std::vector<vec3> uncompressed_image;

void uncompress(){
	printf("Size of image: %lu\n", compressed_image.size());
	for(int16_t elem : compressed_image){
		bool print = false;
		if(elem == 13) print = true;
		int16_t mask = 0b0000000000000011;
		int16_t lower = elem & mask;
		int upper = (elem & (~mask)) >> 2;

		vec3 color = vec3(-1, -1, -1);
		switch (lower) {
			case 0b00:
				color = vec3(1, 1, 1);
				break;
			case 0b01:
				color = vec3(0, 0, 1);
				break;
			case 0b10:
				color = vec3(0, 1, 0);
				break;
			case 0b11:
				color = vec3(0, 0, 0);
				break;
		}

		if(color.x < 0){
			printf("Invalid color code\n");
			exit(1);
		}

		printf("Pushing %d db %.2f %.2f %.2f\n", upper, color.x, color.y, color.z);

		for(int i = 0; i<upper; i++){
			uncompressed_image.push_back(color);
		}


		// if(print){
		// 	printf("%x\n", elem);
		// 	printf("%x\n", lower);
		// 	printf("%x\n", upper);
		// }
	}
	printf("uncompressed size: %lu\n", uncompressed_image.size());
}

struct WorldMap {
	unsigned int vao, vbo[2];	// vao és két vbo: geometria + uv
	std::vector<vec2> vtx;	    // geometria a CPU-n
	Texture texture;            // kép a tapétázáshoz
	int picked = -1;		    // kiválasztott csúcspont sorszáma
	WorldMap() : texture(64, 64){
		texture.updateTexture(64, 64, uncompressed_image);

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(2, &vbo[0]);  // egyszerre két vbo-t kérünk
		// a négyszög csúcsai kezdetben normalizált eszközkoordinátákban
		vtx = { vec2(-1, -1), vec2(1, -1), vec2(1, 1), vec2(-1, 1) };
		updateGPU();               // GPU-ra másoljuk
		glEnableVertexAttribArray(0); // a 0. vbo a 0. bemeneti regisztert táplálja
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL); // csúcsonként 2 float-tal
		// a négyszög csúcsai textúratérben
		// std::vector<vec2> uvs = { vec2(0, 1), vec2(1, 1), vec2(1, 0), vec2(0, 0) };
		std::vector<vec2> uvs = { vec2(0, 0), vec2(1, 0), vec2(1, 1), vec2(0, 1) };
		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]); // GPU-ra másoljuk
		glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(vec2), &uvs[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(1); // a 1. vbo a 1. bemeneti regisztert táplálja
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL); // csúcsonként 2 float-tal
	}
	void updateGPU() { // vtx tömb átmásolása a GPU-ra a vbo[0] VBO-ba
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		glBufferData(GL_ARRAY_BUFFER, vtx.size() * sizeof(vec2), &vtx[0], GL_DYNAMIC_DRAW);
	}
	void Draw(GPUProgram* gpuProgram) {
		int textureUnit = 0; // textúra mintavevő egység
		gpuProgram->setUniform(textureUnit, "textureUnit"); // türkisz nyíl
		texture.Bind(textureUnit);                          // piros nyíl
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);        // négyszög rajzolás
	}
};

class TextureApp : public glApp {
	Geometry<vec2> *line;
	WorldMap* wm;
	GPUProgram* gpuProgram;
	GPUProgram* gpuProgramText;
	bool mousePressed = false;
public:
	TextureApp() : glApp(3, 3, winWidth, winHeight, "Texturing") { }
>>>>>>> 7417295 (Harmadik házi kezdete, nem fogok új repót csinálni mert lusta vagyok.)
	void onInitialization() {
		gpuProgram = new GPUProgram(vertSource, fragSource);
		gpuProgramText = new GPUProgram(vertSourceText, fragSourceText);
		glClearColor(0, 0, 0, 0);     // háttér fekete

<<<<<<< HEAD
		glViewport(0, 0, winHeight, winWidth);

=======
>>>>>>> 7417295 (Harmadik házi kezdete, nem fogok új repót csinálni mert lusta vagyok.)
		glLineWidth(3);

<<<<<<< HEAD
		spline = new CatmullRomSpline();
=======
		uncompress();

		wm = new WorldMap();
		line = new Geometry<vec2>;
		line->Vtx().push_back(vec2(0, 0));
		line->Vtx().push_back(vec2(1, 0));
		line->updateGPU();
>>>>>>> 7417295 (Harmadik házi kezdete, nem fogok új repót csinálni mert lusta vagyok.)
	}
	void onDisplay() {
<<<<<<< HEAD
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);

		spline->Draw(gpuProgram, camera);

		if (current_ball != nullptr) {
			current_ball->Draw(gpuProgram, camera);
		}

		for (Ball* b : balls) {
			b->Draw(gpuProgram, camera);
		}
	}

	void onTimeElapsed(float startTime, float endTime) {
		for (Ball* b : balls) {
			b->updateFromTime(endTime - startTime, &spline->draw_curve.Vtx());
		}

=======
		glClear(GL_COLOR_BUFFER_BIT); // törlés
		glViewport(0, 0, winWidth, winHeight);
		gpuProgramText->Use();
		wm->Draw(gpuProgramText);
		gpuProgram->Use();
		line->Draw(gpuProgram, GL_LINES, vec3(1, 0, 0));
	}
	void onMousePressed(MouseButton button, int pX, int pY) {
	}
	void onMouseReleased(MouseButton button, int pX, int pY) {
	}
	void onMouseMotion(int pX, int pY) {
>>>>>>> 7417295 (Harmadik házi kezdete, nem fogok új repót csinálni mert lusta vagyok.)
		refreshScreen();
	}
};

TextureApp app;
