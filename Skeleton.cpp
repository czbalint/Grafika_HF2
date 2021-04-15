//=============================================================================================
//Mintaprogram: Zold haromszog. Ervenyes 2019. osztol.
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!!
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : Czanik Balint
// Neptun : H7EEPG
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
//=============================================================================================

#include "framework.h"

// vertex shader in GLSL
const char *vertexSource = R"(
	#version 330
precision highp float;

uniform vec3 wLookAt, wRight, wUp;          // pos of eye

layout(location = 0) in vec2 cCamWindowVertex;	// Attrib Array 0
out vec3 p;

void main() {
    gl_Position = vec4(cCamWindowVertex, 0, 1);
    p = wLookAt + wRight * cCamWindowVertex.x + wUp * cCamWindowVertex.y;
}
)";

// fragment shader in GLSL
const char *fragmentSource = R"(
    #version 330
precision highp float;
const int objFaces = 12;
uniform vec3 wEye, v[20];
uniform int planes[objFaces * 3];
uniform vec3 kd, ks, F0;
in vec3 p;
out vec4 fragmentColor;
const vec3 La = vec3(0.5f, 0.6f, 0.6f);
const vec3 Le = vec3(1.2f, 0.5f, 1.8f);
const vec3 lightPosition = vec3(0.2f, -0.2f, 0.25f);
const vec3 ka = vec3(0.2f, 0.3f, 0.2f);
const float shininess = 500.0f;
const int maxdepth = 5;
const float epsilon = 0.01f;

struct Hit {
    float t;
    vec3 position, normal;
    int mat;	// material index
};

struct Ray {
    vec3 start, dir, weight;
};

struct Sphare{
    vec3 center;
    float radius;
};



void getObjPlane(int i, float scale, out vec3 pp, out vec3 normal){
    vec3 p1 = v[planes[3 * i] - 1], p2 = v[planes[3 * i + 1] - 1], p3 = v[planes[3 * i + 2] - 1];
    normal = cross(p2 - p1, p3 - p1);
    if (dot(p1, normal) < 0) normal = -normal;
    pp= p1 * scale + vec3(0, 0, 0.03f);
}

Hit intersectConvexPolyhedron(Ray ray, Hit hit, float scale, int mat){
    for(int i = 0; i < objFaces; i++){
        vec3 p1, normal;
        getObjPlane(i, scale, p1, normal);
        float ti = abs(dot(normal, ray.dir)) > epsilon ? dot(p1 - ray.start, normal) / dot(normal, ray.dir) : -1;
        if (ti <= epsilon || (ti > hit.t && hit.t > 0)) continue;
        vec3 pintersect = ray.start + ray.dir * ti;
        bool outside = false;
        for(int j = 0; j < objFaces; j++){
            if (i == j) continue;
            vec3 p11, n;
            getObjPlane(j, scale, p11, n);
            if (dot(n, pintersect - p11) > 0) {
                outside = true;
                break;
            }
        }
        if (!outside){
            bool portal = true;
            for(int j = 0; j < objFaces; j++){
                if (i == j) continue;
                vec3 p11, n;
                getObjPlane(j, scale, p11, n);
                hit.t = ti;
                hit.position = pintersect;
                hit.normal = normalize(normal);
                if ( abs(dot(n, pintersect - p11)) < 0.1f ) {
                    hit.mat = 1;
                    portal = false;
                    break;
                }
            }
            if (portal){
                hit.mat = 3;
            }
        }
    }
    return hit;
}

Hit intersectImplicit(Ray ray, Hit besthit){
    float a = 1.1f;
    float b = 8.4f;
    float c = 1.5f;
    Hit hit;
    hit.t = -1;
    ray.dir = normalize(ray.dir);
    float A, B, C;

    A = a * ray.dir.x * ray.dir.x + b * ray.dir.y * ray.dir.y;
    B = 2 * a * ray.start.x * ray.dir.x + 2 * b * ray.start.y * ray.dir.y - c * ray.dir.z;
    C = a * ray.start.x * ray.start.x + b * ray.start.y * ray.start.y - c * ray.start.z;

    float disc = (B*B) - (4 * A * C);
    if (disc < 0) return besthit;
    float t1 = (-B + sqrt(disc)) / (2 * A);
    float t2 = (-B - sqrt(disc)) / (2 * A);

    if (t1 <= 0) return besthit;
    vec3 P1 = ray.start + ray.dir * t1;
    vec3 P2 = ray.start + ray.dir * t2;
    if (length(P1) > 0.3f && length(P2) < 0.3f) hit.t = t2;
    if (length(P1) < 0.3f && length(P2) > 0.3f) hit.t = t1;
    if (length(P1) < 0.3f && length(P2) < 0.3f){
        hit.t = length(P1) < length(P2) ? t1 : t2;
    }
    hit.position = ray.start + ray.dir * hit.t;
    vec3 F = vec3(1,0,(2 * a * hit.position.x) / c );
    vec3 G = vec3(0,1,(2 * b * hit.position.y) / c );
    hit.normal = -normalize(cross(F,G));
    hit.mat = 2;
    if (hit.t < besthit.t || besthit.t < 0){
        return hit;
    }
    return besthit;
}

Hit firstIntersect(Ray ray) {
    Hit bestHit;
    bestHit.t = -1;
    bestHit = intersectImplicit(ray, bestHit);
    bestHit = intersectConvexPolyhedron(ray, bestHit, 1.0f, 1);
    if (dot(ray.dir, bestHit.normal) > 0) bestHit.normal *= -1;
    return bestHit;
}

