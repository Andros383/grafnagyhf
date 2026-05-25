//=============================================================================================
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

#include "framework.h"

template<class T> struct Dnum {
	float f;

	T d;
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

struct Camera {
	vec3 wEye, wLookat, wVup;
	float fov, asp, fp, bp;
public:
	Camera() {
		asp = (float)windowWidth / windowHeight;
		fov = 45.0f * (float)M_PI / 180.0f;
		fp = 1; bp = 20;
	}
	mat4 V() { return lookAt(wEye, wLookat, wVup); }
	mat4 P() { return perspective(fov, asp, fp, bp); }
};

struct Material {
	vec3 kd, ks, ka;
	float shininess;
};

struct Light {
	vec3 La, Le;
	vec4 wLightPos;
};

struct Triangle{
	vec3 A, B, C;
};

std::vector<Triangle> allTriangles;
struct RenderState {
	mat4	           MVP, M, Minv, V, P;
	Material *         material;
	std::vector<Light> lights;
	Texture *          texture;
	vec3	           wEye;
};

class Shader : public GPUProgram {
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

	void setUniformTriangles(){
		for(unsigned int i = 0; i < allTriangles.size(); i++){
			Triangle t = allTriangles[i];
			setUniform(t.A, std::string("allTriangles[") + std::to_string(i) +std::string("].A"));
			setUniform(t.B, std::string("allTriangles[") + std::to_string(i) +std::string("].B"));
			setUniform(t.C, std::string("allTriangles[") + std::to_string(i) +std::string("].C"));
		}
	}
};

class PhongShader : public Shader {
	const char * vertexSource = R"(
		#version 330 core
		precision highp float;

		uniform mat4  MVP, M, Minv;
		uniform vec3  wEye;


		layout(location = 0) in vec3  vtxPos;
		layout(location = 1) in vec3  vtxNorm;
		layout(location = 2) in vec2  vtxUV;

		out vec3 wPosition;
		out vec3 wNormal;
		out vec3 wView;
		out vec2 texcoord;

		void main() {
			gl_Position = MVP * vec4(vtxPos, 1);
			vec4 wPos = M * vec4(vtxPos, 1);
		    wView  = wEye * wPos.w - wPos.xyz;
		    wNormal = (vec4(vtxNorm, 0) * Minv).xyz;
		    texcoord = vtxUV;
			wPosition = wPos.xyz;
		}
	)";

	const char * fragmentSource = R"(
		#version 330 core
		precision highp float;

		struct Material {
			vec3 kd, ks, ka;
			float shininess;
		};

		uniform Material material;
		uniform sampler2D diffuseTexture;
		uniform bool textured;

		struct Triangle{
			vec3 A, B, C;
		};
		uniform Triangle allTriangles[62];

		in  vec3 wPosition;
		in  vec3 wNormal;
		in  vec3 wView;
		in  vec2 texcoord;

        out vec4 fragmentColor;

		void main() {
			vec3 N = normalize(wNormal);
			vec3 V = normalize(wView);
			if (dot(N, V) < 0) N = -N;
			vec3 texColor = vec3(1, 1, 1);
			if(textured){
				texColor = texture(diffuseTexture, texcoord).rgb;
			}
			vec3 ka = material.ka * texColor;
			vec3 kd = material.kd * texColor;

			vec3 radiance = vec3(0, 0, 0);

			bool inShadow = false;
			for(int i = 0; i < 50; i++){
				vec3 r1 = allTriangles[i].A;
				vec3 r2 = allTriangles[i].B;
				vec3 r3 = allTriangles[i].C;
				vec3 shadowN = normalize(cross(r2 - r1, r3 - r1));
				vec3 start = wPosition;
				vec3 dir = normalize(vec3(1, 1, 1));

				if(dot(dir, shadowN) > 0){
					shadowN = -shadowN;
				}

				float D = dot(dir, shadowN);
				if(abs(D) <= 0.001) continue;

				float t = dot((r1-start), shadowN) / D;

				if(t<=0.01) continue;

				bool inside = true;

				vec3 p = start + dir*t;
				bool s1 = dot(cross(r2-r1, p-r1), shadowN) > 0.01;
				bool s2 = dot(cross(r3-r2, p-r2), shadowN) > 0.01;
				bool s3 = dot(cross(r1-r3, p-r3), shadowN) > 0.01;

				bool insidePos = s1 && s2 && s3;
				bool insideNeg = !s1 && !s2 && !s3;

				if(insidePos || insideNeg){
					inShadow = true;
					break;
				}
			}

			vec3 L = normalize(vec3(1, 1, 1));
			vec3 H = normalize(L + V);
			float cost = max(dot(N,L), 0), cosd = max(dot(N,H), 0);

			ka = kd*3;
			radiance += ka*vec3(0.4, 0.4, 0.4);
			if(!inShadow){
				radiance += (kd * cost + material.ks * pow(cosd, material.shininess)) * vec3(2, 2, 2);
			}
			fragmentColor = vec4(radiance, 1);
		}
	)";
