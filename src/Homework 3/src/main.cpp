#include "Angel.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

const std::string PRINT_DELIMITER = "------------------------------------------------------";

typedef vec4 color4;
typedef vec3 color3;
typedef vec4 point4;

const vec3 TOP_LEFT_FRONT_CORNER = vec3(-0.7, 1.0, -2.0);
const GLfloat INITIAL_HORIZONTAL_SPEED = 0.01;
const GLfloat INITIAL_VERTICAL_SPEED = -0.015;
const GLfloat INITIAL_Z_SPEED = -0.01;

const point4 INITIAL_LIGHT_POSITION = (1.0, 1.0, 0.0, 0.0);

const GLfloat SCALE_FACTOR = 0.20;
const GLfloat BALL_RADIUS = SCALE_FACTOR;
const GLfloat FOV = 90.0;
const GLfloat zNear = 0.5;
const GLfloat zFar = 5.0;

// Angle by which Bunny should be rotated in X-direction
// Need to do this so Bunny faces the camera
const GLfloat BUNNY_X_ROTATION_ANGLE = -90.0;

vec3 displacement = TOP_LEFT_FRONT_CORNER;

GLfloat curHorizontalSpeed = INITIAL_HORIZONTAL_SPEED;
GLfloat curVerticalSpeed = INITIAL_VERTICAL_SPEED;
GLfloat curZSpeed = INITIAL_Z_SPEED;

// Bool for toggling between 2D and 3D
bool is3D = true;

// Boundaries for the room
GLfloat leftWallBoundary = -1.0;
GLfloat rightWallBoundary = 1.0;
GLfloat bottomWallBoundary = -1.0;
GLfloat topWallBoundary = 1.0;
GLfloat backWallBoundary = -3.0;
GLfloat frontWallBoundary = -zNear;

int curWidth;
int curHeight;

int window;
GLuint PROGRAM;

color4 VERTEX_COLORS[8] = {
    color4(0.0, 0.0, 0.0, 1.0), // black
    color4(1.0, 0.0, 0.0, 1.0), // red
    color4(1.0, 1.0, 0.0, 1.0), // yellow
    color4(0.0, 1.0, 0.0, 1.0), // green
    color4(0.0, 0.0, 1.0, 1.0), // blue
    color4(1.0, 0.0, 1.0, 1.0), // magenta
    color4(1.0, 1.0, 1.0, 1.0), // white
    color4(0.0, 1.0, 1.0, 1.0)  // cyan
};

enum BallShape
{
    SPHERE,
    BUNNY,
    NUM_SHAPES
};

enum DrawColor
{
    BLACK,
    RED,
    YELLOW,
    GREEN,
    BLUE,
    MAGENTA,
    WHITE,
    CYAN,
    COLORFUL,
    NUM_DRAW_COLORS
};

enum ShadingMode
{
    NONE,
    GOURAUD,
    PHONG,
    TEXTURE_SHADE_MODE
};

enum MaterialType
{
    PLASTIC,
    SILVER,
    RUBY,
    JADE,
    RUBBER
};

enum LightMovementMode
{
    FIXED,
    MOVE_WITH_OBJECT
};

enum DisplayMode
{
    SHADING,
    TEXTURE,
    WIREFRAME
};

BallShape curBallShape = SPHERE;
DrawColor curDrawColor = COLORFUL;
ShadingMode curShadeMode = GOURAUD;
DisplayMode curDisplayMode = SHADING;
MaterialType curMaterialType = PLASTIC;
LightMovementMode curLightMovementMode = FIXED;

// Allocate space for NUM_SHAPES VAOs and 1 more for the room / walls
GLuint vao[NUM_SHAPES + 1];

// Model-view and projection matrices uniform location
GLuint ModelView, Projection;

GLuint shadingModeLoc;
GLuint texMapLoc;

void loadModel(std::string path, std::vector<point4> &points, std::vector<vec3> &normals);
void loadPPM(std::string path, std::vector<std::vector<color3>> &colors);

// Put object-specific data in namespaces
namespace wallsContext
{
    GLuint buffer;

    // A room is implemented as a cube with the front face missing
    // Hence, there will be 36 - 6 = 30 vertices
    const int NumVertices = 30;

    ShadingMode shadeMode = NONE;

    point4 points[NumVertices];
    color4 colors[NumVertices];

    point4 vertices[8];

    int Index = 0;

