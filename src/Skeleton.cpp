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

// Nagyhf

#include "framework.h"
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <regex>
#include <stdexcept>
#include <type_traits>

struct Scene;
const int OP_SYS_SCALE = 2;

float random_float(float min, float max){
	return min + ((float)rand() / (float)RAND_MAX) *(max - min);
}

void printvec(vec3 p, std::string name = ""){
	printf("%s : (%.2f, %.2f, %.2f\n", name.c_str(), p.x, p.y , p.z);
}

struct Ray{
	vec3 start;
	vec3 dir;
};

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

const int windowWidth = 900, windowHeight = 900;

struct Camera {
	vec3 wEye, wLookat, wVup;
	float fov, asp, fp, bp;
public:
	Camera() {
		asp = (float)windowWidth / windowHeight;
		fov = 45.0f * (float)M_PI / 180.0f;
		// TODO lehet túl közel van a backplane
		fp = 1; bp = 300;
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
			vec3 kd = material.kd * texColor;

			vec3 radiance = vec3(0, 0, 0);

			vec3 L = normalize(vec3(1, 1, 1));
			vec3 H = normalize(L + V);
			float cost = max(dot(N,L), 0), cosd = max(dot(N,H), 0);

			// Bennemarad, mint diffúz színezése
			vec3 ka = kd*3;
			radiance += ka*vec3(0.4, 0.4, 0.4);
			radiance += (kd * cost + material.ks * pow(cosd, material.shininess)) * vec3(2, 2, 2);
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

class Sphere : public ParamSurface {
public:
	Sphere() { create(); }
	void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) {
		U = U * 2.0f * (float)M_PI, V = V * (float)M_PI;
		X = Cos(U) * Sin(V); Y = Sin(U) * Sin(V); Z = Cos(V);
	}
};

class Cylinder : public ParamSurface {
public:
	Cylinder() { create(); }
	void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) {
		U = U * 2.0f * M_PI, V = V;
		X = Cos(U); Z = Sin(U); Y = V;
	}
};

class Cone : public ParamSurface{

public:
	Cone() { create(); }
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
	Shader* shader;
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
};

struct GameObject{
	vec3 pos, vel, accel;
	bool alive = true;
	// Lehet nem is kell, mert nem hatnak nagyon egymásra az objektumok
	// TODO kell-e pontosabb a parabolapályához?
	virtual void Control(float tstart, float tend, Scene* scene){
	}
	// szerintem minden ugyan így lesz animálva
	// valahol a Control-ba szerintem kéne a megölése
	virtual void Animate(float tstart, float tend){
		float dt = tend - tstart;
		// általános dolog
		pos += vel*dt;
		vel += accel*dt;
	};

	virtual void Draw(RenderState) = 0;

	virtual ~GameObject() = default;
};

struct Cannonball : GameObject{
	Material* material;
	ParamSurface* surface;
	Object3D* object;

	float radius = 0.25;
	Cannonball(Shader* _shader){
		Material* material = new Material();
		material->kd = vec3(0.01, 0.01, 0.01);
		material->ks = vec3(1, 1, 1);
		material->shininess = 100;

		ParamSurface* surface = new Sphere();

		object = new Object3D(_shader, material, nullptr, surface);
		object->scaling = vec3(1, 1, 1) * radius;
	}
	void Control(float tstart, float tend, Scene* scene) override{
		if(this->pos.y < -1) alive = false;
	}
	void Draw(RenderState renderState) override{
		object->translation = pos;
		object->Draw(renderState);
	}
	~Cannonball() override{
		delete object;
	}