public:
	PhongShader() {
		create(vertexSource, fragmentSource);
	}

	void Bind(RenderState state) {
		Use();
		setUniform(state.MVP, "MVP");
		setUniform(state.M, "M");
		setUniform(state.Minv, "Minv");
		setUniform(state.wEye, "wEye");
		if (state.texture != nullptr){
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
		setUniformMaterial(*state.material, "material");
		setUniformTriangles();
	}
};



struct VertexData {
	vec3 position, normal;
	vec2 texcoord;
};

class ParamSurface : public Geometry<VertexData> {
	unsigned int nVtxPerStrip, nStrips;
	std::vector<Triangle> modelTriangles;
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
		return vtxData;
	}

	void create(int N = tessellationLevel, int M = tessellationLevel) {
		nVtxPerStrip = (M + 1) * 2;
		nStrips = N;
		std::vector<VertexData> vtxData;
		for (int i = 0; i < N; i++) {
			for (int j = 0; j <= M; j++) {
				vtxData.push_back(GenVertexData((float)j / M, (float)i / N));
				vtxData.push_back(GenVertexData((float)j / M, (float)(i + 1) / N));
			}
		}

		for(size_t i = 2; i<vtxData.size(); i++){
			Triangle t;
			t.A = vtxData[i-2].position;
			t.B = vtxData[i-1].position;
			t.C = vtxData[i-0].position;
			bool degenerate = false;
			if(length(t.A-t.B) < 0.01) degenerate = true;
			if(length(t.C-t.B) < 0.01) degenerate = true;
			if(length(t.A-t.C) < 0.01) degenerate = true;
			if(!degenerate)
			modelTriangles.push_back(t);
		}

		glBufferData(GL_ARRAY_BUFFER, nVtxPerStrip * nStrips * sizeof(VertexData), &vtxData[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, position));
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, normal));
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, texcoord));
	}

	std::vector<Triangle> getModelTriangles(){
		return modelTriangles;
	}

	void Draw() {
		Bind();
		for (unsigned int i = 0; i < nStrips; i++) glDrawArrays(GL_TRIANGLE_STRIP, i *  nVtxPerStrip, nVtxPerStrip);
	}
};

class Cylinder : public ParamSurface {

public:
	Cylinder() { create(1, 6); }
	void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) {
		U = U * 2.0f * M_PI, V = V * 2;
		X = Cos(U) * 0.3; Z = Sin(U) * 0.3; Y = V;
	}
};

class Cone : public ParamSurface{

public:
	Cone() { create(1, 6); }
	void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) {
		float angle = 0.2;
		float height = 2.0;
		float bottom_radius = tanf(angle/2) * height;
		U = U * 2.0f * M_PI, V = V*2;
		X = Cos(U) * (V + -2) * bottom_radius; Z = Sin(U) * (V + -2) * bottom_radius; Y = V - 2;
	}
};

