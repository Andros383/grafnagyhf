// Nev    : Falucskai Andras
// Neptun : A2PT9J

// Nagyhf
// Vannak felesleges programsorok és kommentek, ékezetek is

#include "framework.h"
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <regex>
#include <stdexcept>
#include <type_traits>

int balls_shot = 0;
int lasers_shot = 0;

struct Scene;
const int OP_SYS_SCALE = 2;

float random_float(float min, float max) {
	return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

void printvec(vec3 p, std::string name = "") {
	printf("%s : (%.2f, %.2f, %.2f\n", name.c_str(), p.x, p.y, p.z);
}

struct Ray {
	vec3 start;
	vec3 dir;
};

template <class T>
struct Dnum {
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

template <class T>
Dnum<T> Exp(Dnum<T> g) { return Dnum<T>(expf(g.f), expf(g.f) * g.d); }
template <class T>
Dnum<T> Sin(Dnum<T> g) { return Dnum<T>(sinf(g.f), cosf(g.f) * g.d); }
template <class T>
Dnum<T> Cos(Dnum<T> g) { return Dnum<T>(cosf(g.f), -sinf(g.f) * g.d); }
template <class T>
Dnum<T> Tan(Dnum<T> g) { return Sin(g) / Cos(g); }
template <class T>
Dnum<T> Sinh(Dnum<T> g) { return Dnum<T>(sinh(g.f), cosh(g.f) * g.d); }
template <class T>
Dnum<T> Cosh(Dnum<T> g) { return Dnum<T>(cosh(g.f), sinh(g.f) * g.d); }
template <class T>
Dnum<T> Tanh(Dnum<T> g) { return Sinh(g) / Cosh(g); }
template <class T>
Dnum<T> Log(Dnum<T> g) { return Dnum<T>(logf(g.f), g.d / g.f); }
template <class T>
Dnum<T> Pow(Dnum<T> g, float n) {
	return Dnum<T>(powf(g.f, n), n * powf(g.f, n - 1) * g.d);
}

typedef Dnum<vec2> Dnum2;

const int tessellationLevel = 100;

const int windowWidth = 900, windowHeight = 900;

struct Camera {
	static constexpr float FOV = 60.0;

	vec3 wEye, wLookat, wVup;
	float fov, asp, fp, bp;

  public:
	Camera() {
		asp = (float)windowWidth / windowHeight;
		fov = FOV * (float)M_PI / 180.0f;
		// TODO lehet túl közel van a backplane
		fp = 1;
		bp = 300;
	}
	mat4 V() { return lookAt(wEye, wLookat, wVup); }
	mat4 P() { return perspective(fov, asp, fp, bp); }
	vec3 cameraDirFromPx(int X, int Y) {
		vec3 eye = wEye;
		vec3 look_dir = normalize(wLookat - wEye);
		vec3 up = normalize(wVup);
		vec3 right = normalize(cross(look_dir, up));

		// feltételezve az 1:1 képarányt
		float fovrad = FOV / 180.0 * M_PI;
		float FOVmult = tanf(fovrad / 2);

		float rightcoeff = (((float)X + 0.5) / ((float)windowWidth / 2) - 1) * FOVmult;
		Y = windowHeight - Y;
		float upcoeff = (((float)Y + 0.5) / ((float)windowHeight / 2) - 1) * FOVmult;

		vec3 dir = look_dir + right * rightcoeff + up * upcoeff;

		return normalize(dir);
	}
};

struct Material {
	vec3 kd, ks, ka;
	float shininess;
};

struct Light {
	vec3 La, Le;
	vec4 wLightPos;
};

struct Triangle {
	vec3 A, B, C;
};

std::vector<Triangle> allTriangles;
struct RenderState {
	mat4 MVP, M, Minv, V, P;
	Material* material;
	Texture* texture;
	vec3 wEye;
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
	const char* vertexSource = R"(
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

	const char* fragmentSource = R"(
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

			radiance += material.ka*texColor*vec3(0.4, 0.4, 0.4);
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
		if (state.texture != nullptr) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			// Mindig 0-ra bindol?
			state.texture->Bind(0);
		}
		setUniformMaterial(*state.material, "material");
	}
};