    void quad(int a, int b, int c, int d)
    {
        colors[Index] = VERTEX_COLORS[a];
        points[Index] = vertices[a];
        Index++;

        colors[Index] = VERTEX_COLORS[b];
        points[Index] = vertices[b];
        Index++;

        colors[Index] = VERTEX_COLORS[c];
        points[Index] = vertices[c];
        Index++;

        colors[Index] = VERTEX_COLORS[a];
        points[Index] = vertices[a];
        Index++;

        colors[Index] = VERTEX_COLORS[c];
        points[Index] = vertices[c];
        Index++;

        colors[Index] = VERTEX_COLORS[d];
        points[Index] = vertices[d];
        Index++;
    }

    // Room will be redrawn on reshape so need way to update vertices
    void update_vertices()
    {
        point4 new_vertices[8] = {
            point4(leftWallBoundary, bottomWallBoundary, 1.0, 1.0),
            point4(leftWallBoundary, topWallBoundary, 1.0, 1.0),
            point4(rightWallBoundary, topWallBoundary, 1.0, 1.0),
            point4(rightWallBoundary, bottomWallBoundary, 1.0, 1.0),
            point4(leftWallBoundary, bottomWallBoundary, -1.0, 1.0),
            point4(leftWallBoundary, topWallBoundary, -1.0, 1.0),
            point4(rightWallBoundary, topWallBoundary, -1.0, 1.0),
            point4(rightWallBoundary, bottomWallBoundary, -1.0, 1.0),
        };

        for (int i = 0; i < 8; i++)
        {
            vertices[i] = new_vertices[i];
        }
    }

    void colorcube()
    {
        update_vertices();

        quad(6, 7, 3, 2);
        quad(7, 4, 0, 3);
        quad(2, 1, 5, 6);
        quad(7, 6, 5, 4);
        quad(1, 0, 4, 5);

        // Reset Index since colorcube will be called multiple times (to redraw)
        Index = 0;
    }
}

namespace sphereContext
{
    GLuint buffer;

    // Approximate a sphere using recursive subdivision
    const int NumTimesToSubdivide = 7;
    const int NumTriangles = 65536;
    const int NumVertices = 3 * NumTriangles;

    point4 points[NumVertices];
    vec3 normals[NumVertices];
    vec2 texCoords[NumVertices];

    GLuint sphereTextures[2];

    std::string earthTexPath = "earth.ppm";
    std::vector<std::vector<color3>> earthTexImg;

    std::string basketballTexPath = "basketball.ppm";
    std::vector<std::vector<color3>> basketballTexImg;

    int Index = 0;

    void triangle(const point4 &a, const point4 &b, const point4 &c)
    {
        vec3 normal = normalize(cross(b - a, c - b));

        points[Index] = a;
        normals[Index] = normal;
        texCoords[Index] = vec2(0.5 + atan2(a.x, a.z) / (2 * M_PI), 0.5 + asin(a.y) / M_PI);
        Index++;

        points[Index] = b;
        normals[Index] = normal;
        texCoords[Index] = vec2(0.5 + atan2(b.x, b.z) / (2 * M_PI), 0.5 + asin(b.y) / M_PI);
        Index++;

        points[Index] = c;
        normals[Index] = normal;
        texCoords[Index] = vec2(0.5 + atan2(c.x, c.z) / (2 * M_PI), 0.5 + asin(c.y) / M_PI);
        Index++;
    }

    point4 unit(const point4 &p)
    {
        float len = p.x * p.x + p.y * p.y + p.z * p.z;

        point4 t;
        if (len > DivideByZeroTolerance)
        {
            t = p / sqrt(len);
            t.w = 1.0;
        }

        return t;
    }

    void divide_triangle(const point4 &a, const point4 &b,
                         const point4 &c, int count)
    {
        if (count > 0)
        {
            point4 v1 = unit(a + b);
            point4 v2 = unit(a + c);
            point4 v3 = unit(b + c);

            divide_triangle(a, v1, v2, count - 1);
            divide_triangle(c, v2, v3, count - 1);
            divide_triangle(b, v3, v1, count - 1);
            divide_triangle(v1, v3, v2, count - 1);
        }
        else
        {
            triangle(a, b, c);
        }
    }

    void tetrahedron(int count)
    {
        point4 v[4] = {
            vec4(0.0, 0.0, 1.0, 1.0),
            vec4(0.0, 0.942809, -0.333333, 1.0),
            vec4(-0.816497, -0.471405, -0.333333, 1.0),
            vec4(0.816497, -0.471405, -0.333333, 1.0)};

        divide_triangle(v[0], v[1], v[2], count);
        divide_triangle(v[3], v[2], v[1], count);
        divide_triangle(v[0], v[3], v[1], count);
        divide_triangle(v[0], v[2], v[3], count);
    }

