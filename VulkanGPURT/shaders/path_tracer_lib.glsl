struct Ray {
    vec3 origin;
    vec3 direction;
};

struct HitRecord {
    vec3 p;
    vec3 normal;
    float t;
    vec3 albedo;
    float materialType;
    float fuzz;
    float refractionIndex;
    float frontFace;
};

float degreesToRadians(float degrees) {
    return degrees * PI / 180.0;
}

// Simple hash function for random number generation
uint wangHash(uint s) {
    s = (s ^ 61u) ^ (s >> 16u);
    s *= 9u;
    s = s ^ (s >> 4u);
    s *= 0x27d4eb2du;
    s = s ^ (s >> 15u);
    return s;
}

float rand01(inout uint state) {
    state = wangHash(state);
    return float(state) / 4294967296.0;
}

vec3 randomInUnitSphere(inout uint state) {
    for (int i = 0; i < 16; ++i) {
        vec3 p = vec3(rand01(state), rand01(state), rand01(state)) * 2.0 - vec3(1.0);
        if (dot(p, p) < 1.0) {
            return p;
        }
    }

    return normalize(vec3(rand01(state), rand01(state), rand01(state)) * 2.0 - vec3(1.0));
}

vec3 randomUnitVector(inout uint state) {
    return normalize(randomInUnitSphere(state));
}

// Generate a random point in a unit disk for depth of field sampling
vec2 randomInUnitDisk(inout uint state) {
    for (int i = 0; i < 16; ++i) {
        vec2 p = vec2(rand01(state), rand01(state)) * 2.0 - vec2(1.0);
        if (dot(p, p) < 1.0) {
            return p;
        }
    }
    return vec2(0.0, 0.0);
}

// Schlick's approximation for reflectance
float reflectance(float cosine, float refractionIndex) {
    float r0 = (1.0 - refractionIndex) / (1.0 + refractionIndex);
    r0 = r0 * r0;
    return r0 + (1.0 - r0) * pow(1.0 - cosine, 5.0);
}

// Ray-sphere intersection test
bool hitSphere(Sphere sphere, Ray ray, float tMin, float tMax, out HitRecord rec) {
    vec3 center = sphere.centerRadius.xyz;
    float radius = sphere.centerRadius.w;

    vec3 oc = ray.origin - center;
    float a = dot(ray.direction, ray.direction);
    float halfB = dot(oc, ray.direction);
    float c = dot(oc, oc) - radius * radius;
    float discriminant = halfB * halfB - a * c;

    if (discriminant < 0.0) {
        return false;
    }

    float sqrtd = sqrt(discriminant);

    float root = (-halfB - sqrtd) / a;
    if (root < tMin || root > tMax) {
        root = (-halfB + sqrtd) / a;
        if (root < tMin || root > tMax) {
            return false;
        }
    }

    rec.t = root;
    rec.p = ray.origin + rec.t * ray.direction;
    vec3 outwardNormal = normalize((rec.p - center) / radius);
    rec.frontFace = dot(ray.direction, outwardNormal) < 0.0 ? 1.0 : 0.0;
    rec.normal = rec.frontFace > 0.5 ? outwardNormal : -outwardNormal;
    rec.albedo = sphere.albedoMaterialType.rgb;
    rec.materialType = sphere.albedoMaterialType.w;
    rec.fuzz = sphere.materialParams.x;
    rec.refractionIndex = sphere.materialParams.y;
    return true;
}

// Test for ray intersections with all objects in the scene and find the closest hit
bool hitWorld(Ray ray, float tMin, float tMax, out HitRecord rec) {
    HitRecord temp;
    bool hitAnything = false;
    float closest = tMax;

    for (uint i = 0u; i < pc.sphereCount; ++i) {
        if (hitSphere(sceneBuffer.spheres[i], ray, tMin, closest, temp)) {
            hitAnything = true;
            closest = temp.t;
            rec = temp;
        }
    }

    return hitAnything;
}

