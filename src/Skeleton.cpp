//=============================================================================================
// Textúra leképzés
//=============================================================================================
#include "framework.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <ios>
// csúcspont árnyaló
const char * vertSourceText = R"(
	#version 330

	layout(location = 0) in vec2 vertexXY;	// Attrib Array 0
	layout(location = 1) in vec2 vertexUV;			// Attrib Array 1

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
	uniform bool texturing;
	uniform vec3 color;

	in vec2 texCoord;			// variable input: interpolated texture coordinates
	out vec4 outColor;		// output that goes to the raster memory as told by glBindFragDataLocation

	void main() {
		if(texturing){
			outColor = texture(textureUnit, texCoord);
		}else{
			outColor = vec4(color, 1);
		}
	}
)";

// print vec
void pvec(vec2 v){
	printf("(%.2f %.2f)\n", v.x, v.y);
}
void pvec(vec3 v){
	printf("(%.2f %.2f %.2f)\n", v.x, v.y, v.z);
}
void pvec(vec4 v){
	printf("(%.2f %.2f %.2f %.2f)\n", v.x, v.y, v.z, v.w);
}

template<typename T> T lerp(T a, T b, float t){
	return a*(1-t) + t * b;
}




const int winWidth = 600, winHeight = 600;

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

// input v = phi, theta
// r = 1
// beágyazó tér???
vec3 sphere23d(vec2 v){
	float x = sin(v.x)*cos(v.y);
	float y = sin(v.x)*sin(v.y);
	float z = cos(v.y);
	return vec3(x, y, z);
}

// input v = phi, theta
vec2 sphere2merc(vec2 v){
	return vec2(v.x, log(tan(M_PI_4f + v.y /2)));
}
// output v = phi, theta
vec2 merc2sphere(vec2 v){
	float out;
	out = v.y;
	out = exp(out);
	out = atan(out);
	out = out - M_PI_4f;
	out = out*2;
	return vec2(v.x, out);
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
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		int textureUnit = 0; // textúra mintavevő egység
		gpuProgram->setUniform(textureUnit, "textureUnit"); // türkisz nyíl
		texture.Bind(textureUnit);      		                    // piros nyíl
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);        // négyszög rajzolás
	}

	void valamiupdate()	{
		for(int i = 0; i<uncompressed_image.size(); i++){
			if((i+ i/64)%2 == 0){
				if(uncompressed_image[i].x > 0) uncompressed_image[i].x = 0.5;
				if(uncompressed_image[i].y > 0) uncompressed_image[i].y = 0.5;
				if(uncompressed_image[i].z > 0) uncompressed_image[i].z = 0.5;
			}
		}
		texture.updateTexture(64, 64, uncompressed_image);
	}
};

struct Test{
	// point stored as sphere coordinates
	std::vector<vec2> points;
	Geometry<vec2> drawPoints;

	void addSpherePoint(vec2 p){
		printf("added point ");
		pvec(sphere2merc(p));
		drawPoints.Vtx().push_back(sphere2merc(p));
		drawPoints.updateGPU();
	}

	void Draw(GPUProgram* prog){
		drawPoints.Draw(prog, GL_POINTS, vec3(1, 0, 0));
	}
};

class TextureApp : public glApp {
	Geometry<vec2> *line;
	WorldMap* wm;
	GPUProgram* gpuProgram;
	bool mousePressed = false;

	Test* test;
	// TODO: Legyen ez alapján a viewport
	int vpX = 0, vpY = 0, vpWidth = winWidth, vpHeight = winHeight;
	vec3 PixelToNDC(int pX, int pY) {
		pY = winHeight - pY;
		return vec3(2.0f * (pX - vpX) / vpWidth - 1, 2.0f * (pY - vpY) / vpHeight - 1, 1);
	}
public:
	TextureApp() : glApp(3, 3, winWidth, winHeight, "Texturing") { }
	void onInitialization() {
		gpuProgram = new GPUProgram(vertSourceText, fragSourceText);
		glClearColor(0, 0, 0, 0);     // háttér fekete


		// MEGNÉZNI A JÓT
		glLineWidth(3);
		glPointSize(5);

		uncompress();

		wm = new WorldMap();
		line = new Geometry<vec2>;
		test = new Test();

		auto a = vec2(0.25, -M_PI_4f);
		auto b = vec2(0.25, M_PI_4f);

		for(int i = 0; i < 100; i++){
			printf("i: %d\t", i);
			auto v = lerp(a, b, (float)i/100);
			pvec(v);
			test->addSpherePoint(v);
		}



		line->Vtx().push_back(vec2(0, 0));
		line->Vtx().push_back(vec2(1, 0));
		line->updateGPU();
		printf("\n");
	}
	void onDisplay() {
		glClear(GL_COLOR_BUFFER_BIT); // törlés
		glViewport(0, 0, winWidth, winHeight);
		gpuProgram->setUniform(true, "texturing");
		wm->Draw(gpuProgram);
		gpuProgram->setUniform(false, "texturing");
		line->Draw(gpuProgram, GL_LINES, vec3(1, 0, 0));
		test->Draw(gpuProgram);
	}
	void onMousePressed(MouseButton button, int pX, int pY) {
		printf("asd\n");
		wm->valamiupdate();
	}
	void onMouseReleased(MouseButton button, int pX, int pY) {
	}
	void onMouseMotion(int pX, int pY) {
		refreshScreen();
	}
};

TextureApp app;