    void initTextures()
    {
        loadPPM(earthTexPath, earthTexImg);
        loadPPM(basketballTexPath, basketballTexImg);

        glGenTextures(2, sphereTextures);

        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

        glBindTexture(GL_TEXTURE_2D, sphereTextures[0]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, basketballTexImg[0].size(), basketballTexImg.size(), 0,
                     GL_RGB, GL_UNSIGNED_BYTE, basketballTexImg.data());
        glGenerateMipmap(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, sphereTextures[1]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, earthTexImg[0].size(), earthTexImg.size(), 0,
                     GL_RGB, GL_UNSIGNED_BYTE, earthTexImg.data());
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    void initSphere()
    {
        tetrahedron(NumTimesToSubdivide);
        initTextures();
    }

}

namespace bunnyContext
{
    GLuint buffer;

    int NumVertices;

    std::vector<point4> points;
    std::vector<vec3> normals;

    std::string modelPath = "bunny.off";

    void initBunny()
    {
        loadModel("bunny.off", points, normals);

        NumVertices = points.size();
        // colors.resize(NumVertices);

        // for (int colorIdx = 0; colorIdx < NumVertices; colorIdx++)
        // {
        //     colors[colorIdx] = VERTEX_COLORS[colorIdx % 8];
        // }
    }
}

namespace MaterialInfo
{
    color4 material_ambient;
    color4 material_diffuse;
    color4 material_specular;

    float material_shininess = 100.0;

    color4 ambient_product;
    color4 diffuse_product;
    color4 specular_product;

    void updateMaterial()
    {
        if (curMaterialType == PLASTIC)
        {
            material_ambient = color4(0.0, 0.0, 0.0, 1.0);
            material_diffuse = color4(0.5, 0.5, 0.0, 1.0);
            material_specular = color4(0.60, 0.60, 0.50, 1.0);
            material_shininess = 32.0;
        }
        else if (curMaterialType == SILVER)
        {
            material_ambient = color4(0.19225, 0.19225, 0.19225, 1.0);
            material_diffuse = color4(0.50754, 0.50754, 0.50754, 1.0);
            material_specular = color4(0.508273, 0.508273, 0.508273, 1.0);
            material_shininess = 51.2;
        }
        else if (curMaterialType == RUBY)
        {
            material_ambient = color4(0.1745, 0.01175, 0.01175, 0.55);
            material_diffuse = color4(0.61424, 0.04136, 0.04136, 0.55);
            material_specular = color4(0.727811, 0.626959, 0.626959, 0.55);
            material_shininess = 76.8;
        }
        else if (curMaterialType == JADE)
        {
            material_ambient = color4(0.135, 0.2225, 0.1575, 0.95);
            material_diffuse = color4(0.54, 0.89, 0.63, 0.95);
            material_specular = color4(0.316228, 0.316228, 0.316228, 0.95);
            material_shininess = 12.8;
        }
        else if (curMaterialType == RUBBER)
        {
            material_ambient = color4(0.0, 0.05, 0.05, 1.0);
            material_diffuse = color4(0.4, 0.5, 0.5, 1.0);
            material_specular = color4(0.04, 0.7, 0.7, 1.0);
            material_shininess = 10.0;
        }
    }
}

namespace LightInfo
{
    // Point light source
    // point4 light_position(1.0, 1.0, 0.0, 1.0);

    // Directional light source
    point4 light_position(1.0, 1.0, 0.0, 0.0);

    color4 light_ambient(0.2, 0.2, 0.2, 1.0);
    color4 light_diffuse(1.0, 1.0, 1.0, 1.0);
    color4 light_specular(1.0, 1.0, 1.0, 1.0);

    bool isAmbientOn = true;
    bool isDiffuseOn = true;
    bool isSpecularOn = true;