	float intersect(const Ray& ray){
		// Előadásról
		// nem kell a material, ilyesmi, csak hogy ütközik-e az időn belül
		// Az idő Dt és a lézer hosszától függ
		vec3 dist = ray.start - pos;
		float a = dot(ray.dir, ray.dir);
		float b = dot(dist, ray.dir) * 2;
		float c = dot(dist, dist) - radius * radius;
		float discr = b * b - 4 * a * c;

		if (discr < 0){
			return -1;
		}
		else {
			discr = sqrtf(discr);
		}

		float t1 = (-b + discr)/2/a, t2 = (-b - discr)/2/a;
		// Ez a speciális eset nálam is áll, mert tetszőleges Ray ütközik
		if (t1 <= 0) return -1; // t1 >= t2 for sure in this special case

		// a lézer modellje csak előre néz a sugárhoz képest
		float t = (t2 > 0) ? t2 : t1;

		return t;
	}
};

struct Floor : GameObject{
	Object3D* object;
	Floor(Shader* _shader){
		Material* material = new Material();
		material->kd = vec3(0, 1, 0);
		// material->kd = vec3(0.2, 0.2, 0.2);
		material->ka = vec3(0.1, 0.1, 0.1);

		ParamSurface* surface = new Square();


		std::vector<vec3> texture;
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

		Texture* kockas = new Texture(20, 20);
		kockas->updateTexture(20, 20, texture);

		object = new Object3D(_shader, material, kockas, surface);

		object->scaling = vec3(50, 1, 50);
		pos = vec3(0, -1, 0);
	}
	void Draw(RenderState renderState) override{
		object->translation = pos;
		object->Draw(renderState);
	}

	~Floor() override{
		delete object;
	}
};

struct Cannon : GameObject{
	static float MAX_SHOOT_TIME;
	static float MIN_SHOOT_TIME;
	static float MIN_SHOOT_SPEED;
	static float MAX_SHOOT_SPEED;

	Scene* scene;
	Object3D* barrel;
	Object3D* body;

	vec3 rotationDir = vec3(0, 1, 0);
	float nextShoot = 1.5;
	Cannon(Shader* _shader, Scene* _scene){
		scene = _scene;

		Material* material = new Material();
		material->kd = vec3(0.1, 0.1, 0.1);
		material->ks = material->kd * 2;
		material->ka = material->kd * 3;
		material->shininess = 100;

		ParamSurface* body_surface = new Sphere();

		body = new Object3D(_shader, material, nullptr, body_surface);
		body->scaling = vec3(0.5, 0.5, 0.5);

		ParamSurface* barrel_surface = new Cylinder();
		barrel = new Object3D(_shader, material, nullptr, barrel_surface);
		barrel->scaling = vec3(0.3, 1.5, 0.3);
	}
	void Draw(RenderState renderState) override{
		body->translation = pos;
		barrel->translation = pos;

		vec3 up = vec3(0, 1, 0);

		barrel->rotationAngle = acosf(dot(normalize(up), normalize(rotationDir)));
		if(abs(barrel->rotationAngle)>0.05f){
			// Ha kicsi, ne forgassuk, fajul a cross product
			barrel->rotationAxis = cross(normalize(up), normalize(rotationDir));
		}

		body->Draw(renderState);
		barrel->Draw(renderState);
	}


	void Control(float tstart, float tend, Scene* scene) override{
		float dt = tend-tstart;
		nextShoot -= dt;
		if(nextShoot > 0) return;

		nextShoot = random_float(MIN_SHOOT_TIME, MAX_SHOOT_TIME);

		Shoot();
	}

	void Shoot();

	~Cannon() override{
		delete body;
	}
};
float Cannon::MIN_SHOOT_TIME = 2.0;
float Cannon::MAX_SHOOT_TIME = 10.0;
float Cannon::MIN_SHOOT_SPEED = 10.0;
float Cannon::MAX_SHOOT_SPEED = 30.0;

struct Laser : GameObject{
	static float LASER_LENGTH;
	static float LASER_SPEED;