class VolumetricShader : public GPUProgram {
	const char* volVertSrc = R"(
		#version 330 core
		uniform mat4 VP;
		uniform int viewportHeight;
		layout(location = 0) in vec3 wPos;
		layout(location = 1) in vec4 color;
		layout(location = 2) in float size;
		layout(location = 3) in int textureIdx;
		out vec4 modulation;
		out float textureIndex;

		void main() {
			gl_Position = VP * vec4(wPos, 1.0f);
			gl_PointSize = size * viewportHeight / gl_Position.w;
			modulation = color;
			textureIndex = textureIdx;
		}
)";

	const char* volFragSrc = R"(
		#version 330 core
		precision highp float;

		uniform sampler2D textureMap[1];
		in vec4 modulation;
		in float textureIndex;
		out vec4 outColor;

		void main() {
			// manuális színezés itt, mert nem tudom hogyan kell alfát átvinni
			// textúra feltöltés amit kézzel csinálok csak vec3-at támogat
			// gondolom lodepng-ben lenne, csak nem bogozom ki
			outColor = texture(textureMap[int(0)], gl_PointCoord.xy) * modulation;
			// tényleges szín fekete, + az alapján, hogy mennyire fehér az átlátszóság
			outColor = vec4(0, 0, 0, 1-outColor.x);
		}
	)";

  public:
	VolumetricShader() {
		create(volVertSrc, volFragSrc);
	}
	void Bind(RenderState state) {
		Use();
		setUniform(state.P * state.V, "VP");
		setUniform(windowHeight, "viewportHeight");
		glEnable(GL_PROGRAM_POINT_SIZE);
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

		for (size_t i = 2; i < vtxData.size(); i++) {
			Triangle t;
			t.A = vtxData[i - 2].position;
			t.B = vtxData[i - 1].position;
			t.C = vtxData[i - 0].position;
			bool degenerate = false;
			if (length(t.A - t.B) < 0.01)
				degenerate = true;
			if (length(t.C - t.B) < 0.01)
				degenerate = true;
			if (length(t.A - t.C) < 0.01)
				degenerate = true;
			if (!degenerate)
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

	std::vector<Triangle> getModelTriangles() {
		return modelTriangles;
	}

	void Draw() {
		Bind();
		for (unsigned int i = 0; i < nStrips; i++)
			glDrawArrays(GL_TRIANGLE_STRIP, i * nVtxPerStrip, nVtxPerStrip);
	}
};

class Sphere : public ParamSurface {
  public:
	Sphere() { create(); }
	void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) {
		U = U * 2.0f * (float)M_PI, V = V * (float)M_PI;
		X = Cos(U) * Sin(V);
		Y = Sin(U) * Sin(V);
		Z = Cos(V);
	}
};

class Cylinder : public ParamSurface {
  public:
	Cylinder() { create(); }
	void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) {
		U = U * 2.0f * M_PI, V = V;
		X = Cos(U);
		Z = Sin(U);
		Y = V;
	}
};

class Cone : public ParamSurface {

  public:
	Cone() { create(); }
	void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) {
		float angle = 0.2;
		float height = 2.0;
		float bottom_radius = tanf(angle / 2) * height;
		U = U * 2.0f * M_PI, V = V * 2;
		X = Cos(U) * (V + -2) * bottom_radius;
		Z = Sin(U) * (V + -2) * bottom_radius;
		Y = V - 2;
	}
};

class Square : public ParamSurface {
  public:
	Square() { create(1, 1); }
	void eval(Dnum2& U, Dnum2& V, Dnum2& X, Dnum2& Y, Dnum2& Z) {
		U = U, V = V;
		X = U - 0.5;
		Z = V - 0.5;
		Y = 0;
	}
};

struct Object3D {
	Shader* shader;
	Material* material;
	Texture* texture;
	ParamSurface* geometry;
	vec3 scaling, translation, rotationAxis;
	float rotationAngle;

  public:
	Object3D(Shader* _shader, Material* _material, Texture* _texture, ParamSurface* _geometry) : scaling(vec3(1, 1, 1)), translation(vec3(0, 0, 0)), rotationAxis(0, 0, 1), rotationAngle(0) {
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
		state.MVP = state.P * state.V * state.M;
		state.material = material;
		state.texture = texture;
		if (texture != nullptr) {
			shader->setUniform(true, "textured");
		} else {
			shader->setUniform(false, "textured");
		}
		shader->Bind(state);
		geometry->Draw();
	}
};