    void updateLightingComponents()
    {
        color4 ambient_product = isAmbientOn ? light_ambient * MaterialInfo::material_ambient : 0.0;
        color4 diffuse_product = isDiffuseOn ? light_diffuse * MaterialInfo::material_diffuse : 0.0;
        color4 specular_product = isSpecularOn ? light_specular * MaterialInfo::material_specular : 0.0;

        glUniform4fv(glGetUniformLocation(PROGRAM, "AmbientProduct"),
                     1, ambient_product);
        glUniform4fv(glGetUniformLocation(PROGRAM, "DiffuseProduct"),
                     1, diffuse_product);
        glUniform4fv(glGetUniformLocation(PROGRAM, "SpecularProduct"),
                     1, specular_product);

        glUniform4fv(glGetUniformLocation(PROGRAM, "LightPosition"),
                     1, light_position);

        glUniform1f(glGetUniformLocation(PROGRAM, "Shininess"),
                    MaterialInfo::material_shininess);
    }
}

void loadModel(std::string path, std::vector<point4> &points, std::vector<vec3> &normals)
{
    std::string line;
    std::ifstream modelFile(path);

    int numVertices, numTriangles, numEdges;

    if (modelFile.is_open())
    {
        // Skip header (tje line with "OFF")
        std::string header;
        std::getline(modelFile, header);

        // Parse model info
        modelFile >> numVertices >> numTriangles >> numEdges;

        // Container to hold vertices
        std::vector<point4> baseVertices(numVertices, point4(0.0, 0.0, 0.0, 1.0));

        // Need coordinate range to normalize the model coordinates to [-1, 1]
        point4 maxCoord = point4(std::numeric_limits<float>::min());
        point4 minCoord = point4(std::numeric_limits<float>::max());

        float xRange, yRange, zRange;

        for (int vertexIdx = 0; vertexIdx < numVertices; vertexIdx++)
        {
            modelFile >> baseVertices[vertexIdx].x >> baseVertices[vertexIdx].y >> baseVertices[vertexIdx].z;

            maxCoord.x = std::max(maxCoord.x, baseVertices[vertexIdx].x);
            maxCoord.y = std::max(maxCoord.y, baseVertices[vertexIdx].y);
            maxCoord.z = std::max(maxCoord.z, baseVertices[vertexIdx].z);

            minCoord.x = std::min(minCoord.x, baseVertices[vertexIdx].x);
            minCoord.y = std::min(minCoord.y, baseVertices[vertexIdx].y);
            minCoord.z = std::min(minCoord.z, baseVertices[vertexIdx].z);
        }

        xRange = maxCoord.x - minCoord.x;
        yRange = maxCoord.y - minCoord.y;
        zRange = maxCoord.z - minCoord.z;

        for (int vertexIdx = 0; vertexIdx < numVertices; vertexIdx++)
        {
            // Scale vertices to [-1, 1] range
            float scaledX = 2.0f * (baseVertices[vertexIdx].x - minCoord.x) / xRange - 1.0f;
            float scaledY = 2.0f * (baseVertices[vertexIdx].y - minCoord.y) / yRange - 1.0f;
            float scaledZ = 2.0f * (baseVertices[vertexIdx].z - minCoord.z) / zRange - 1.0f;

            baseVertices[vertexIdx].x = scaledX;
            baseVertices[vertexIdx].y = scaledY;
            baseVertices[vertexIdx].z = scaledZ;
        }

        int numFaces, vertIdxX, vertIdxY, vertIdxZ;

        // Parse triangles and append vertices
        for (int triangleIdx = 0; triangleIdx < numTriangles; triangleIdx++)
        {
            modelFile >> numFaces >> vertIdxX >> vertIdxY >> vertIdxZ;

            point4 a = baseVertices[vertIdxX];
            point4 b = baseVertices[vertIdxY];
            point4 c = baseVertices[vertIdxZ];

            points.emplace_back(a);
            points.emplace_back(b);
            points.emplace_back(c);

            vec3 normal = normalize(cross(b - a, c - b));

            normals.emplace_back(normal);
            normals.emplace_back(normal);
            normals.emplace_back(normal);
        }
    }
}

void loadPPM(std::string path, std::vector<std::vector<color3>> &colors)
{
    std::string line;
    std::ifstream ppmFile(path);

    if (ppmFile.is_open())
    {
        std::string header;
        std::getline(ppmFile, header);

        if (header.compare("P3") == 0)
        {
            std::string comment;

            int height, width, maxValue;

            ppmFile >> height >> width >> maxValue;

            for (int i = 0; i < height; i++)
            {
                colors.push_back(std::vector<color3>());

                for (int j = 0; j < width; j++)
                {
                    int R, G, B;

                    ppmFile >> R >> G >> B;

                    float scaled_R = R / float(maxValue);
                    float scaled_G = G / float(maxValue);
                    float scaled_B = B / float(maxValue);

                    colors[i].push_back(color3(scaled_R, scaled_G, scaled_B));
                }
            }
        }
        else
        {
            std::cout << "File is not a PPM file" << std::endl;
        }
    }
}

void menu(int num)
{
    if (num == 0)
    {
        glutDestroyWindow(window);
        exit(0);
    }
    else if (num == 1)
    {
        curShadeMode = GOURAUD;
    }
    else if (num == 2)
    {
        curShadeMode = PHONG;
    }
    else if (num == 3)
    {
        curMaterialType = PLASTIC;
        MaterialInfo::updateMaterial();
        LightInfo::updateLightingComponents();
    }
    else if (num == 4)
    {
        curMaterialType = SILVER;
        MaterialInfo::updateMaterial();
        LightInfo::updateLightingComponents();
    }
    else if (num == 5)
    {
        curMaterialType = RUBY;
        MaterialInfo::updateMaterial();
        LightInfo::updateLightingComponents();
    }
    else if (num == 6)
    {
        curMaterialType = JADE;
        MaterialInfo::updateMaterial();
        LightInfo::updateLightingComponents();
    }
    else if (num == 7)
    {
        curMaterialType = RUBBER;
        MaterialInfo::updateMaterial();
        LightInfo::updateLightingComponents();
    }

    else if (num == 8)
    {
        curDisplayMode = WIREFRAME;

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else if (num == 9)
    {
        if (curDisplayMode == WIREFRAME)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        curDisplayMode = SHADING;
        curShadeMode = GOURAUD;
    }

    else if (num == 10)
    {
        if (curDisplayMode == WIREFRAME)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        curDisplayMode = TEXTURE;
        curShadeMode = TEXTURE_SHADE_MODE;

        glUniform1i(shadingModeLoc, static_cast<int>(curShadeMode));

        glBindTexture(GL_TEXTURE_2D, sphereContext::sphereTextures[0]);
    }
    else if (num == 11)
    {
        if (curDisplayMode == WIREFRAME)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        curDisplayMode = TEXTURE;
        curShadeMode = TEXTURE_SHADE_MODE;

        glUniform1i(shadingModeLoc, static_cast<int>(curShadeMode));

        glBindTexture(GL_TEXTURE_2D, sphereContext::sphereTextures[1]);
    }
    else if (num == 12)
    {
        if (curDisplayMode == WIREFRAME)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        curDisplayMode = TEXTURE;
        curShadeMode = TEXTURE_SHADE_MODE;

        // 1D Texture here
    }

    else if (num == 13)
    {
        LightInfo::isAmbientOn = !LightInfo::isAmbientOn;
        LightInfo::updateLightingComponents();
    }
    else if (num == 14)
    {
        LightInfo::isDiffuseOn = !LightInfo::isDiffuseOn;
        LightInfo::updateLightingComponents();
    }
    else if (num == 15)
    {
        LightInfo::isSpecularOn = !LightInfo::isSpecularOn;
        LightInfo::updateLightingComponents();
    }

    else if (num == 16)
    {
        curLightMovementMode = FIXED;

        LightInfo::light_position = INITIAL_LIGHT_POSITION;
        LightInfo::updateLightingComponents();
    }
    else if (num == 17)
    {

        curLightMovementMode = MOVE_WITH_OBJECT;
    }

    glutPostRedisplay();
}

void createMenu(void)
{
    int shading_mode_submenu = glutCreateMenu(menu);
    glutAddMenuEntry("Gouraud", 1);
    glutAddMenuEntry("Phong", 2);

    int material_type_submenu = glutCreateMenu(menu);
    glutAddMenuEntry("Plastic", 3);
    glutAddMenuEntry("Silver", 4);
    glutAddMenuEntry("Ruby", 5);
    glutAddMenuEntry("Jade", 6);
    glutAddMenuEntry("Rubber", 7);

    int display_mode_submenu = glutCreateMenu(menu);
    glutAddMenuEntry("Wireframe", 8);
    glutAddMenuEntry("Shading", 9);
    glutAddMenuEntry("Texture (Basketball)", 10);
    glutAddMenuEntry("Texture (Earth)", 11);
    glutAddMenuEntry("Texture (1D)", 12);

    int light_components_submenu = glutCreateMenu(menu);
    glutAddMenuEntry("Toggle Ambient", 13);
    glutAddMenuEntry("Toggle Diffuse", 14);
    glutAddMenuEntry("Toggle Specular", 15);

    int light_position_submenu = glutCreateMenu(menu);
    glutAddMenuEntry("Fixed", 16);
    glutAddMenuEntry("Move with Object", 18);

    int menu_id = glutCreateMenu(menu);

    glutAddSubMenu("Shading", shading_mode_submenu);
    glutAddSubMenu("Material", material_type_submenu);
    glutAddSubMenu("Display Mode", display_mode_submenu);
    glutAddSubMenu("Light Components", light_components_submenu);
    glutAddSubMenu("Light Position", light_position_submenu);

    glutAddMenuEntry("Quit", 0);

    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

// For setting the projection matrix when toggling between 2D and 3D
void setProjectionMatrix()
{
    mat4 projection;

    // Calculating the aspect ratio
    GLfloat aspect = (GLfloat)curWidth / (GLfloat)curHeight;

    // Use perspective projection if 3D
    // Orthographic if 2D
    if (is3D)
    {
        projection = Perspective(FOV, aspect, zNear, zFar);
    }
    else
    {
        projection = curWidth <= curHeight ? Ortho(-1.0, 1.0, -1.0 / aspect,
                                                   1.0 / aspect, zNear, zFar)
                                           : Ortho(-1.0 * aspect, 1.0 * aspect, -1.0, 1.0, zNear, zFar);
    }

    glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);
}

void toggleColor(point4 colors[], int numVertices)
{
    // Advance enum to next color
    curDrawColor = DrawColor((curDrawColor + 1) % NUM_DRAW_COLORS);

    // Skip white since white won't be seen (since the background is also white)
    curDrawColor = curDrawColor == WHITE ? DrawColor((curDrawColor + 1) % NUM_DRAW_COLORS) : curDrawColor;

    // Get index from enum (to index into color array)
    int colorIndex = static_cast<int>(curDrawColor);

    // Set color values
    for (int i = 0; i < numVertices; i++)
    {
        colors[i] = (curDrawColor == COLORFUL) ? VERTEX_COLORS[i % 8] : VERTEX_COLORS[colorIndex];
    }
}

// OpenGL initialization
void init()
{
    // Load shaders and use the resulting shader program
    PROGRAM = InitShader("vshader.glsl", "fshader.glsl");

    sphereContext::initSphere();
    bunnyContext::initBunny();
    wallsContext::colorcube();

    GLuint vPosition = glGetAttribLocation(PROGRAM, "vPosition");
    GLuint vColor = glGetAttribLocation(PROGRAM, "vColor");
    GLuint vNormal = glGetAttribLocation(PROGRAM, "vNormal");
    GLuint vTexCoord = glGetAttribLocation(PROGRAM, "vTexCoord");

    // Retrieve transformation uniform variable locations
    ModelView = glGetUniformLocation(PROGRAM, "ModelView");
    Projection = glGetUniformLocation(PROGRAM, "Projection");
    texMapLoc = glGetUniformLocation(PROGRAM, "texMap");

    shadingModeLoc = glGetUniformLocation(PROGRAM, "ShadeMode");

    mat4 projection;
    projection = Perspective(FOV, 1.0, zNear, zFar);

    // Create a vertex array object
    glGenVertexArrays(NUM_SHAPES + 1, vao);

    // Initialization for SPHERE
    glBindVertexArray(vao[0]);

    glEnableVertexAttribArray(vPosition);
    glEnableVertexAttribArray(vNormal);
    glEnableVertexAttribArray(vTexCoord);

    glGenBuffers(1, &sphereContext::buffer);
    glBindBuffer(GL_ARRAY_BUFFER, sphereContext::buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sphereContext::points) + sizeof(sphereContext::normals) + sizeof(sphereContext::texCoords), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sphereContext::points), sphereContext::points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(sphereContext::points), sizeof(sphereContext::normals), sphereContext::normals);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(sphereContext::points) + sizeof(sphereContext::normals), sizeof(sphereContext::texCoords), sphereContext::texCoords);

    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(sphereContext::points)));
    glVertexAttribPointer(vTexCoord, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(sphereContext::points) + sizeof(sphereContext::normals)));

    // Initialization for BUNNY
    glBindVertexArray(vao[1]);

    glEnableVertexAttribArray(vPosition);
    // glEnableVertexAttribArray(vColor);
    glEnableVertexAttribArray(vNormal);

    glGenBuffers(1, &bunnyContext::buffer);
    glBindBuffer(GL_ARRAY_BUFFER, bunnyContext::buffer);
    glBufferData(GL_ARRAY_BUFFER, bunnyContext::points.size() * sizeof(point4) + bunnyContext::normals.size() * sizeof(vec3), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, bunnyContext::points.size() * sizeof(point4), &bunnyContext::points[0]);
    glBufferSubData(GL_ARRAY_BUFFER, bunnyContext::points.size() * sizeof(point4), bunnyContext::normals.size() * sizeof(vec3), &bunnyContext::normals[0]);
    // glBufferData(GL_ARRAY_BUFFER, bunnyContext::points.size() * sizeof(point4) + bunnyContext::colors.size() * sizeof(point4), NULL, GL_STATIC_DRAW);
    // glBufferSubData(GL_ARRAY_BUFFER, 0, bunnyContext::points.size() * sizeof(point4), &bunnyContext::points[0]);
    // glBufferSubData(GL_ARRAY_BUFFER, bunnyContext::points.size() * sizeof(point4), bunnyContext::colors.size() * sizeof(point4), &bunnyContext::colors[0]);

    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(bunnyContext::points.size() * sizeof(point4)));
    // glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(bunnyContext::points.size() * sizeof(point4)));

    // Initialization for WALLS / ROOM
    glBindVertexArray(vao[2]);

    glEnableVertexAttribArray(vPosition);
    glEnableVertexAttribArray(vColor);

    glGenBuffers(1, &wallsContext::buffer);
    glBindBuffer(GL_ARRAY_BUFFER, wallsContext::buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(wallsContext::points) + sizeof(wallsContext::colors), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(wallsContext::points), wallsContext::points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(wallsContext::points), sizeof(wallsContext::colors), wallsContext::colors);

    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(wallsContext::points)));

    MaterialInfo::updateMaterial();
    LightInfo::updateLightingComponents();

    // Set current program object
    glUseProgram(PROGRAM);

    glUniform1i(texMapLoc, 0);

    // Enable hiddden surface removal
    glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);

    // Set state variable "clear color" to clear buffer with.
    glClearColor(1.0, 1.0, 1.0, 1.0);
}