class Square : public ParamSurface{
public:
	Square() { create(1, 1); }
	void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) {
		U = U, V = V;
		X = U - 0.5; Z = V - 0.5; Y = 0;
	}
};

struct Object3D {
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
	std::vector<Triangle> getTriangles(){
		mat4 M, Minv;
		SetModelingTransform(M, Minv);

		std::vector<Triangle> transformedTriangles;
		for(Triangle t : geometry->getModelTriangles()){
			Triangle out;
			vec4 a = vec4(t.A.x, t.A.y, t.A.z, 1);
			vec4 at = M*a;
			out.A = vec3(at.x, at.y, at.z);

			vec4 b = vec4(t.B.x, t.B.y, t.B.z, 1);
			vec4 bt = M*b;
			out.B = vec3(bt.x, bt.y, bt.z);

			vec4 c = vec4(t.C.x, t.C.y, t.C.z, 1);
			vec4 ct = M*c;
			out.C = vec3(ct.x, ct.y, ct.z);

			transformedTriangles.push_back(out);
		}
		return transformedTriangles;
	}
};

struct ConeObj : Object3D{
	vec3 axis, tip;
	ConeObj(vec3 _tip, vec3 _axis, Shader * _shader, Material * _material, Texture * _texture, ParamSurface* _geometry) :
	Object3D(_shader, _material, _texture, _geometry){
		tip = _tip;
		axis = -_axis;
	}
	void SetModelingTransform(mat4 &M, mat4 &Minv) override{
		rotationAngle = -acosf(dot(normalize(axis), normalize(vec3(0, 1, 0))));
		rotationAxis = normalize(cross(axis, vec3(0, 1, 0)));
		M = translate(tip) * rotate(rotationAngle, rotationAxis) * scale(scaling);
		Minv = scale(vec3(1 / scaling.x, 1 / scaling.y, 1 / scaling.z)) * rotate(-rotationAngle, rotationAxis) * translate(-translation);
	}
};

