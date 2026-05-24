//=============================================================================================
// Computer Graphics Sample Program: 3D engine-let
// Shader: Gouraud, Phong, NPR
// Material: diffuse + Phong-Blinn
// Texture: CPU-procedural
// Geometry: sphere, tractricoid, torus, mobius, klein-bottle, boy, dini
// Camera: perspective
// Light: point or directional sources
//=============================================================================================
#include <cmath>
#include <system_error>
#include <tuple>
#include <vector>
const float OP_SYS_SCALE = 2.0;


#include "framework.h"
// TODO mindenhol ezt az ambienst használni:
// textúrák, meg az alap, ahol van, talán shaderben
const vec3 AMBIENT = vec3(0.4, 0.4, 0.4);

//---------------------------
template<class T> struct Dnum { // Dual numbers for automatic derivation
//---------------------------
	float f; // function value

	T d;  // derivatives
	Dnum(float f0 = 0, T d0 = T(0)) { f = f0, d = d0; }
	Dnum operator+(Dnum r) { return Dnum(f + r.f, d + r.d); }
	Dnum operator-(Dnum r) { return Dnum(f - r.f, d - r.d); }
	Dnum operator*(Dnum r) {
		return Dnum(f * r.f, f * r.d + d * r.f);
	}
	Dnum operator/(Dnum r) {
		return Dnum(f / r.f, (r.f * d - r.d * f) / r.f / r.f);
	}
};

// Elementary functions prepared for the chain rule as well
template<class T> Dnum<T> Exp(Dnum<T> g) { return Dnum<T>(expf(g.f), expf(g.f)*g.d); }
template<class T> Dnum<T> Sin(Dnum<T> g) { return  Dnum<T>(sinf(g.f), cosf(g.f)*g.d); }
template<class T> Dnum<T> Cos(Dnum<T>  g) { return  Dnum<T>(cosf(g.f), -sinf(g.f)*g.d); }
template<class T> Dnum<T> Tan(Dnum<T>  g) { return Sin(g) / Cos(g); }
template<class T> Dnum<T> Sinh(Dnum<T> g) { return  Dnum<T>(sinh(g.f), cosh(g.f)*g.d); }
template<class T> Dnum<T> Cosh(Dnum<T> g) { return  Dnum<T>(cosh(g.f), sinh(g.f)*g.d); }
template<class T> Dnum<T> Tanh(Dnum<T> g) { return Sinh(g) / Cosh(g); }
template<class T> Dnum<T> Log(Dnum<T> g) { return  Dnum<T>(logf(g.f), g.d / g.f); }
template<class T> Dnum<T> Pow(Dnum<T> g, float n) {
	return  Dnum<T>(powf(g.f, n), n * powf(g.f, n - 1) * g.d);
}

typedef Dnum<vec2> Dnum2;

const int tessellationLevel = 100;

const int windowWidth = 600, windowHeight = 600;

//---------------------------
struct Camera { // 3D camera
//---------------------------
	vec3 wEye, wLookat, wVup;   // extrinsic
	float fov, asp, fp, bp;		// intrinsic
public:
	Camera() {
		asp = (float)windowWidth / windowHeight;
		fov = 45.0f * (float)M_PI / 180.0f;
		fp = 1; bp = 20;
	}
	mat4 V() { return lookAt(wEye, wLookat, wVup); }
	mat4 P() { return perspective(fov, asp, fp, bp); }
};

//---------------------------
struct Material {
//---------------------------
	vec3 kd, ks, ka;
	float shininess;
};

//---------------------------
struct Light {
//---------------------------
	vec3 La, Le;
	vec4 wLightPos; // homogeneous coordinates, can be at ideal point
};

//---------------------------
struct RenderState {
//---------------------------
	mat4	           MVP, M, Minv, V, P;
	Material *         material;
	std::vector<Light> lights;
	Texture *          texture;
	vec3	           wEye;
};

//---------------------------
class Shader : public GPUProgram {
//---------------------------
public:
	virtual void Bind(RenderState state) = 0;

	void setUniformMaterial(const Material& material, const std::string& name) {
		setUniform(material.kd, name + ".kd");
		setUniform(material.ks, name + ".ks");
		setUniform(material.ka, name + ".ka");
		setUniform(material.shininess, name + ".shininess");
	}

	void setUniformLight(const Light& light, const std::string& name) {
		setUniform(light.La, name + ".La");
		setUniform(light.Le, name + ".Le");
		setUniform(light.wLightPos, name + ".wLightPos");
	}
};

//---------------------------
class PhongShader : public Shader {
//---------------------------
	const char * vertexSource = R"(
		#version 330 core
		precision highp float;