struct GameObject {
	vec3 pos, vel, accel;
	bool alive = true;
	// Lehet nem is kell, mert nem hatnak nagyon egymásra az objektumok
	// TODO kell-e pontosabb a parabolapályához?
	virtual void Control(float tstart, float tend, Scene* scene) {
	}
	// szerintem minden ugyan így lesz animálva
	// valahol a Control-ba szerintem kéne a megölése
	virtual void Animate(float tstart, float tend) {
		float dt = tend - tstart;
		// általános dolog
		pos += vel * dt;
		vel += accel * dt;
	};

	virtual void Draw(RenderState) = 0;

	virtual ~GameObject() = default;
};

struct Cannonball : GameObject {
	static bool autoShootingEnabled;
	static float AUTO_SHOOT_AT_START;

	static Material* material;
	static ParamSurface* surface;
	Object3D* object;

	bool shotAt = false;
	float autoShootAt = AUTO_SHOOT_AT_START;

	float radius = 0.25;
	Cannonball(Shader* _shader) {
		if(material == nullptr){
			material = new Material();
			material->kd = vec3(0.01, 0.01, 0.01);
			material->ks = vec3(1, 1, 1);
			material->shininess = 100;
		}
		if(surface == nullptr){
			surface = new Sphere();
		}

		object = new Object3D(_shader, material, nullptr, surface);
		object->scaling = vec3(1, 1, 1) * radius;
	}
	void Control(float tstart, float tend, Scene* scene) override;
	void Draw(RenderState renderState) override {
		object->translation = pos;
		object->Draw(renderState);
	}
	~Cannonball() override {
		delete object;
	}

	float intersect(const Ray& ray, float radius_mult = 1) {
		// Előadásról
		// nem kell a material, ilyesmi, csak hogy ütközik-e az időn belül
		// Az idő Dt és a lézer hosszától függ
		float newrad = radius_mult * radius_mult;
		vec3 dist = ray.start - pos;
		float a = dot(ray.dir, ray.dir);
		float b = dot(dist, ray.dir) * 2;
		float c = dot(dist, dist) - newrad * newrad;
		float discr = b * b - 4 * a * c;

		if (discr < 0) {
			return -1;
		} else {
			discr = sqrtf(discr);
		}

		float t1 = (-b + discr) / 2 / a, t2 = (-b - discr) / 2 / a;
		// Ez a speciális eset nálam is áll, mert tetszőleges Ray ütközik
		if (t1 <= 0)
			return -1; // t1 >= t2 for sure in this special case

		// a lézer modellje csak előre néz a sugárhoz képest
		float t = (t2 > 0) ? t2 : t1;

		return t;
	}
};
bool Cannonball::autoShootingEnabled = false;
float Cannonball::AUTO_SHOOT_AT_START = 75;

Material* Cannonball::material = nullptr;
ParamSurface* Cannonball::surface = nullptr;

struct Floor : GameObject {
	Object3D* object;
	Floor(Shader* _shader) {
		// Ebből csak egy van, nem fogom staticozni
		Material* material = new Material();
		material->kd = vec3(0, 1, 0);
		material->ka = material->kd * 3;
		ParamSurface* surface = new Square();

		std::vector<vec3> texture;
		vec3 blue = vec3(0, 0.1, 0.3);
		vec3 white = vec3(0.3, 0.3, 0.3);
		for (int i = 0; i < 20; i++) {
			for (int j = 0; j < 20; j++) {
				vec3 active = white;
				if ((i + j) % 2 == 0) {
					active = blue;
				}
				texture.push_back(active);
			}
		}

		Texture* kockas = new Texture(20, 20);
		kockas->updateTexture(20, 20, texture);

		object = new Object3D(_shader, material, kockas, surface);

		object->scaling = vec3(100, 1, 100);
		pos = vec3(0, -1, 0);
	}
	void Draw(RenderState renderState) override {
		object->translation = pos;
		object->Draw(renderState);
	}

	~Floor() override {
		delete object;
	}
};

struct Cannon : GameObject {
	static float MAX_SHOOT_TIME;
	static float MIN_SHOOT_TIME;
	static float MIN_SHOOT_SPEED;
	static float MAX_SHOOT_SPEED;

	static Material* material;
	static ParamSurface* bodySurface;
	static ParamSurface* barrelSurface;

	Scene* scene;
	Object3D* barrel;
	Object3D* body;

