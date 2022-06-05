#version 410

// per-fragment interpolated values from the vertex shader
in vec3 fN;
in vec3 fL;
in vec3 fV;

in vec2 texCoord2D;
in float texCoord1D;

uniform sampler2D texMap2D;
uniform sampler1D texMap1D;

in vec4 color;

uniform vec4 AmbientProduct;
uniform vec4 DiffuseProduct;
uniform vec4 SpecularProduct;
uniform float Shininess;

uniform int ShadeMode;

out vec4 fragColor;

void main()
{
     // Phong
     if (ShadeMode == 2)
     {
          // Normalize the input lighting vectors
          vec3 N = normalize(fN);
          vec3 V = normalize(fV);
          vec3 L = normalize(fL);
          vec3 H = normalize(L + V);
          vec4 ambient = AmbientProduct;

          float Kd = max(dot(L, N), 0.0);
          vec4 diffuse = Kd * DiffuseProduct;

          float Ks = pow(max(dot(N, H), 0.0), Shininess);
          vec4 specular = Ks * SpecularProduct;

          // discard the specular highlight if the light's behind the vertex
          if (dot(L, N) < 0.0)
               specular = vec4(0.0, 0.0, 0.0, 1.0);

          fragColor = ambient + diffuse + specular;
          fragColor.a = 1.0;
     }
     else if (ShadeMode == 0 || ShadeMode == 1)
     {
          fragColor = color;
     } else if (ShadeMode == 3) {
          fragColor = texture2D(texMap2D, texCoord2D);
     } else if (ShadeMode == 4) {
          fragColor = texture1D(texMap1D, texCoord1D);
     }
}    