		struct Light {
			vec3 La, Le;
			vec4 wLightPos;
		};

		uniform mat4  MVP, M, Minv; // MVP, Model, Model-inverse
		uniform Light[8] lights;    // light sources
		uniform int   nLights;
		uniform vec3  wEye;         // pos of eye

		layout(location = 0) in vec3  vtxPos;            // pos in modeling space
		layout(location = 1) in vec3  vtxNorm;      	 // normal in modeling space
		layout(location = 2) in vec2  vtxUV;

		out vec3 wNormal;		    // normal in world space
		out vec3 wView;             // view in world space
		out vec3 wLight[8];		    // light dir in world space
		out vec2 texcoord;

		void main() {
			// gl_Position = vec4(vtxPos, 1) * MVP; // to NDC
			gl_Position = MVP * vec4(vtxPos, 1); // Saját
			// vectors for radiance computation
			// vec4 wPos = vec4(vtxPos, 1) * M;
			vec4 wPos = M * vec4(vtxPos, 1); // Saját
			for(int i = 0; i < nLights; i++) {
				wLight[i] = lights[i].wLightPos.xyz * wPos.w - wPos.xyz * lights[i].wLightPos.w;
			}
		    wView  = wEye * wPos.w - wPos.xyz;
		    // wNormal = (Minv * vec4(vtxNorm, 0) ).xyz;
		    wNormal = (vec4(vtxNorm, 0) * Minv).xyz; // Saját, nem biztos hogy jó
		    texcoord = vtxUV;
		}
	)";

	// fragment shader in GLSL
	const char * fragmentSource = R"(
		#version 330 core
		precision highp float;

		struct Light {
			vec3 La, Le;
			vec4 wLightPos;
		};

		struct Material {
			vec3 kd, ks, ka;
			float shininess;
		};

		uniform Material material;
		uniform Light[8] lights;    // light sources
		uniform int   nLights;
		uniform sampler2D diffuseTexture;
		uniform bool textured;


		uniform vec3 allTriangles[20];


		in  vec3 wNormal;       // interpolated world sp normal
		in  vec3 wView;         // interpolated world sp view
		in  vec3 wLight[8];     // interpolated world sp illum dir
		in  vec2 texcoord;

        out vec4 fragmentColor; // output goes to frame buffer

		void main() {
			vec3 N = normalize(wNormal);
			vec3 V = normalize(wView);
			if (dot(N, V) < 0) N = -N;	// prepare for one-sided surfaces like Mobius or Klein
			// Alapból ez, hogy amikor szorozzuk, csak átjöjjön a ka, kd
			vec3 texColor = vec3(1, 1, 1);
			if(textured){
				texColor = texture(diffuseTexture, texcoord).rgb;
			}
			vec3 ka = material.ka * texColor;
			vec3 kd = material.kd * texColor;

			// textúrázunk trükkösen
			// vec3 ka = material.ka;
			// vec3 kd = material.kd;

			vec3 radiance = vec3(0, 0, 0);
			// for(int i = 0; i < nLights; i++) {
			// 	vec3 L = normalize(wLight[i]);
			// 	vec3 H = normalize(L + V);
			// 	float cost = max(dot(N,L), 0), cosd = max(dot(N,H), 0);
			// 	// kd and ka are modulated by the texture
			// 	radiance += ka * lights[i].La +
   //                         (kd * cost + material.ks * pow(cosd, material.shininess)) * lights[i].Le;
			// }
			// Csak egy fényforrás van, egyelőre hardcodeolom a shaderbe
			// Iránya 1, 1, 1
			vec3 L = normalize(vec3(1, 1, 1));
			vec3 H = normalize(L + V);
			float cost = max(dot(N,L), 0), cosd = max(dot(N,H), 0);
			// Ambiens 0.4, 0.4, 0.4, rendes sugársűrűség 2, 2, 2
			radiance += ka * vec3(0.4, 0.4, 0.4) + (kd * cost + material.ks * pow(cosd, material.shininess)) * vec3(2, 2, 2);
			fragmentColor = vec4(radiance, 1);
		}
	)";
public:
	PhongShader() {
		create(vertexSource, fragmentSource);
	}

	void Bind(RenderState state) {
		Use(); 		// make this program run
		setUniform(state.MVP, "MVP");
		setUniform(state.M, "M");
		setUniform(state.Minv, "Minv");
		setUniform(state.wEye, "wEye");
		if (state.texture != nullptr){
			// ne legyen fura blend
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			(*state.texture, std::string("diffuseTexture"));
		}
		setUniformMaterial(*state.material, "material");

		// Fények beégetve
		// setUniform((int)state.lights.size(), "nLights");
		// for (unsigned int i = 0; i < state.lights.size(); i++) {
		// 	setUniformLight(state.lights[i], std::string("lights[") + std::to_string(i) + std::string("]"));
		// }
	}
};