	vec3 rotationDir = vec3(0, 1, 0);
	float nextShoot = 0;
	Cannon(Shader* _shader, Scene* _scene) {
		scene = _scene;

		if(material == nullptr){
			material = new Material();
			material->kd = vec3(0.1, 0.1, 0.1);
			material->ks = material->kd * 2;
			material->ka = material->kd * 3;
			material->shininess = 100;
		}

		if(bodySurface == nullptr){
			bodySurface = new Sphere();
		}

		if(barrelSurface == nullptr){
			barrelSurface = new Cylinder();
		}


		body = new Object3D(_shader, material, nullptr, bodySurface);
		body->scaling = vec3(0.5, 0.5, 0.5);

		barrel = new Object3D(_shader, material, nullptr, barrelSurface);
		barrel->scaling = vec3(0.3, 1.5, 0.3);
	}
	void Draw(RenderState renderState) override {
		body->translation = pos;
		barrel->translation = pos;

		vec3 up = vec3(0, 1, 0);

		barrel->rotationAngle = acosf(dot(normalize(up), normalize(rotationDir)));
		if (abs(barrel->rotationAngle) > 0.05f) {
			// Ha kicsi, ne forgassuk, fajul a cross product
			barrel->rotationAxis = cross(normalize(up), normalize(rotationDir));
		}

		body->Draw(renderState);
		barrel->Draw(renderState);
	}

	void Control(float tstart, float tend, Scene* scene) override {
		float dt = tend - tstart;
		nextShoot -= dt;
		if (nextShoot > 0)
			return;

		nextShoot = random_float(MIN_SHOOT_TIME, MAX_SHOOT_TIME);

		Shoot();
	}

	void Shoot();

	~Cannon() override {
		delete barrel;
		delete body;
	}
};

float Cannon::MIN_SHOOT_TIME = 2.0;
float Cannon::MAX_SHOOT_TIME = 10.0;
float Cannon::MIN_SHOOT_SPEED = 10.0;
float Cannon::MAX_SHOOT_SPEED = 20.0;

Material* Cannon::material = nullptr;
ParamSurface* Cannon::bodySurface = nullptr;
ParamSurface* Cannon::barrelSurface = nullptr;

struct Laser : GameObject {
	static float LASER_LENGTH;
	static float LASER_SPEED;

	Material* material;
	ParamSurface* surface;
	Object3D* object;
	Laser(Shader* _shader) {
		Material* material = new Material();
		material->kd = vec3(1, 0, 0);
		material->ka = material->kd * 3;

		ParamSurface* surface = new Cylinder();

		object = new Object3D(_shader, material, nullptr, surface);
		object->scaling = vec3(0.05, LASER_LENGTH, 0.05);
	}
	void Control(float tstart, float tend, Scene* scene) override;
	void Draw(RenderState renderState) override {
		object->translation = pos;

		vec3 up = vec3(0, 1, 0);

		object->rotationAngle = acosf(dot(normalize(up), normalize(vel)));
		if (abs(object->rotationAngle) > 0.05f) {
			// Ha kicsi, ne forgassuk, fajul a cross product
			object->rotationAxis = cross(normalize(up), normalize(vel));
		}

		object->Draw(renderState);
	}
	Ray getRay() {
		Ray r;
		r.dir = vel;
		r.start = pos;
		return r;
	}
	~Laser() override {
		delete object;
	}
};

float Laser::LASER_LENGTH = 1.0;
float Laser::LASER_SPEED = 30.0;

struct LaserCannon : GameObject {

	Scene* scene;
	Object3D* barrel;
	Object3D* body;

	vec3 rotationDir = vec3(0, 1, 0);
	LaserCannon(Shader* _shader) {
		// Nem staticozom, csak egy van belőle
		Material* bodyMaterial = new Material();
		bodyMaterial->kd = vec3(0.3, 0.1, 0.1);
		bodyMaterial->ks = bodyMaterial->kd * 2;
		bodyMaterial->ka = bodyMaterial->kd * 0;
		bodyMaterial->shininess = 100;

		Material* barrel_material = new Material();
		barrel_material->kd = vec3(0.5, 0.1, 0.1);
		barrel_material->ks = bodyMaterial->kd * 2;
		barrel_material->ka = bodyMaterial->kd * 2;
		barrel_material->shininess = 100;

		ParamSurface* body_surface = new Sphere();

		body = new Object3D(_shader, bodyMaterial, nullptr, body_surface);
		pos = vec3(0, -0.5, 0);
		body->scaling = vec3(1, 1, 1);

		ParamSurface* barrel_surface = new Cylinder();
		barrel = new Object3D(_shader, barrel_material, nullptr, barrel_surface);
		barrel->scaling = vec3(0.2, 3, 0.2);
	}
	void Draw(RenderState renderState) override {
		body->translation = pos;
		barrel->translation = pos;

		vec3 up = vec3(0, 1, 0);

		barrel->rotationAngle = acosf(dot(normalize(up), normalize(rotationDir)));
		if (abs(barrel->rotationAngle) > 0.05f) {
			// Ha kicsi, ne forgassuk, fajul a cross product
			barrel->rotationAxis = cross(normalize(up), normalize(rotationDir));
		}

		body->Draw(renderState);
		barrel->Draw(renderState);
	}