// Compute the color seen along a ray by tracing it through the scene and accumulating radiance contributions
vec3 rayColor(Ray ray, inout uint state) {
    vec3 throughput = vec3(1.0);
    vec3 radiance = vec3(0.0);

    for (uint bounce = 0u; bounce < pc.maxBounces; ++bounce) {
        HitRecord rec;
        if (hitWorld(ray, 0.001, 1e30, rec)) {
            vec3 attenuation = vec3(1.0);
            vec3 scatterDir = rec.normal;
            bool scattered = true;

            uint materialType = uint(rec.materialType + 0.5);
            if (materialType == MATERIAL_LAMBERTIAN) {
                scatterDir = rec.normal + randomUnitVector(state);
                if (dot(scatterDir, scatterDir) < 1e-6) {
                    scatterDir = rec.normal;
                }
                attenuation = rec.albedo;
            } else if (materialType == MATERIAL_METAL) {
                vec3 reflected = reflect(normalize(ray.direction), rec.normal);
                scatterDir = reflected + rec.fuzz * randomUnitVector(state);
                attenuation = rec.albedo;
                scattered = dot(scatterDir, rec.normal) > 0.0;
            } else if (materialType == MATERIAL_DIELECTRIC) {
                attenuation = vec3(1.0);
                float ri = rec.frontFace > 0.5 ? (1.0 / rec.refractionIndex) : rec.refractionIndex;
                vec3 unitDirection = normalize(ray.direction);
                float cosTheta = min(dot(-unitDirection, rec.normal), 1.0);
                float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
                bool cannotRefract = ri * sinTheta > 1.0;
                if (cannotRefract || reflectance(cosTheta, ri) > rand01(state)) {
                    scatterDir = reflect(unitDirection, rec.normal);
                } else {
                    scatterDir = refract(unitDirection, rec.normal, ri);
                }
            } else {
                scatterDir = rec.normal + randomUnitVector(state);
                attenuation = rec.albedo;
            }

            if (!scattered) {
                break;
            }

            throughput *= attenuation;
            float originBiasSign = dot(scatterDir, rec.normal) > 0.0 ? 1.0 : -1.0;
            ray.origin = rec.p + rec.normal * (0.001 * originBiasSign);
            ray.direction = normalize(scatterDir);
        } else {
            vec3 unitDir = normalize(ray.direction);
            float t = 0.5 * (unitDir.y + 1.0);
            vec3 sky = mix(vec3(1.0), vec3(0.5, 0.7, 1.0), t);
            radiance += throughput * sky;
            break;
        }
    }

    return radiance;
}

// Generate a ray from the camera through the specified pixel, applying depth of field if enabled
Ray makeCameraRay(uvec2 pixel, inout uint state) {
    vec3 lookFrom = cameraBuffer.camera.lookFromDefocusAngle.xyz;
    float defocusAngle = cameraBuffer.camera.lookFromDefocusAngle.w;
    vec3 lookAt = cameraBuffer.camera.lookAtFocusDist.xyz;
    float focusDist = cameraBuffer.camera.lookAtFocusDist.w;
    vec3 viewUp = cameraBuffer.camera.viewUpVfov.xyz;
    float vfov = cameraBuffer.camera.viewUpVfov.w;

    float theta = degreesToRadians(vfov);
    float h = tan(theta * 0.5);
    float viewportHeight = 2.0 * h * focusDist;
    float viewportWidth = viewportHeight * (float(pc.width) / float(pc.height));

    vec3 w = normalize(lookFrom - lookAt);
    vec3 u = normalize(cross(viewUp, w));
    vec3 v = cross(w, u);

    vec3 viewportU = viewportWidth * u;
    vec3 viewportV = viewportHeight * -v;

    vec3 pixelDeltaU = viewportU / float(pc.width);
    vec3 pixelDeltaV = viewportV / float(pc.height);

    vec3 viewportUpperLeft = lookFrom - (focusDist * w) - viewportU * 0.5 - viewportV * 0.5;
    vec3 pixel00 = viewportUpperLeft + 0.5 * (pixelDeltaU + pixelDeltaV);

    vec2 pixelOffset = vec2(rand01(state) - 0.5, rand01(state) - 0.5);
    vec3 pixelSample = pixel00 + (float(pixel.x) + pixelOffset.x) * pixelDeltaU + (float(pixel.y) + pixelOffset.y) * pixelDeltaV;

    vec3 rayOrigin = lookFrom;
    if (defocusAngle > 0.0) {
        float defocusRadius = focusDist * tan(degreesToRadians(defocusAngle * 0.5));
        vec3 defocusDiskU = u * defocusRadius;
        vec3 defocusDiskV = v * defocusRadius;
        vec2 disk = randomInUnitDisk(state);
        rayOrigin = lookFrom + disk.x * defocusDiskU + disk.y * defocusDiskV;
    }

    Ray ray;
    ray.origin = rayOrigin;
    ray.direction = normalize(pixelSample - rayOrigin);
    return ray;
}