vec3 trace(Ray ray) {
    vec3 outRadiance = vec3(0, 0, 0);
    int  portalCount = 0;
    int goldCont = 0;

    while(portalCount < maxdepth && goldCont < maxdepth){
        Hit hit = firstIntersect(ray);
        if (hit.mat < 2){
            vec3 lightdir = normalize(lightPosition - hit.position);
            float cosTheta = dot(hit.normal, lightdir);
            if (cosTheta > 0) {
                vec3 LeIn = Le / dot(lightPosition - hit.position, lightPosition - hit.position);
                outRadiance += ray.weight * LeIn * kd * cosTheta;
                vec3 halfway = normalize(-ray.dir + lightdir);
                float cosDelta = dot(hit.normal, halfway);
                if (cosDelta > 0) outRadiance += ray.weight * LeIn * ks * pow(cosDelta, shininess);
            }
            ray.weight *= ka;
            outRadiance += ray.weight;
            return outRadiance;
        }
        if (hit.mat == 2){
            ray.weight *= F0 + (vec3(1, 1, 1) - F0) * pow(1 - dot(-ray.dir, hit.normal), 5);
            ray.start = hit.position + hit.normal * epsilon;
            ray.dir = reflect(ray.dir, hit.normal);
            goldCont++;
        }

        if (hit.mat == 3){
            if (portalCount < maxdepth){
                float alfa = 72.0f * 3.14 / 180;
                ray.start = hit.position + hit.normal * epsilon;
                ray.dir = reflect(ray.dir, hit.normal);
                vec3 tmp = ray.start + ray.dir;
                ray.start = ray.start * cos(alfa) + cross(hit.normal, ray.start) * sin(alfa) + hit.normal * dot(hit.normal, ray.start) * (1 - cos(alfa));
                tmp = tmp * cos(alfa) + cross(hit.normal, tmp) * sin(alfa) + hit.normal * dot(hit.normal, tmp) * (1 - cos(alfa));
                ray.dir = normalize(tmp - ray.start);
                portalCount++;
            }
        }
    }
    outRadiance += ray.weight * La;
    return outRadiance;
}

void main() {
    Ray ray;
    ray.start = wEye;
    ray.dir = normalize(p - wEye);
    ray.weight = vec3(1,1,1);
    fragmentColor = vec4(trace(ray), 1);
}
)";

struct Camera {
	vec3 eye, lookat, right, pvup, rvup;
	float fov = 45 * (float)M_PI / 180;

	Camera() : eye(0,1,1), pvup(0,0,1), lookat(0,0,0) { set(); }
	void set() {
        vec3 w = eye - lookat;
        float f = length(w);
        right = normalize(cross(pvup, w)) * f * tanf(fov / 2);
        rvup = normalize(cross(w, right)) * f * tanf(fov / 2);
	}
	void Animate(float t) {
	    float r = sqrt(eye.x * eye.x + eye.y * eye.y);
	    eye = vec3(r * cos(t) + lookat.x, r * sin(t) + lookat.y, eye.z);
	    set();
	}

};

Camera camera;
GPUProgram shader;

float F(float n, float k) {
    return ((n-1)*(n-1) + k*k) / ((n+1)*(n+1)+k*k);
}

// Initialization, create an OpenGL context
void onInitialization() {
	glViewport(0, 0, windowWidth, windowHeight);
	 unsigned int vao, vbo;
	 glGenVertexArrays(1, &vao); glBindVertexArray(vao);
	 glGenBuffers(1, &vbo); glBindBuffer(GL_ARRAY_BUFFER, vbo);
	 float vertexCoord[] = {-1,-1,1,-1,1,1,-1,1};
	 glBufferData(GL_ARRAY_BUFFER, sizeof(vertexCoord), vertexCoord, GL_STATIC_DRAW);
	 glEnableVertexAttribArray(0);
	 glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,NULL);

    shader.create(vertexSource,fragmentSource,"fragmentColor");
    shader.Use();
    const float g = 0.618f, G = 1.618f;
    std::vector<vec3> v = {
            vec3(0, g, G), vec3(0, -g, G), vec3(0, -g, -G), vec3(0, g, -G), vec3(G, 0, g), vec3(-G, 0, g), vec3(-G, 0, -g), vec3(G, 0, -g),
            vec3(g,G,0), vec3(-g,G,0), vec3(-g,-G,0), vec3(g,-G,0), vec3(1,1,1), vec3(-1,1,1), vec3(-1,-1,1), vec3(1,-1,1),
            vec3(1,-1,-1), vec3(1,1,-1), vec3(-1,1,-1), vec3(-1,-1,-1)
    };
    for (int i = 0; i < v.size(); i++) shader.setUniform(v[i], "v[" + std::to_string(i) + "]");

    std::vector<int> planes = {
            1,2,16, 1,13,9, 1,14,6, 2,15,11, 3,4,18, 3,17,12, 3,20,7, 19,10,9, 16,12,17, 5,8,18, 14,10,19, 6,7,20
    };
    for (int i = 0; i < planes.size(); i++) shader.setUniform(planes[i], "planes[" + std::to_string(i) + "]");

    shader.setUniform(vec3(1.3f, 0.2f, 0.2f), "kd");
    shader.setUniform(vec3(1,1,1), "ks");
    shader.setUniform(vec3(F(0.17,3.1), F(0.35,2.7), F(1.5,1.9)), "F0");
}

// Window has become invalid: Redraw
void onDisplay() {
    glClearColor(0,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shader.setUniform(camera.eye, "wEye");
    shader.setUniform(camera.lookat, "wLookAt");
    shader.setUniform(camera.right, "wRight");
    shader.setUniform(camera.rvup, "wUp");
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glutSwapBuffers();
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {

}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {

}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) {
}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {

}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	camera.Animate(glutGet(GLUT_ELAPSED_TIME) / 2000.0f);
	glutPostRedisplay();
}