	~LaserCannon() override {
		delete barrel;
		delete body;
	}
};

struct VolumeSample {
	vec3 wPosition;
	// helye világkoordinátákban
	vec4 color;
	// modulációs szín
	float size, angle; // méret és elfordulási szög a képernyőn
	int textureIdx;
	// pamacs képének textúrája
	VolumeSample(vec3 wP, vec4 c, float s, float a, int tex) {
		wPosition = wP;
		color = c;
		size = s;
		angle = a, textureIdx = tex;
	}
};

struct Volume : public Geometry<VolumeSample> {
	void Add(vec3 pos, vec4 col, float size, float angle, int texIdx = 0) {
		VolumeSample pp = {pos, col, size, angle, texIdx};

		Vtx().push_back(pp);
	}
	void Sort(vec3 wEye) { // pamacsok rendezése
		sort(Vtx().begin(), Vtx().end(), [&](const auto& l, const auto& r) {
			return length(l.wPosition - wEye) > length(r.wPosition - wEye);
		});
	}
	void Draw() { // pamacsok GPU-ra másoláa és rajzolása
		if (Vtx().size() == 0)
			return;
		Bind();
		glBufferData(GL_ARRAY_BUFFER, Vtx().size() * sizeof(VolumeSample),
					 &Vtx()[0], GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0); // position
		glEnableVertexAttribArray(1); // color
		glEnableVertexAttribArray(2); // size és angle
		glEnableVertexAttribArray(3); // textureIdx
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VolumeSample),
							  (void*)offsetof(VolumeSample, wPosition));
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VolumeSample),
							  (void*)offsetof(VolumeSample, color));
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VolumeSample),
							  (void*)offsetof(VolumeSample, size));
		glVertexAttribPointer(3, 1, GL_INT, GL_FALSE, sizeof(VolumeSample),
							  (void*)offsetof(VolumeSample, textureIdx));
		glDrawArrays(GL_POINTS, 0, Vtx().size());
		Vtx().clear();
	}
};

const int nSplats = 1;

struct VolumetricGameObject : GameObject {
	static VolumetricShader* shader;
	// árnyalóprogram
	static Volume* volume;
	// az összes pamacs
	// pamacstextúrák
	static Texture* splatTexture[nSplats];
	// az összegyűlt pamacsok lefényképezése
	static void Flush(RenderState state) {
		if (volume->Vtx().size() > 0) {
			glEnable(GL_BLEND); // kompozitálás engedélyezése
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			for (int i = 0; i < nSplats; ++i)
				splatTexture[i]->Bind(i);
			shader->Bind(state);
			volume->Sort(state.wEye);
			volume->Draw();
			glDisable(GL_BLEND); // kompozitálás tiltása
		}
	}
};
VolumetricShader* VolumetricGameObject::shader = nullptr;
Volume* VolumetricGameObject::volume = nullptr;
Texture* VolumetricGameObject::splatTexture[nSplats];

struct Particle {
	vec3 displacement, velocity, acceleration;
	float size, dsize, lifeTime;
	vec4 color, dcolor;