	Material* material;
	ParamSurface* surface;
	Object3D* object;
	Laser(Shader* _shader){
		Material* material = new Material();
		material->kd = vec3(1, 0.01, 0.01);

		ParamSurface* surface = new Cylinder();

		object = new Object3D(_shader, material, nullptr, surface);
		object->scaling = vec3(0.025, LASER_LENGTH, 0.025);
	}
	void Control(float tstart, float tend, Scene* scene) override;
	void Draw(RenderState renderState) override{
		object->translation = pos;

		vec3 up = vec3(0, 1, 0);

		object->rotationAngle = acosf(dot(normalize(up), normalize(vel)));
		if(abs(object->rotationAngle)>0.05f){
			// Ha kicsi, ne forgassuk, fajul a cross product
			object->rotationAxis = cross(normalize(up), normalize(vel));
		}

		object->Draw(renderState);
	}
	Ray getRay(){
		Ray r;
		r.dir = vel;
		r.start = pos;
		return r;
	}
	~Laser() override{
		delete object;
	}
};

float Laser::LASER_LENGTH = 1.0;
float Laser::LASER_SPEED = 30.0;

struct Scene {
	static float GRAVITY;

	Shader* phongShader;
	int current = 0;
	std::vector<GameObject *> objects[2];
	Camera camera;
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
	void testBuild(){
		GameObject* laser = new Laser(phongShader);
		laser->vel = vec3(0, 1, 0);
		Join(laser);

		GameObject* ball = new Cannonball(phongShader);
		ball->pos = vec3(0, 2, 0);
		ball->accel = vec3(0, 0, 0);
		Join(ball);
	}

	void Build() {
		phongShader = new PhongShader();
		camera.wEye = vec3(0, 10, 50);
		camera.wLookat = vec3(0, 10, 0);
		camera.wVup = vec3(0, 1, 0);

		// camera.wEye = vec3(0, 1, 10);
		// camera.wLookat = vec3(0, 0, 0);
		// testBuild();
		// return;

		GameObject* ball = new Cannonball(phongShader);
		GameObject* floor = new Floor(phongShader);

		for(int i = 0; i<10; i++){
			GameObject* cannon = new Cannon(phongShader, this);
			cannon->pos = vec3(-20, 0, i*2 - 9.5);
			Join(cannon);
		}

		ball->accel = vec3(0, -1, 0);
		ball->vel = vec3(-1, 3, 0);
		ball->pos = vec3(10, 5, 0);
		Join(ball);
		Join(floor);

	}
	void Join(GameObject* o){
		objects[1-current].push_back(o);
	}
	void Render() {
		RenderState state;
		state.wEye = camera.wEye;
		state.V = camera.V();
		state.P = camera.P();
		for (GameObject * obj : objects[current]) obj->Draw(state);
	}
	void Simulate(float tstart, float tend){
		const float dt = 0.05f; // dt kicsi
		for (float t = tstart; t < tend; t += dt) {
			float Dt = fmin(dt, tend - t);
			for (auto * obj : objects[current]){
				obj->Control(t, t + Dt, this);
			}
			for (auto * obj : objects[current]) {
				if (obj->alive) Join(obj); // élők az új tömbbe
				else{
					delete obj; // nem élők törlése
				}
			}
			objects[current].clear();
			current = 1 - current; // ping-pong
			for (auto * obj : objects[current]) obj->Animate(t, t + Dt);
		}
	}
	void addCannonball(vec3 startpos, vec3 startvel){
		GameObject* cannonball = new Cannonball(phongShader);
		cannonball->pos = startpos;
		cannonball->vel = startvel;
		cannonball->accel = vec3(0, -GRAVITY, 0);
		Join(cannonball);
	}