//----------------------------------------------------------------------------

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render the walls first, then the objects
    // Translate back a bit so that scene is visible
    // Scaling should only apply to objects (so (1.0, 1.0, 1.0) is used for the scale matrix)
    mat4 model_view = Translate(vec3(0.0, 0.0, -2.0)) * Scale(1.0, 1.0, 1.0);

    // Draw room
    glBindVertexArray(vao[2]);
    glBindBuffer(GL_ARRAY_BUFFER, wallsContext::buffer);
    glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
    glUniform1i(shadingModeLoc, static_cast<int>(wallsContext::shadeMode));
    glDrawArrays(GL_TRIANGLES, 0, wallsContext::NumVertices);

    // Use different matrices for objects other than the room
    model_view = (Translate(displacement) * Scale(SCALE_FACTOR, SCALE_FACTOR, SCALE_FACTOR));

    glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);

    switch (curBallShape)
    {
    case SPHERE:
        glBindVertexArray(vao[0]);
        glBindBuffer(GL_ARRAY_BUFFER, sphereContext::buffer);
        glUniform1i(shadingModeLoc, static_cast<int>(curShadeMode));
        glDrawArrays(GL_TRIANGLES, 0, sphereContext::NumVertices);
        break;
    case BUNNY:
        glBindVertexArray(vao[1]);
        glBindBuffer(GL_ARRAY_BUFFER, bunnyContext::buffer);

        // Modify and send new ModelView matrix for Bunny
        // Rotate in X-direction
        // Need to do this so BUnny faces camera
        model_view = model_view * RotateX(BUNNY_X_ROTATION_ANGLE);
        glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);

        glUniform1i(shadingModeLoc, static_cast<int>(curShadeMode));
        glDrawArrays(GL_TRIANGLES, 0, bunnyContext::NumVertices);
        break;
    }

    glFlush();
    glutSwapBuffers();
}