  public:
	Particle(vec3 dir) {
		lifeTime = random_float(1, 2);
		size = random_float(0.5, 1.5);
		dsize = size / lifeTime;

		// alapból dir irányba megy
		velocity = normalize(dir) * 4;
		// random szórás
		velocity += vec3(random_float(-1, 1), random_float(-1, 1), random_float(-1, 1));
		// rand^2-el szorozva, hogy egy picit megálljanak a levegőben
		velocity = velocity * pow(random_float(0, 1), 2);


		color = vec4(random_float(3, 4), random_float(2, 4), random_float(0.3f, 1.3f), 1);
		dcolor = vec4(0, -2.0f, -0.8f, -1) / lifeTime;
	}
	bool Animate(float dt) {
		lifeTime -= dt;
		if (lifeTime <= 0)
			return false;
		displacement += velocity * dt;
		velocity += acceleration * dt;
		size -= dsize * dt;
		color += dcolor * dt;
		return true;
	}
};

class Explosion : public VolumetricGameObject {
	std::vector<Particle> particles;

  public:
	Explosion(vec3 _position, vec3 dir) {
		// dir: lézer iránya
		// egy picit arra szórja a füstöt
		pos = _position;
		for (int i = 0; i < 50; i++){
			Particle p = Particle(dir);
			particles.push_back(p);
		}
	}
	void Animate(float tstart, float tend) {
		alive = false;
		for (auto& particle : particles)
			alive |= particle.Animate(tend - tstart);
	}
	void Draw(RenderState state) {
		for (auto& particle : particles) {
			if (particle.lifeTime > 0) {
				vec3 p = pos + particle.displacement;
				volume->Add(p, particle.color, particle.size, 0, 0);
			}
		}
	}
};

struct Scene {
	static float GRAVITY;
	static int CANNON_NUM;

	bool matrix = false;

	Shader* phongShader;
	int current = 0;
	std::vector<GameObject*> objects[2];
	LaserCannon* laserCannon;
	Camera camera;

  public:

	void Build() {
		phongShader = new PhongShader();
		camera.wEye = vec3(0, 10, 50);
		camera.wLookat = vec3(0, 10, 0);
		camera.wVup = vec3(0, 1, 0);


		GameObject* floor = new Floor(phongShader);
		laserCannon = new LaserCannon(phongShader);

		int maxi = CANNON_NUM;
		for (int i = 0; i < maxi; i++) {
			float radius = 20;
			GameObject* cannon = new Cannon(phongShader, this);
			cannon->pos = vec3(radius * cosf((float)i / maxi * M_PI * 2), 0, radius * sinf((float)i / maxi * M_PI * 2));
			Join(cannon);
		}

		Join(floor);
		Join(laserCannon);
	}
	void Join(GameObject* o) {
		objects[1 - current].push_back(o);
	}
	void Render() {
		RenderState state;
		state.wEye = camera.wEye;
		state.V = camera.V();
		state.P = camera.P();
		for (GameObject* obj : objects[current])
			obj->Draw(state);

		VolumetricGameObject::Flush(state);
	}
	void rotateCamera(float angle) {
		float x = camera.wEye.x;
		float z = camera.wEye.z;
		float radius = std::sqrt(x * x + z * z);

		float theta = atan2(z, x);

		theta += angle;

		camera.wEye.x = cos(theta) * radius;
		camera.wEye.z = sin(theta) * radius;
	}

	void Simulate(float tstart, float tend) {
		const float dt = 0.05f; // dt kicsi
		for (float t = tstart; t < tend; t += dt) {
			float Dt = fmin(dt, tend - t);
			for (auto* obj : objects[current]) {
				obj->Control(t, t + Dt, this);
			}
			for (auto* obj : objects[current]) {
				if (obj->alive)
					Join(obj); // élők az új tömbbe
				else {
					delete obj; // nem élők törlése
				}
			}
			objects[current].clear();
			current = 1 - current; // ping-pong
			for (auto* obj : objects[current])
				obj->Animate(t, t + Dt);

			if (matrix) {
				rotateCamera(0.01);
			}
		}
	}
	void addCannonball(vec3 startpos, vec3 startvel) {
		GameObject* cannonball = new Cannonball(phongShader);
		cannonball->pos = startpos;
		cannonball->vel = startvel;
		cannonball->accel = vec3(0, -GRAVITY, 0);
		Join(cannonball);
	}