std::vector<vec3> allTriangles;

struct VertexData {
	vec3 position, normal;
	vec2 texcoord;
};

//---------------------------
class ParamSurface : public Geometry<VertexData> {
//---------------------------
	unsigned int nVtxPerStrip, nStrips;
public:
	ParamSurface() { nVtxPerStrip = nStrips = 0; }

	virtual void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) = 0;

	VertexData GenVertexData(float u, float v) {
		VertexData vtxData;
		vtxData.texcoord = vec2(u, v);
		Dnum2 X, Y, Z;
		Dnum2 U(u, vec2(1, 0)), V(v, vec2(0, 1));
		eval(U, V, X, Y, Z);
		vtxData.position = vec3(X.f, Y.f, Z.f);
		vec3 drdU(X.d.x, Y.d.x, Z.d.x), drdV(X.d.y, Y.d.y, Z.d.y);
		vtxData.normal = cross(drdU, drdV);

		printf("Added pos: %f, %f, %f\n", vtxData.position.x, vtxData.position.y, vtxData.position.z);
		return vtxData;
	}

	void create(int N = tessellationLevel, int M = tessellationLevel) {
		nVtxPerStrip = (M + 1) * 2; // kifejezetten a for loopból kitalálva
		nStrips = N;
		std::vector<VertexData> vtxData;	// vertices on the CPU
		for (int i = 0; i < N; i++) {
			for (int j = 0; j <= M; j++) {
				vtxData.push_back(GenVertexData((float)j / M, (float)i / N));
				vtxData.push_back(GenVertexData((float)j / M, (float)(i + 1) / N));
			}
		}
		printf("#of points in a generated geometry: %d\n", nVtxPerStrip * nStrips);

		glBufferData(GL_ARRAY_BUFFER, nVtxPerStrip * nStrips * sizeof(VertexData), &vtxData[0], GL_STATIC_DRAW);
		// Enable the vertex attribute arrays
		glEnableVertexAttribArray(0);  // attribute array 0 = POSITION
		glEnableVertexAttribArray(1);  // attribute array 1 = NORMAL
		glEnableVertexAttribArray(2);  // attribute array 2 = TEXCOORD0
		// attribute array, components/attribute, component type, normalize?, stride, offset
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, position));
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, normal));
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, texcoord));
	}

	void Draw() {
		Bind();
		for (unsigned int i = 0; i < nStrips; i++) glDrawArrays(GL_TRIANGLE_STRIP, i *  nVtxPerStrip, nVtxPerStrip);
	}
};

//---------------------------
class Sphere : public ParamSurface {
//---------------------------
public:
	Sphere() { create(100, 100); }
	void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) {
		// TODO Itt miért van egy vessző?
		U = U * 2.0f * (float)M_PI; V = V * (float)M_PI;
		X = Cos(U) * Sin(V); Y = Sin(U) * Sin(V); Z = Cos(V);
	}
};

//---------------------------
class Cylinder : public ParamSurface {
//---------------------------

public:
	Cylinder() { create(1, 6); }
	void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) {
		U = U * 2.0f * M_PI, V = V * 2 - 1.0f;
		X = Cos(U); Z = Sin(U); Y = V;
	}
};

// 45 fokos, majd scale megoldja?
class Cone : public ParamSurface{

public:
	// Cone() { create(10, 60); }
	Cone() { create(1, 6); }
	void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) {
		// TODO kivinni mert konstans
		float angle = 0.2;
		float height = 2.0;
		float bottom_radius = tanf(angle) * height;
		U = U * 2.0f * M_PI, V = V;
		X = Cos(U) * (V + -1) * bottom_radius; Z = Sin(U) * (V + -1) * bottom_radius; Y = V - 1;
	}
};

class Square : public ParamSurface{
	// TODO lehet manuálisan?
public:
	Square() { create(1, 1); }
	void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) {
		U = U, V = V;
		X = U - 0.5; Z = V - 0.5; Y = 0;
	}
};

//---------------------------
struct Object3D {
//---------------------------
	Shader *   shader;
	Material * material;
	Texture *  texture;
	ParamSurface* geometry;
	vec3 scaling, translation, rotationAxis;
	float rotationAngle;
public:
	Object3D(Shader * _shader, Material * _material, Texture * _texture, ParamSurface* _geometry) :
		scaling(vec3(1, 1, 1)), translation(vec3(0, 0, 0)), rotationAxis(0, 0, 1), rotationAngle(0) {
		shader = _shader;
		texture = _texture;
		material = _material;
		geometry = _geometry;
	}

