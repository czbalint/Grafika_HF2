#version 330
precision highp float;

const vec3 La = vec3(0.5f, 0.6f, 0.6f);
const vec3 Le = vec3(1.5f, 1.5f, 0.8f);
const vec3 lightPosition = vec3(0.2f, 0.1f, 0.2f);
const vec3 ka = vec3(0.5f, 0.5f, 0.5f);
const float shininess = 100.0f;
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

const int objFaces = 12;
uniform int top;
uniform vec3 wEye, v[20];
uniform int planes[objFaces * 3];
uniform vec3 kd[2], ks[2], F0;

void getObjPlane(int i, float scale, out vec3 p, out vec3 normal){
    vec3 p1 = v[planes[3 * i] - 1], p2 = v[planes[3 * i + 1] - 1], p3 = v[planes[3 * i + 2] - 1];
    normal = cross(p2 - p1, p3 - p1);
    if (dot(p1, normal) < 0) normal = -normal;
    p = p1 * scale + vec3(0, 0, 0.03f);
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
                if (abs(dot(n, pintersect - p11)) < 0.1f) {
                    hit.mat = mat;
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

Hit intersactSpahre(Ray ray, Hit besthit, vec3 cen){
    Sphare object;
    object.radius = 0.3f;
    object.center = cen;
    Hit hit;
    hit.t = -1;
    vec3 dist = ray.start - object.center;
    float a = dot(ray.dir, ray.dir);
    float b = dot(dist, ray.dir) * 2.0f;
    float c = dot(dist, dist) - object.radius * object.radius;
    float discr = b * b - 4.0f * a * c;
    if (discr < 0) return besthit;
    float sqrt_discr = sqrt(discr);
    float t1 = (-b + sqrt_discr) / 2.0f / a;	// t1 >= t2 for sure
    float t2 = (-b - sqrt_discr) / 2.0f / a;
    if (t1 <= 0) return besthit;
    hit.t = (t2 > 0) ? t2 : t1;
    hit.position = ray.start + ray.dir * hit.t;
    hit.normal = (hit.position - object.center) / object.radius;
    hit.mat = 2;
    if (hit.t < besthit.t || besthit.t < 0){
        return hit;
    }
    return besthit;

}

Hit intersectImplicit(Ray ray, Hit besthit){
    float a = 1.1f;
    float b = 8.4f;
    float c = 1.5f;
    Hit hit;
    hit.t = -1;
    ray.dir = normalize(ray.dir);
    float A, B, C;

//    A = (a * (ray.dir.x * ray.dir.x)) + (b * (ray.dir.y * ray.dir.y));
//    B = 2 * a * ray.start.x * ray.dir.x + 2 * b * ray.start.y * ray.dir.y - c * ray.dir.z;
//    C = a * (ray.start.x * ray.start.x) + b * (ray.start.y * ray.start.y) - c * ray.dir.z;

    A = a * ray.dir.x * ray.dir.x + b * ray.dir.y * ray.dir.y;
    B = 2 * a * ray.start.x * ray.dir.x + 2 * b * ray.start.y * ray.dir.y - c * ray.dir.z;
    C = a * ray.start.x * ray.start.x + b * ray.start.y * ray.start.y - c * ray.start.z;

    float disc = (B*B) - (4 * A * C);
    if (disc < 0) return besthit;
    float t1 = (-B + sqrt(disc)) / (2 * A);
    float t2 = (-B - sqrt(disc)) / (2 * A);

    vec3 pos1 = ray.start + ray.dir * t1;
    if (t2 > 0) {
        vec3 pos2 = ray.start + ray.dir * t2;
        if (length(pos2) < 0.3f)
        if (length(pos1) < 0.3f)
        hit.t = (t2 < t1) ? t2 : t1;
        else
        hit.t = t2;
        else if (length(pos1) < 0.3)
        hit.t = t1;
        else
        return besthit;
    } else if (length(pos1) < 0.3f)
    hit.t = t1;
    else
    return besthit;

    hit.position = ray.start + ray.dir * hit.t;

//    if (t1 <= 0) return besthit;
//    hit.t = (t2 > 0) ? t2 : t1;
//    hit.position = dis + ray.dir * hit.t;
    //if (distance(vec3(0,0,0), hit.position) > 0.3f) return besthit;
    vec3 F = vec3(1,0,(2 * a * hit.position.x) / c );
    vec3 G = vec3(0,1,(2 * b * hit.position.y) / c );
    //hit.normal = -normalize(cross(F,G));
    hit.normal = normalize(vec3(-2.0f * a * hit.position.x / c, -2.0f * b * hit.position.y / c, 1.0f));
    hit.mat = 2;
    if (hit.t < besthit.t || besthit.t < 0){
        return hit;
    }
    return besthit;
}

//Hit solveQuadratic(float a, float b, float c, Ray ray, Hit hit, float zmin, float zmax, float normz){
//    float discr = b * b - 4.0f * a * c;
//    if (discr >= 0){
//        float sqrt_discr = sqrt(discr);
//        float t1 = (-b + sqrt_discr) / 2.0f / a;
//        vec3 p = ray.start + ray.dir * t1;
//        if (p.z > zmax || p.z < zmin) t1 = -1;
//        float t2 = (-b - sqrt_discr) / 2.0f / a;
//        p = ray.start + ray.dir * t2;
//        if (t2 > 0 && (t2 < t1 || t1 < 0)) t1 = t2;
//        if (t1 > 0 && (t1 < hit.t || hit.t < 0)) {
//            hit.t = t1;
//            hit.position = ray.start + ray.dir * hit.t;
//            hit.normal = normalize(vec3(-hit.position.x, -hit.position.y, normz));
//            hit.mat = 2;
//        }
//    }
//    return hit;
//}
//
//Hit intersectMirascope(Ray ray, Hit hit) {
//    const float f = 0.25f;
//    const float H = 0.98f * f;
//
//    float a = dot(ray.dir.xy, ray.dir.xy);
//    float b = dot(ray.dir.xy, ray.start.xy) * 2 - 4 * f * ray.dir.z;
//    float c = dot(ray.start.xy, ray.start.xy) - 4 * f * ray.start.z;
//    hit = solveQuadratic(a,b,c,ray,hit,0,f/2,2*f);
//    if (top == 0) return hit;
//    b += 8 * f * ray.dir.z;
//    c += 8 * f * ray.start.z - 4 * f * f;
//    hit = solveQuadratic(a, b, c, ray, hit, f/2, H, -2 * f);
//    return hit;
//}

Hit firstIntersect(Ray ray) {
    Hit bestHit;
    bestHit.t = -1;
    //bestHit = intersectMirascope(ray, bestHit);
    //bestHit = intersectConvexPolyhedron(ray, bestHit, 0.02f, 0);
    bestHit = intersectImplicit(ray, bestHit);
    //bestHit = intersactSpahre(ray, bestHit, vec3(0,0,0));
    //bestHit = intersactSpahre(ray, bestHit, vec3(0.7,0.7,0.7));
    //abestHit = intersactSpahre(ray, bestHit, vec3(-0.7,-0.7,-0.7));
    bestHit = intersectConvexPolyhedron(ray, bestHit, 1f, 1);

    if (dot(ray.dir, bestHit.normal) > 0) bestHit.normal = bestHit.normal * (-1);
    return bestHit;
}

vec3 trace(Ray ray) {
    vec3 outRadiance = vec3(0, 0, 0);
    int  portalCount = 0;

    for(int d = 0; d < maxdepth; d++) {
        Hit hit = firstIntersect(ray);
        //if (hit.t < 0) break;
        if (hit.mat < 2){
            vec3 lightdir = normalize(lightPosition - hit.position);
            float cosTheta = dot(hit.normal, lightdir);
            if (cosTheta > 0) {
                vec3 LeIn = Le / dot(lightPosition - hit.position, lightPosition - hit.position);
                outRadiance += ray.weight * LeIn * kd[hit.mat] * cosTheta;
                vec3 halfway = normalize(-ray.dir + lightdir);
                float cosDelta = dot(hit.normal, halfway);
                if (cosDelta > 0) outRadiance += ray.weight * LeIn * ks[hit.mat] * pow(cosDelta, shininess);
            }
            ray.weight *= ka;
            outRadiance += ray.weight;
            return outRadiance;
        }
        if (hit.mat == 2){
            ray.weight *= F0 + (vec3(1, 1, 1) - F0) * pow(1 - dot(-ray.dir, hit.normal), 5);
            ray.start = hit.position + hit.normal * epsilon;
            ray.dir = reflect(ray.dir, hit.normal);

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
            } else {
                break;
            }
            d--;
            continue;
        }
    }
    outRadiance += ray.weight * La;
    return outRadiance;
}

in vec3 p;
out vec4 fragmentColor;

void main() {
    Ray ray;
    ray.start = wEye;
    ray.dir = normalize(p - wEye);
    ray.weight = vec3(1,1,1);
    fragmentColor = vec4(trace(ray), 1);
}