	void ShootLaserAt(Cannonball* ball) {
		// bináris kereséssel ütközési időpont számítása
		float left = 0;
		// Ennyi idő alatt el kéne találni
		float right = 10;
		float T;
		int iter_count = 0;
		while (iter_count++ < 100) {
			T = (left + right) / 2;
			vec3 posAtT = ball->pos + ball->vel * T + ball->accel * T * T / 2.0;
			// Ha eléri, akkor kevesebb idő alatt is eléri?
			// Tudni, hogy 0, 0, 0-ból indul a lézer
			if (length(posAtT) / Laser::LASER_SPEED < T) {
				right = T;
			} else {
				// Ha nem éri el, akkor több idő kell
				left = T;
			}
		}
		// Végére 10 / 2^10 pontosságú lesz, mivel középpontra céloz, feltehető hogy el fogja találni

		vec3 posAtT = ball->pos + ball->vel * T + ball->accel * T * T / 2.0;

		// Föld alatt találná el
		if (posAtT.y < -1)
			return;

		vec3 laser_dir = normalize(posAtT);

		GameObject* laser = new Laser(phongShader);
		laser->pos = vec3(0, 0, 0);
		laser->vel = laser_dir * Laser::LASER_SPEED;

		ball->shotAt = true;
		laserCannon->rotationDir = laser_dir;
		Join(laser);
		lasers_shot++;
	}
	void ShootAutoLaser() {
		// Most mindenre lő
		for (GameObject* o : objects[current]) {
			if (Cannonball* ball = dynamic_cast<Cannonball*>(o)) {
				if (!ball->shotAt) {
					ShootLaserAt(ball);
				}
			}
		}
	}

	Cannonball* SelectBall(const Ray& r, float radius_mult = 2) {
		Cannonball* minBall = nullptr;
		float tMin = 10000.0;
		for (GameObject* o : objects[current]) {
			if (Cannonball* ball = dynamic_cast<Cannonball*>(o)) {
				float t = ball->intersect(r, radius_mult);
				if (t > 0 && t < tMin) {
					minBall = ball;
					tMin = t;
				}
			}
		}
		return minBall;
	}

	void toggleMatrix() {
		matrix = !matrix;
		Cannonball::autoShootingEnabled = matrix;
		if (matrix) {
			Cannon::MIN_SHOOT_TIME = 1.0;
			Cannon::MAX_SHOOT_TIME = 3.0;
		} else {
			Cannon::MIN_SHOOT_TIME = 2.0;
			Cannon::MAX_SHOOT_TIME = 10.0;
		}
	}

	std::vector<GameObject*> getGameObjects() {
		return objects[current];
	}
};
float Scene::GRAVITY = 10.0;
int Scene::CANNON_NUM = 20;

void Cannon::Shoot() {
	float speed = random_float(Cannon::MIN_SHOOT_SPEED, Cannon::MAX_SHOOT_SPEED);

	// Közép felé lő
	vec3 shoot_dir = normalize(-pos);
	// Felfelé tart, picit jobban is mint sejteti a szám (nem egységvektor)
	shoot_dir += vec3(0, 0.5, 0);

	// fel/le random
	shoot_dir += vec3(0, random_float(0.5, 0.5), 0);

	vec3 side = cross(normalize(vec3(0, 1, 0)), normalize(-pos));

	shoot_dir += side * random_float(-1, 1);

	rotationDir = shoot_dir;

	scene->addCannonball(pos, shoot_dir * speed);
	balls_shot++;
}

void Laser::Control(float tstart, float tend, Scene* scene) {
	float dt = tend - tstart;
	std::vector<GameObject*> objects = scene->getGameObjects();
	Ray r = this->getRay();
	for (GameObject* o : objects) {
		if (Cannonball* ball = dynamic_cast<Cannonball*>(o)) {

			float t = ball->intersect(r, 0.5);
			if (t > 0 && t < LASER_LENGTH + dt * LASER_SPEED) {
				ball->alive = false;
				alive = false;
				scene->Join(new Explosion(ball->pos, vel));
			}
		}
	}

	if (length(pos) > 100)
		alive = false;
}

void Cannonball::Control(float tstart, float tend, Scene* scene) {
	float dt = tend-tstart;
	if (autoShootingEnabled && autoShootAt > 0) {
		autoShootAt -= dt;
		if (autoShootAt <= 0) {
			scene->ShootLaserAt(this);
		}
	}
	if (this->pos.y < -1)
		alive = false;
}


struct EngineApp : public glApp {
	Scene scene;
	float timeMul = 1.0;
	static float CLICK_RADIUS_MULT;
  public:
	EngineApp() : glApp(3, 3, windowWidth, windowHeight, "3D Engine-ke") {}