//---------------------------------------------------------------------
//
// reshape
//

void reshape(int w, int h)
{
    glViewport(0, 0, w, h);

    curWidth = w;
    curHeight = h;

    // Calculate aspect ratio
    GLfloat aspect = (GLfloat)curWidth / (GLfloat)curHeight;

    // Update wall boundaries depending on width and height
    if (curWidth <= curHeight)
    {
        leftWallBoundary = -1.0;
        rightWallBoundary = 1.0;

        bottomWallBoundary = -1.0 / aspect;
        topWallBoundary = 1.0 / aspect;
    }
    else
    {
        bottomWallBoundary = -1.0;
        topWallBoundary = 1.0;

        leftWallBoundary = -1.0 * aspect;
        rightWallBoundary = 1.0 * aspect;
    }

    // Need to update wall vertices on reshape since room will be scaled
    wallsContext::colorcube();

    // Bind wall buffer send updated vertex data
    glBindVertexArray(vao[3]);
    glBindBuffer(GL_ARRAY_BUFFER, wallsContext::buffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(wallsContext::points), wallsContext::points);

    // Projection matrix may need to be updated
    setProjectionMatrix();
}

//----------------------------------------------------------------------------

void idle(void)
{
    // Ball should bounce off boundaries
    if (displacement.x + curHorizontalSpeed <= leftWallBoundary + BALL_RADIUS)
    {
        displacement.x = leftWallBoundary + BALL_RADIUS;
        curHorizontalSpeed = -curHorizontalSpeed;
    }
    else if (displacement.x + curHorizontalSpeed >= rightWallBoundary - BALL_RADIUS)
    {

        displacement.x = rightWallBoundary - BALL_RADIUS;
        curHorizontalSpeed = -curHorizontalSpeed;
    }

    if (displacement.y + curVerticalSpeed <= bottomWallBoundary + BALL_RADIUS)
    {
        displacement.y = bottomWallBoundary + BALL_RADIUS;
        curVerticalSpeed = -curVerticalSpeed;
    }
    else if (displacement.y + curVerticalSpeed >= topWallBoundary - BALL_RADIUS)
    {
        displacement.y = topWallBoundary - BALL_RADIUS;
        curVerticalSpeed = -curVerticalSpeed;
    }

    // Make ball bounce off front / back walls only if 3D is toggled
    if (is3D)
    {
        if (displacement.z + curZSpeed <= backWallBoundary + BALL_RADIUS)
        {
            displacement.z = backWallBoundary + BALL_RADIUS;
            curZSpeed = -curZSpeed;
        }
        else if (displacement.z + curZSpeed >= frontWallBoundary - BALL_RADIUS)
        {
            displacement.z = frontWallBoundary - BALL_RADIUS;
            curZSpeed = -curZSpeed;
        }
    }

    displacement += vec3(curHorizontalSpeed, curVerticalSpeed, curZSpeed);

    glutPostRedisplay();
}

