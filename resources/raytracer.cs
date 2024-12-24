#version 430
layout(local_size_x = 16, local_size_y = 16) in;
layout(rgba32f, binding = 0) uniform image2D imgOutput;
layout(std140, binding = 1) uniform AccumulationBlock
{
    uint frameCount;
};
layout(rgba32f, binding = 1) uniform image2D accumulationImage;

//Global variables
const float MIN_DIST = 0.0001;  
const float MAX_DIST = 1000.0;

//Anti alsiasing
const int SAMPLES = 4;
const float ONE_OVER_SAMPLES = 1.0 / float(SAMPLES);

//Material types
const int MATERIAL_DIFFUSE = 0;
const int MATERIAL_METAL = 1;
const int MATERIAL_GLASS = 2;

//Camera uniforms 
layout(std140, binding = 0) uniform CameraBlock
{
    vec4 cameraPos;
    vec4 cameraFront;
    vec4 cameraUp;
    vec4 cameraRight;
    vec2 fovAndAspect;
    vec2 padding;
};

struct Material
{
    int type;
    vec3 albedo;
    float roughness;  
    float ior;     //index of refraction
};

struct Ray
{
    vec3 origin;
    vec3 direction;
};

struct HitRecord
{
    vec3 p;
    vec3 normal;
    float t;
    bool front_face;
    Material material;
};


uint seed;
uint wang_hash(uint seed)
{
    seed = (seed ^ 61) ^ (seed >> 16);
    seed *= 9;
    seed = seed ^ (seed >> 4);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15);
    return seed;
}

float random_float()
{
    seed = wang_hash(seed);
    return float(seed) / 4294967296.0;
}

vec3 random_unit_vector()
{
    float z = random_float() * 2.0 - 1.0;
    float a = random_float() * 2.0 * 3.1415926;
    float r = sqrt(1.0 - z * z);
    return vec3(r * cos(a), r * sin(a), z);
}

vec3 random_on_hemisphere(vec3 normal)
{
    vec3 on_unit_sphere = random_unit_vector();
    return dot(on_unit_sphere, normal) > 0.0 ? on_unit_sphere : -on_unit_sphere;
}

vec2 random_in_unit_square()
{
    return vec2(random_float(), random_float());
}

//Material Functions
vec3 reflect(vec3 v, vec3 n)
{
    return v - 2.0 * dot(v, n) * n;
}

vec3 refract(vec3 uv, vec3 n, float etai_over_etat)
{
    float cos_theta = min(dot(-uv, n), 1.0);
    vec3 r_out_perp = etai_over_etat * (uv + cos_theta * n);
    vec3 r_out_parallel = -sqrt(abs(1.0 - dot(r_out_perp, r_out_perp))) * n;
    return r_out_perp + r_out_parallel;
}

//Schlick approximation for glass reflectivity
float schlick(float cosine, float ref_idx)
{
    float r0 = (1.0 - ref_idx) / (1.0 + ref_idx);
    r0 = r0 * r0;
    return r0 + (1.0 - r0) * pow((1.0 - cosine), 5.0);
}

vec2 get_subpixel_offset(int sampleIdx)
{
    int x = sampleIdx % 2;
    int y = sampleIdx / 2;

    vec2 stratifiedPos = vec2(x, y) * 0.5;
    vec2 jitter = random_in_unit_square() * 0.5;

    return stratifiedPos + jitter;
}


Ray createCameraRay(vec2 uv)
{
    //Convert UV from [0,1] to [-1,1] and apply aspect ratio correction
    vec2 ndc = uv * 2.0 - 1.0;
    ndc.x *= fovAndAspect.y;
    float tanFov = tan(fovAndAspect.x * 0.5);

    //Create camera space vectors
    vec3 rayDir = normalize(
        cameraFront.xyz +
        ndc.x * tanFov * cameraRight.xyz +
        ndc.y * tanFov * cameraUp.xyz
    );

    return Ray(cameraPos.xyz, rayDir);
}

bool intersectSphere(Ray ray, vec3 center, float radius, out HitRecord rec)
{
    vec3 oc = ray.origin - center;
    float a = dot(ray.direction, ray.direction);
    float half_b = dot(oc, ray.direction);
    float c = dot(oc, oc) - radius * radius;
    float discriminant = half_b * half_b - a * c;

    if (discriminant < 0.0) return false;

    float sqrtd = sqrt(discriminant);
    
    float root = (-half_b - sqrtd) / a;

    if (root < MIN_DIST || root > MAX_DIST)
    {
        root = (-half_b + sqrtd) / a;  
        if (root < MIN_DIST || root > MAX_DIST)
            return false;
    }

    rec.t = root;
    rec.p = ray.origin + rec.t * ray.direction;
    vec3 outward_normal = (rec.p - center) / radius;
    rec.front_face = dot(ray.direction, outward_normal) < 0;
    rec.normal = rec.front_face ? outward_normal : -outward_normal;

    rec.normal = normalize(rec.normal);
    return true;
}