	void ShootLaserAt(Cannonball* ball){
		// bináris kereséssel ütközési időpont számítása
		float left = 0;
		// Ennyi idő alatt el kéne találni
		float right = 10;
		float T;
		int iter_count = 0;
		while (iter_count++ < 10) {
			T = (left+right) / 2;
			vec3 posAtT = ball->pos + ball->vel * T + ball->accel*T*T/2.0;
			// Ha eléri, akkor kevesebb idő alatt is eléri?
			// Tudni, hogy 0, 0, 0-ból indul a lézer
			if(length(posAtT) / Laser::LASER_SPEED < T){
				right = T;
			}else {
				// Ha nem éri el, akkor több idő kell
				left = T;
			}
		}
		// Végére 10 / 2^10 pontosságú lesz, mivel középpontra céloz, feltehető hogy el fogja találni

		vec3 posAtT = ball->pos + ball->vel * T + ball->accel*T*T/2.0;
		vec3 laser_dir = normalize(posAtT);

		GameObject* laser = new Laser(phongShader);
		laser->pos = vec3(0, 0, 0);
		laser->vel = laser_dir*Laser::LASER_SPEED;
		Join(laser);
	}
	void ShootLaser(){
		// Most mindenre lő
		for(GameObject* o : objects[current]){
			if(Cannonball* ball = dynamic_cast<Cannonball*>(o)){
				ShootLaserAt(ball);
			}
		}
	}

	std::vector<GameObject *> getGameObjects(){
		return objects[current];
	}
};
float Scene::GRAVITY = 10.0;
void Cannon::Shoot(){
		float speed = random_float(Cannon::MIN_SHOOT_SPEED, Cannon::MAX_SHOOT_SPEED);
		float horiz_angle = random_float(-M_PI / 8.0, M_PI / 8.0);
		float vert_angle = random_float(M_PI_4, M_PI_2 - M_PI / 8.0);

		vec3 shoot_dir;
		shoot_dir.x = cosf(horiz_angle) * cosf(vert_angle);
		shoot_dir.y = sinf(vert_angle);
		shoot_dir.z = sinf(horiz_angle) * cosf(vert_angle);

		rotationDir = shoot_dir;
		printvec(shoot_dir, std::string("shoot_dir"));

		scene->addCannonball(pos, shoot_dir * speed);
	}

void Laser::Control(float tstart, float tend, Scene* scene) {
	float dt = tend-tstart;
	std::vector<GameObject *> objects = scene->getGameObjects();
	Ray r = this->getRay();
	for(GameObject* o : objects){
		if(Cannonball* ball = dynamic_cast<Cannonball*>(o)){

			float t = ball->intersect(r);
			if( t > 0 && t < LASER_LENGTH + dt*LASER_SPEED){
				ball->alive = false;
				alive = false;
			}
		}
	}

	if(length(pos) > 100) alive = false;
}

class EngineApp : public glApp {
	Scene scene;
public:
	EngineApp() : glApp(3, 3, windowWidth, windowHeight, "3D Engine-ke") { }

	void onInitialization() {
		glViewport(0, 0, windowWidth*OP_SYS_SCALE, windowHeight*OP_SYS_SCALE);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		srand(time(nullptr));

		scene.Build();

		scene.ShootLaser();
	}
	void onDisplay() {
		glClearColor(0.529, 0.808, 0.922, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		scene.Render();
	}
	void onKeyboard(int key){
		static bool debounce = true;
		if(debounce){
			debounce = false;
			return;
		}
		debounce = true;

		if(key == 'l'){
			scene.ShootLaser();
		}

		if(key != 'a' && key != 'd') return;


		float x = scene.camera.wEye.x;
		float z = scene.camera.wEye.z;
		float radius = std::sqrt(x*x + z*z);

		float theta = atan2(z, x);
		if(key == 'a'){
			theta += M_PI / 32;
		}
		if(key == 'd'){
			theta -= M_PI / 32;
		}
		scene.camera.wEye.x = cos(theta) * radius;
		scene.camera.wEye.z = sin(theta) * radius;

		refreshScreen();
	}
	void onTimeElapsed(float startTime, float endTime){
		scene.Simulate(startTime, endTime);
		refreshScreen();
	}
} app;