	virtual void SetModelingTransform(mat4& M, mat4& Minv) {
		M = translate(translation) * rotate(rotationAngle, rotationAxis) * scale(scaling);
		Minv = scale(vec3(1 / scaling.x, 1 / scaling.y, 1 / scaling.z)) * rotate(-rotationAngle, rotationAxis) * translate(-translation);
	}

	void Draw(RenderState state) {
		mat4 M, Minv;
		SetModelingTransform(M, Minv);
		state.M = M;
		state.Minv = Minv;
		state.MVP = state.P*state.V*state.M;
		state.material = material;
		state.texture = texture;
		if(texture != nullptr){
			shader->setUniform(true, "textured");
		}else {
			shader->setUniform(false, "textured");
		}
		shader->Bind(state);
		geometry->Draw();
	}

	virtual void Animate(float tstart, float tend) { rotationAngle = 0.8f * tend; }
};

struct ConeObj : Object3D{
	vec3 axis, tip;
	float angle = 0.2, height = 2;
	ConeObj(vec3 _tip, vec3 _axis, Shader * _shader, Material * _material, Texture * _texture, ParamSurface* _geometry) :
	Object3D(_shader, _material, _texture, _geometry){
		tip = _tip;
		axis = _axis;
	}
	void SetModelingTransform(mat4 &M, mat4 &Minv) override{
		// modell beállítása
		float sideScale = tanf(angle)*height;
		// M = scale(vec3(sideScale, 2, sideScale));

		rotationAngle = acosf(dot(normalize(axis), normalize(vec3(0, 1, 0))));
		// TODO ha nem megy, még egy normalize
		rotationAxis = normalize(cross(axis, vec3(0, 1, 0)));
		// scale nem is lesz
		// translation.x = tip.x * 1/sideScale;
		// translation.y = tip.y * 1/height;
		// translation.z = tip.z * 1/sideScale;
		M = translate(tip) * rotate(rotationAngle, rotationAxis) * scale(scaling);
		// TODO ez is kell!
		Minv = scale(vec3(1 / scaling.x, 1 / scaling.y, 1 / scaling.z)) * rotate(-rotationAngle, rotationAxis) * translate(-translation);
	}
};