//----------------------------------------------------------------------------

void keyboard(unsigned char key, int x, int y)
{
    // Reset ball to initial position
    // Reset ball speed
    if (key == 'I' | key == 'i')
    {
        displacement = TOP_LEFT_FRONT_CORNER;
        curHorizontalSpeed = INITIAL_HORIZONTAL_SPEED;
        curVerticalSpeed = INITIAL_VERTICAL_SPEED;
        curZSpeed = INITIAL_Z_SPEED;
    }

    // Toggle between 2D and 3D
    if (key == 'V' | key == 'v')
    {
        is3D = !is3D;
        curZSpeed = is3D ? INITIAL_Z_SPEED : 0;

        setProjectionMatrix();
    }

    // Print input command overview
    if (key == 'H' | key == 'h')
    {
        std::cout << PRINT_DELIMITER << std::endl;
        std::cout << "Press H => Print an overview of input commands" << std::endl;
        std::cout << "Press D => Toggle between solid and wireframe rendering" << std::endl;
        std::cout << "Press C => Toggle between colors" << std::endl;
        std::cout << "Press I => Reset ball to initial position" << std::endl;
        std::cout << "Press V => Toggle between 2D and 3D" << std::endl;
        std::cout << "Press Q => Quit the program" << std::endl;
        std::cout << "Left-mouse click => Toggle between ball shapes" << std::endl;
        std::cout << PRINT_DELIMITER << std::endl;
    }

    // Quit the program
    if (key == 'Q' | key == 'q')
    {
        exit(0);
    }
}

//----------------------------------------------------------------------------

void mouse(int button, int state, int x, int y)
{
    // Toggle betwene shapes on left-mouse click
    if (state == GLUT_DOWN)
    {
        switch (button)
        {
        case GLUT_LEFT_BUTTON:
            curBallShape = BallShape((curBallShape + 1) % NUM_SHAPES);

            if (curBallShape == SPHERE)
            {
                glBindVertexArray(vao[0]);
                glBindBuffer(GL_ARRAY_BUFFER, sphereContext::buffer);
            }
            else if (curBallShape == BUNNY)
            {
                glBindVertexArray(vao[1]);
                glBindBuffer(GL_ARRAY_BUFFER, bunnyContext::buffer);
            }

            break;
        }
    }
}

//----------------------------------------------------------------------------

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(1024, 1024);
    glutInitWindowPosition(50, 50);
    glutCreateWindow("Bouncing Ball");

    glewExperimental = GL_TRUE;
    glewInit();

    init();

    createMenu();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}