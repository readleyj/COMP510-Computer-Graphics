#version 410

in vec4 vPosition;
in vec3 vNormal;
in vec4 vColor;

out vec4 color;
out vec3 fN;
out vec3 fV;
out vec3 fL;

uniform mat4 ModelView;
uniform mat4 Projection;

uniform vec4 AmbientProduct;
uniform vec4 DiffuseProduct;
uniform vec4 SpecularProduct;

uniform vec4 LightPosition;
uniform float Shininess;

uniform int ShadeMode;

void main()
{
    // Gouraud
    if (ShadeMode == 1) {
        // Transform vertex  position into eye coordinates
        vec3 pos = (ModelView * vPosition).xyz;

        vec3 L = normalize((ModelView * LightPosition).xyz - pos);
        vec3 E = normalize(-pos);
        vec3 H = normalize(L + E);

        // Transform vertex normal into eye coordinates
        vec3 N = normalize(ModelView * vec4(vNormal, 0.0)).xyz;

        // Compute terms in the illumination equation
        vec4 ambient = AmbientProduct;

        float Kd = max(dot(L, N), 0.0);
        vec4 diffuse = Kd * DiffuseProduct;

        float Ks = pow(max(dot(N, H), 0.0), Shininess);
        vec4 specular = Ks * SpecularProduct;

        if (dot(L, N) < 0.0)
        {
            specular = vec4(0.0, 0.0, 0.0, 1.0);
        }

        color = ambient + diffuse + specular;
        color.a = 1.0;
    } 
    // Phong
    else if (ShadeMode == 2)
    {
        // Transform vertex position into camera coord.
        vec3 pos = (ModelView * vPosition).xyz;

        // normal direction in camera coordinates
        fN = (ModelView * vec4(vNormal, 0.0)).xyz;

        // viewer direction in camera coordinates
        fV = -pos;

        // light direction if directional light source
        fL = LightPosition.xyz;

        // if point light source
        if (LightPosition.w != 0.0) {
            fL = LightPosition.xyz - pos;
        }
    }
    // No shading
    else if (ShadeMode == 0) {
        color = vColor;
    }

    gl_Position = Projection * ModelView * vPosition;
}