//ray scatter function
bool scatter(Ray r_in, HitRecord rec, out vec3 attenuation, out Ray scattered)
{
    if (rec.material.type == MATERIAL_DIFFUSE)
    {
        vec3 scatter_direction = rec.normal + random_unit_vector();
        scattered = Ray(rec.p, normalize(scatter_direction));
        attenuation = rec.material.albedo;
        return true;
    }
    else if (rec.material.type == MATERIAL_METAL)
    {
        vec3 reflected = reflect(normalize(r_in.direction), rec.normal);
        scattered = Ray(rec.p, normalize(reflected + rec.material.roughness * random_unit_vector()));
        attenuation = rec.material.albedo;
        return dot(scattered.direction, rec.normal) > 0.0;
    }
    else if (rec.material.type == MATERIAL_GLASS)
    {
        attenuation = vec3(1.0);
        float refraction_ratio = rec.front_face ?
            (1.0 / rec.material.ior) : rec.material.ior;

        vec3 unit_direction = normalize(r_in.direction);
        float cos_theta = min(dot(-unit_direction, rec.normal), 1.0);
        float sin_theta = sqrt(1.0 - cos_theta * cos_theta);

        bool cannot_refract = refraction_ratio * sin_theta > 1.0;
        vec3 direction;

        if (cannot_refract || schlick(cos_theta, refraction_ratio) > random_float())
            direction = reflect(unit_direction, rec.normal);
        else
            direction = refract(unit_direction, rec.normal, refraction_ratio);

        scattered = Ray(rec.p, direction);
        return true;
    }
    return false;
}

vec3 ray_color(Ray r)
{
    vec3 attenuation = vec3(1.0);
    Ray current_ray = r;

    // Define materials
    Material diffuse_material = Material(MATERIAL_DIFFUSE, vec3(0.7, 0.3, 0.3), 0.0, 0.0);
    Material metal_material = Material(MATERIAL_METAL, vec3(0.8, 0.8, 0.8), 0.1, 0.0);
    Material glass_material = Material(MATERIAL_GLASS, vec3(1.0), 0.0, 1.0);
    Material ground_material = Material(MATERIAL_DIFFUSE, vec3(0.1, 0.1, 0.1), 0.0, 0.0);

    // Spheres with different materials
    vec3 sphere1_center = vec3(-2.0, 0.0, -3.0);  // Diffuse
    vec3 sphere2_center = vec3(0.0, 0.0, -3.0);   // Metal
    vec3 sphere3_center = vec3(2.0, 0.0, -3.0);   // Glass
    vec3 ground_center = vec3(0.0, -1001.0, -3.0);

    float sphere_radius = 1.0;
    float ground_radius = 1000.0;

    const int MAX_BOUNCES = 100;

    for (int bounce = 0; bounce < MAX_BOUNCES; bounce++)
    {
        HitRecord rec;
        bool hit_anything = false;
        float closest_so_far = MAX_DIST;
        HitRecord temp_rec;

        // Test all spheres
        if (intersectSphere(current_ray, sphere1_center, sphere_radius, temp_rec))
        {
            temp_rec.material = diffuse_material;
            if (temp_rec.t < closest_so_far)
            {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }

        if (intersectSphere(current_ray, sphere2_center, sphere_radius, temp_rec))
        {
            temp_rec.material = metal_material;
            if (temp_rec.t < closest_so_far)
            {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }

        if (intersectSphere(current_ray, sphere3_center, sphere_radius, temp_rec))
        {
            temp_rec.material = glass_material;
            if (temp_rec.t < closest_so_far)
            {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }

        if (intersectSphere(current_ray, ground_center, ground_radius, temp_rec))
        {
            temp_rec.material = ground_material;
            if (temp_rec.t < closest_so_far)
            {
                hit_anything = true;
                closest_so_far = temp_rec.t;
                rec = temp_rec;
            }
        }

        if (hit_anything)
        {
            Ray scattered;
            vec3 scatter_attenuation;
            if (scatter(current_ray, rec, scatter_attenuation, scattered))
            {
                attenuation *= scatter_attenuation;
                current_ray = scattered;

                if (max(max(attenuation.x, attenuation.y), attenuation.z) < 0.01)
                {
                    return vec3(0.0);
                }
            }
            else
            {
                return vec3(0.0);
            }
        }
        else
        {
            // Sky color
            float t = 0.5 * (normalize(current_ray.direction).y + 1.0);
            vec3 skyColorTop = vec3(0.529, 0.808, 0.922);
            vec3 skyColorBottom = vec3(1.0, 1.0, 1.0);
            return attenuation * mix(skyColorBottom, skyColorTop, t);
        }
    }

    return attenuation * 0.1;
}

void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    ivec2 screenSize = imageSize(imgOutput);

    if (pixel.x >= screenSize.x || pixel.y >= screenSize.y)
    {
        return;
    }

    // Initialize random seed
    seed = uint(pixel.x ^ pixel.y ^ frameCount ^ uint(gl_GlobalInvocationID.x * 1973 + gl_GlobalInvocationID.y * 9277));

    // Accumulate samples
    vec3 pixelColor = vec3(0.0);

    for (int i = 0; i < SAMPLES; i++)
    {
        vec2 offset = get_subpixel_offset(i);
        vec2 uv = (vec2(pixel) + offset) / vec2(screenSize);
        Ray currentRay = createCameraRay(uv);
        pixelColor += ray_color(currentRay);
    }

    // Average samples
    vec3 currentColor = pixelColor * ONE_OVER_SAMPLES;

    // Temporal accumulation
    vec4 accumulatedColor = imageLoad(accumulationImage, pixel);
    vec3 finalColor;

    if (frameCount == 0)
    {
        finalColor = currentColor;
    }
    else
    {
        float weight = 1.0 / float(frameCount + 1);
        finalColor = mix(accumulatedColor.rgb, currentColor, weight);
    }

    // Store results
    imageStore(accumulationImage, pixel, vec4(finalColor, 1.0));
    imageStore(imgOutput, pixel, vec4(finalColor, 1.0));
}