//---------------------------
struct Scene {
//---------------------------
	std::vector<Object3D *> objects;
	Camera camera; // 3D camera
	std::vector<Light> lights;
	std::vector<vec3> texture;
public:
	void generateTexture(){
		vec3 blue = vec3(0, 0.1, 0.3);
		vec3 white = vec3(0.3, 0.3, 0.3);
		for(int i = 0; i < 20; i++){
			for(int j = 0; j < 20; j++){
				// TODO paritás ellenőrzés, jó-e?
				vec3 active = white;
				if((i + j) % 2 == 0){
					active = blue;
				}
				texture.push_back(active);
			}
		}
	}
	void Build() {
		// Shaders
		printf("Elotte\n");
		Shader * phongShader = new PhongShader();
		printf("Utana\n");

		// Materials
		Material * material0 = new Material;
		material0->kd = vec3(1, 1, 0);
		material0->ks = vec3(0, 0, 0);
		material0->ka = vec3(0.5f, 0.5f, 0);
		material0->shininess = 100;

		Material * texture_material = new Material;
		texture_material->kd = vec3(1, 1, 1);
		texture_material->ks = vec3(0, 0, 0);
		texture_material->ka = vec3(0, 0, 0);


		// Textures

		// Geometries

		ParamSurface* sphere = new Sphere();
		ParamSurface* cylinder = new Cylinder();
		ParamSurface* cone = new Cone();
		ParamSurface* square = new Square();

		Material * yellowPointer = new Material;
		yellowPointer->kd = vec3(1, 1, 0);
		yellowPointer->ks = vec3(0, 0, 0);
		yellowPointer->ka = vec3(0.5f, 0.5f, 0);
		yellowPointer->shininess = 100;

		Object3D * pointerSphere = new Object3D(phongShader, material0, nullptr, sphere);
		pointerSphere->translation = vec3(0, 1, 0.8);
		pointerSphere->scaling = vec3(1, 1, 1) * 0.01;
		objects.push_back(pointerSphere);

		// Object * testCylinder = new Object(phongShader, material0, nullptr, cylinder);
		// testCylinder->translation = vec3(0, 0, 0);
		// testCylinder->scaling = vec3(1, 1, 1);
		// objects.push_back(testCylinder);

		Material * cyanMaterial = new Material;
		cyanMaterial->kd = vec3(0.1, 0.2, 0.3);
		// TODO full fehér ha ezt berakom
		// cyanMaterial->ks = vec3(2, 2, 2);
		// "A rücskös anyagok ambiens visszaverődési tényezője a diffúz tényezőjének a háromszorosa."
		// ez ezt jelenti? vagy csak az ambienst kéne beállítani?
		// szerintem nem
		cyanMaterial->ka = cyanMaterial->kd*3;
		material0->shininess = 100;
		ConeObj * cyanCone = new ConeObj(vec3(0, 1, 0), vec3(-0.1, -1, -0.05)*-1, phongShader, cyanMaterial, nullptr, cone);
		cyanCone->translation = vec3(0, 0, 0);
		objects.push_back(cyanCone);

		Material * magentaMaterial = new Material;
		magentaMaterial->kd = vec3(0.3, 0, 0.2);
		// TODO full fehér ha ezt berakom
		// magentaMaterial->ks = vec3(2, 2, 2);
		// "A rücskös anyagok ambiens visszaverődési tényezője a diffúz tényezőjének a háromszorosa."
		// ez ezt jelenti? vagy csak az ambienst kéne beállítani?
		// szerintem nem
		magentaMaterial->ka = magentaMaterial->kd*3;
		material0->shininess = 20;
		ConeObj * magentaCone = new ConeObj(vec3(0, 1, 0.8), vec3(0.2, -1, 0)*-1, phongShader, magentaMaterial, nullptr, cone);
		magentaCone->translation = vec3(0, 0, 0);
		objects.push_back(magentaCone);

		Texture* kockas = new Texture(20, 20);
		generateTexture();
		kockas->updateTexture(20, 20, texture);
		Object3D * floor = new Object3D(phongShader, texture_material, kockas, square);
		floor->translation = vec3(0, -1, 0);
		// TODO jól beállítani a méretet
		floor->scaling = vec3(20, 1, 20);
		objects.push_back(floor);

		// Camera
		// EZ AZ OK
		// camera.wEye = vec3(0, 1, 4);
		printf("Rossz kamera");
		// nem lehet 0 wEye?
		camera.wEye = vec3(1, 2, -1);
		camera.wLookat = vec3(0, 0, 0);
		camera.wVup = vec3(0, 1, 0);

		// Fények beégetve
		// Lights
		// lights.resize(1);
		// lights[0].wLightPos = vec4(5, 5, 4, 0);	// ideal point -> directional light source
		// lights[0].La = vec3(0, 0, 0);
		// lights[0].Le = vec3(1, 1, 1);
	}

	void Render() {
		RenderState state;
		state.wEye = camera.wEye;
		state.V = camera.V();
		state.P = camera.P();
		state.lights = lights;
		for (Object3D * obj : objects) obj->Draw(state);
	}

	void Animate(float tstart, float tend) {
		float dt = tend-tstart;

		// TODO automatikus forgás a teszteléshez
		float r = 3.0;

		for (Object3D * obj : objects) obj->Animate(tstart, tend);
	}
};

class EngineApp : public glApp {
	Scene scene;
public:
	EngineApp() : glApp(3, 3, windowWidth, windowHeight, "3D Engine-ke") { }

	void onInitialization() {
		// TODO NEM KELL OP SYS SCALE
		glViewport(0, 0, windowWidth*OP_SYS_SCALE, windowHeight*OP_SYS_SCALE);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		scene.Build();
	}
	void onDisplay() {
		glClearColor(0.5, 0.5f, 0.5f, 1.0f);				// background color
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the screen
		scene.Render();
	}
	void onKeyboard(int key){
		if(key != 'a' && key != 'd') return;

		// LINUX DEBOUNCE
		static bool debounce = true;
		if(debounce){
			debounce = false;
			return;
		}
		debounce = true;

		printf("Pressed %c\n", key);

		float x = scene.camera.wEye.x;
		float z = scene.camera.wEye.z;
		float theta = atan2(z, x);
		if(key == 'a'){
			theta -= M_PI / 4;
		}
		if(key == 'd'){
			theta += M_PI / 4;
		}
		scene.camera.wEye.x = cos(theta) * 4.0;
		scene.camera.wEye.z = sin(theta) * 4.0;
		scene.camera.wEye.y = 1;

		vec3 campos = scene.camera.wEye;
		printf("Camera pos: %.2f, %.2f, %.2f", campos.x, campos.y, campos.z);
	}
	void onTimeElapsed(float tstart, float tend) {
		// scene.Animate(tstart, tend);
		refreshScreen();
	}
} app;