	void onInitialization() {
		glViewport(0, 0, windowWidth * OP_SYS_SCALE, windowHeight * OP_SYS_SCALE);
		glEnable(GL_DEPTH_TEST);
		glDisable(GL_CULL_FACE);

		// Minden konstans itt is legyen beállítva, könnyebb változtatáshoz
		Cannonball::autoShootingEnabled = false;
		Cannonball::AUTO_SHOOT_AT_START = 0.5;

		Cannon::MIN_SHOOT_TIME = 2.0;
		Cannon::MAX_SHOOT_TIME = 10.0;
		Cannon::MIN_SHOOT_SPEED = 10.0;
		Cannon::MAX_SHOOT_SPEED = 20.0;

		Laser::LASER_LENGTH = 1.0;
		Laser::LASER_SPEED = 30.0;

		Scene::CANNON_NUM = 2;

		timeMul = 1.0;
		CLICK_RADIUS_MULT = 2.0;

		VolumetricGameObject::shader = new VolumetricShader();
		VolumetricGameObject::volume = new Volume();

		Texture* t = new Texture();

		std::vector<vec3> texture_vec;
		vec3 black = vec3(0, 0, 1);
		vec3 white = vec3(1, 1, 1);
		for (int i = 0; i < 20; i++) {
			for (int j = 0; j < 20; j++) {
				vec3 active = white;
				int dist_from_center = sqrt(abs(10-j) * abs(10-j) + abs(10-i)*abs(10-i));
				float coeff = (float)dist_from_center / 20;
				vec3 final_color = white * coeff;
				texture_vec.push_back(final_color);
			}
		}
		t->updateTexture(20, 20, texture_vec);

		VolumetricGameObject::splatTexture[0] = t;

		srand(time(nullptr));

		scene.Build();
	}
	void onDisplay() {
		// Szép kék háttérszín
		glClearColor(0.529, 0.808, 0.922, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		scene.Render();
	}
	void onKeyboard(int key) {

		// Linuxon kell a debounce, mert minden keyboard input dupla
		static bool debounce = true;
		if (debounce) {
			debounce = false;
			return;
		}
		debounce = true;

		// Mátrix mód be / kikapcsolása
		if (key == 'm') {
			scene.toggleMatrix();
		}

		// automatikusan mindenre lézert lő
		if (key == 'l') {
			scene.ShootAutoLaser();
		}

		if(key == 'e'){
			Cannonball::autoShootingEnabled = !Cannonball::autoShootingEnabled;
		}

		if(key == 'r'){
			scene.camera.wLookat.y += 1;
		}
		if(key == 'f'){
			scene.camera.wLookat.y -= 1;
		}
		if(key == 'w'){
			scene.camera.wEye.y += 1;
		}
		if(key == 's'){
			scene.camera.wEye.y -= 1;
		}

		if (key == 'a') {
			scene.rotateCamera(M_PI / 32);
		}
		if (key == 'd') {
			scene.rotateCamera(-M_PI / 32);
		}

		if(key == 't'){
			timeMul += 0.1;
			if(timeMul > 5) timeMul = 5;
		}
		if(key == 'g'){
			timeMul -= 0.1;
			if(timeMul < 0) timeMul = 0;
		}

		if(key == 'q'){
			auto objects = scene.getGameObjects();
			for (GameObject* o : objects) {
				if (Cannon* cannon = dynamic_cast<Cannon*>(o)) {
					cannon->nextShoot = 0;
				}
			}
		}

		refreshScreen();
	}
	void onMousePressed(MouseButton button, int pX, int pY) {
		if (button != MOUSE_LEFT)
			return;

		Ray r;
		r.dir = scene.camera.cameraDirFromPx(pX, pY);
		r.start = scene.camera.wEye;

		Cannonball* ball = scene.SelectBall(r, CLICK_RADIUS_MULT);

		if (ball != nullptr) {
			scene.ShootLaserAt(ball);
		}
	}
	void onTimeElapsed(float startTime, float endTime) {
		startTime = startTime*timeMul;
		endTime = endTime*timeMul;

		static float lastStatTime = -1;
		if (lastStatTime < endTime - 2) {
			printf("Lasers/Cannonballs : %d/%d (%f)\n", lasers_shot, balls_shot, (float)lasers_shot / balls_shot * 100.0);
			lastStatTime = endTime;
		}
		scene.Simulate(startTime, endTime);
		refreshScreen();
	}
} app;
float EngineApp::CLICK_RADIUS_MULT;