struct Scene {
	std::vector<Object3D *> objects;
	Camera camera;
	std::vector<Light> lights;
	std::vector<vec3> texture;
public:
	void generateTexture(){
		vec3 blue = vec3(0, 0.1, 0.3);
		vec3 white = vec3(0.3, 0.3, 0.3);
		for(int i = 0; i < 20; i++){
			for(int j = 0; j < 20; j++){
				vec3 active = white;
				if((i + j) % 2 == 0){
					active = blue;
				}
				texture.push_back(active);
			}
		}
	}
	void Build() {
		Shader * phongShader = new PhongShader();

		ParamSurface* cylinder = new Cylinder();
		ParamSurface* cone = new Cone();
		ParamSurface* square = new Square();

		Material* firstCylinderMaterial = new Material;
		firstCylinderMaterial->kd = vec3(0.17, 0.35, 1.5);
		firstCylinderMaterial->ks = vec3(3.1, 2.7, 1.9);
		firstCylinderMaterial->shininess = 100;

		Object3D * firstCylinder = new Object3D(phongShader, firstCylinderMaterial, nullptr, cylinder);
		firstCylinder->translation = vec3(1, -1, 0);
		vec3 firstAxis = normalize(vec3(0.1, 1, 0));
		firstCylinder->rotationAngle = -acosf(dot(firstAxis, vec3(0, 1, 0)));
		firstCylinder->rotationAxis = normalize(cross(firstAxis, vec3(0, 1, 0)));

		Material* secondCylinderMaterial = new Material;
		secondCylinderMaterial->kd = vec3(1.3, 1.3, 1.3);
		secondCylinderMaterial->ks = vec3(0, 0, 0);
		secondCylinderMaterial->ka = secondCylinderMaterial->kd*3;
		secondCylinderMaterial->shininess = 1;

		Object3D * secondCylinder = new Object3D(phongShader, secondCylinderMaterial, nullptr, cylinder);
		secondCylinder->translation = vec3(0, -1, -0.8);
		vec3 secondAxis = normalize(vec3(-0.2, 1, -0.1));
		secondCylinder->rotationAngle = -acosf(dot(secondAxis, vec3(0, 1, 0)));
		secondCylinder->rotationAxis = normalize(cross(secondAxis, vec3(0, 1, 0)));

		Material* thirdCylinderMaterial = new Material;
		thirdCylinderMaterial->kd = vec3(0.3, 0.2, 0.1);
		thirdCylinderMaterial->ks = vec3(2, 2, 2);
		thirdCylinderMaterial->ka = thirdCylinderMaterial->kd*3;
		thirdCylinderMaterial->shininess = 100;

		Object3D * thirdCylinder = new Object3D(phongShader, thirdCylinderMaterial, nullptr, cylinder);
		thirdCylinder->translation = vec3(-1, -1, 0);
		vec3 thirdAxis = normalize(vec3(0, 1, 0.1));
		thirdCylinder->rotationAngle = -acosf(dot(thirdAxis, vec3(0, 1, 0)));
		thirdCylinder->rotationAxis = normalize(cross(thirdAxis, vec3(0, 1, 0)));

		Material * cyanMaterial = new Material;
		cyanMaterial->kd = vec3(0.1, 0.2, 0.3);
		cyanMaterial->ks = vec3(2, 2, 2);
		cyanMaterial->shininess = 100;
		ConeObj * cyanCone = new ConeObj(vec3(0, 1, 0), vec3(-0.1, -1, -0.05), phongShader, cyanMaterial, nullptr, cone);
		cyanCone->translation = vec3(0, 0, 0);

		Material * magentaMaterial = new Material;
		magentaMaterial->kd = vec3(0.3, 0, 0.2);
		magentaMaterial->ks = vec3(2, 2, 2);
		magentaMaterial->shininess = 20;
		ConeObj * magentaCone = new ConeObj(vec3(0, 1, 0.8), vec3(0.2, -1, 0), phongShader, magentaMaterial, nullptr, cone);
		magentaCone->translation = vec3(0, 0, 0);

		Material * texture_material = new Material;
		texture_material->kd = vec3(1, 1, 1);
		texture_material->ks = vec3(0, 0, 0);
		texture_material->ka = vec3(0, 0, 0);

		Texture* kockas = new Texture(20, 20);
		generateTexture();
		kockas->updateTexture(20, 20, texture);
		Object3D * floor = new Object3D(phongShader, texture_material, kockas, square);
		floor->translation = vec3(0, -1, 0);
		floor->scaling = vec3(20, 1, 20);

		objects.push_back(firstCylinder);
		objects.push_back(secondCylinder);
		objects.push_back(thirdCylinder);
		objects.push_back(cyanCone);
		objects.push_back(magentaCone);
		objects.push_back(floor);

		for(Object3D* o : objects){
			auto trias = o->getTriangles();
			for(Triangle t : trias){
				allTriangles.push_back(t);
			}
		}

		camera.wEye = vec3(0, 1, 4);
		camera.wLookat = vec3(0, 0, 0);
		camera.wVup = vec3(0, 1, 0);
	}

	void Render() {
		RenderState state;
		state.wEye = camera.wEye;
		state.V = camera.V();
		state.P = camera.P();
		state.lights = lights;
		for (Object3D * obj : objects) obj->Draw(state);
	}
};

class EngineApp : public glApp {
	Scene scene;
public:
	EngineApp() : glApp(3, 3, windowWidth, windowHeight, "3D Engine-ke") { }

	void onInitialization() {
		glViewport(0, 0, windowWidth, windowHeight);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);
		scene.Build();
	}
	void onDisplay() {
		glClearColor(0.5, 0.5f, 0.5f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		scene.Render();
	}
	void onKeyboard(int key){
		if(key != 'a' && key != 'd') return;

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

		refreshScreen();
	}
} app;
