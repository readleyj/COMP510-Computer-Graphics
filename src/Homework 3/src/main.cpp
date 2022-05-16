#include "Angel.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

const std::string PRINT_DELIMITER = "------------------------------------------------------";

typedef vec4 color4;
typedef vec4 point4;

const vec3 TOP_LEFT_FRONT_CORNER = vec3(-0.7, 1.0, -2.0);
const GLfloat INITIAL_HORIZONTAL_SPEED = 0.01;
const GLfloat INITIAL_VERTICAL_SPEED = -0.015;
const GLfloat INITIAL_Z_SPEED = -0.01;

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
    CUBE,
    SPHERE,
    BUNNY,
    NUM_SHAPES
};

enum DrawMode
{
    SOLID,
    WIREFRAME,
    NUM_DRAW_MODES
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
    PHONG
};

enum MaterialType
{
    PLASTIC,
    SILVER,
    RUBY,
    JADE,
    RUBBER
};

BallShape curBallShape = SPHERE;
DrawMode curDrawMode = SOLID;
DrawColor curDrawColor = COLORFUL;
ShadingMode curShadeMode = GOURAUD;
MaterialType curMaterialType = PLASTIC;

// Allocate space for NUM_SHAPES VAOs and 1 more for the room / walls
GLuint vao[NUM_SHAPES + 1];

// Model-view and projection matrices uniform location
GLuint ModelView, Projection;

GLuint shadingModeLoc;

void loadModel(std::string path, std::vector<point4> *points);

// Put object-specific data in namespaces
namespace cubeContext
{
    GLuint buffer;

    const int NumVertices = 36;

    point4 points[NumVertices];
    vec3 normals[NumVertices];

    point4 vertices[8] = {
        point4(-1.0, -1.0, 1.0, 1.0),
        point4(-1.0, 1.0, 1.0, 1.0),
        point4(1.0, 1.0, 1.0, 1.0),
        point4(1.0, -1.0, 1.0, 1.0),
        point4(-1.0, -1.0, -1.0, 1.0),
        point4(-1.0, 1.0, -1.0, 1.0),
        point4(1.0, 1.0, -1.0, 1.0),
        point4(1.0, -1.0, -1.0, 1.0),
    };

    int Index = 0;

    void quad(int a, int b, int c, int d)
    {
        vec4 u = vertices[b] - vertices[a];
        vec4 v = vertices[c] - vertices[b];

        vec3 normal = normalize(cross(u, v));

        points[Index] = vertices[a];
        normals[Index] = normal;
        Index++;

        points[Index] = vertices[b];
        normals[Index] = normal;
        Index++;

        points[Index] = vertices[c];
        normals[Index] = normal;
        Index++;

        points[Index] = vertices[a];
        normals[Index] = normal;
        Index++;

        points[Index] = vertices[c];
        normals[Index] = normal;
        Index++;

        points[Index] = vertices[d];
        normals[Index] = normal;
        Index++;
    }

    // generate 12 triangles: 36 vertices and 36 colors
    void colorcube()
    {
        quad(1, 0, 3, 2);
        quad(2, 3, 7, 6);
        quad(3, 0, 4, 7);
        quad(6, 5, 1, 2);
        quad(4, 5, 6, 7);
        quad(5, 4, 0, 1);
    }
}

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

        quad(2, 3, 7, 6);
        quad(3, 0, 4, 7);
        quad(6, 5, 1, 2);
        quad(4, 5, 6, 7);
        quad(5, 4, 0, 1);

        // Reset Index since colorcube will be called multiple times (to redraw)
        Index = 0;
    }
}

namespace sphereContext
{
    GLuint buffer;

    // Approximate a sphere using recursive subdivision

    const int NumTimesToSubdivide = 5;
    const int NumTriangles = 4096;
    const int NumVertices = 3 * NumTriangles;

    point4 points[NumVertices];
    vec3 normals[NumVertices];

    int Index = 0;

    void triangle(const point4 &a, const point4 &b, const point4 &c)
    {
        vec3 normal = normalize(cross(b - a, c - b));

        points[Index] = a;
        normals[Index] = normal;
        Index++;

        points[Index] = b;
        normals[Index] = normal;
        Index++;

        points[Index] = c;
        normals[Index] = normal;
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
}

namespace bunnyContext
{
    GLuint buffer;

    int NumVertices;

    std::vector<point4> points;
    std::vector<color4> colors;

    std::string modelPath = "bunny.off";

    void initBunny()
    {
        loadModel("bunny.off", &points);

        NumVertices = points.size();
        colors.resize(NumVertices);

        for (int colorIdx = 0; colorIdx < NumVertices; colorIdx++)
        {
            colors[colorIdx] = VERTEX_COLORS[colorIdx % 8];
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

    //     PLASTIC,
    //     SILVER,
    //     RUBY,
    //     EMERALD,
    //     RUBBER

    void updateMaterial()
    {
        if (curMaterialType == PLASTIC)
        {
            material_ambient = color4(0.0, 0.1, 0.06, 1.0);
            material_diffuse = color4(0.0, 0.50980392, 0.50980392, 1.0);
            material_specular = color4(0.50196078, 0.50196078, 0.50196078, 1.0);
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

        ambient_product = LightInfo::light_ambient * material_ambient;
        diffuse_product = LightInfo::light_diffuse * material_diffuse;
        specular_product = LightInfo::light_specular * material_specular;

        glUniform4fv(glGetUniformLocation(PROGRAM, "AmbientProduct"),
                     1, ambient_product);
        glUniform4fv(glGetUniformLocation(PROGRAM, "DiffuseProduct"),
                     1, diffuse_product);
        glUniform4fv(glGetUniformLocation(PROGRAM, "SpecularProduct"),
                     1, specular_product);

        glUniform4fv(glGetUniformLocation(PROGRAM, "LightPosition"),
                     1, LightInfo::light_position);

        glUniform1f(glGetUniformLocation(PROGRAM, "Shininess"),
                    material_shininess);
    }
}

void loadModel(std::string path, std::vector<point4> *points)
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

            points->emplace_back(baseVertices[vertIdxX]);
            points->emplace_back(baseVertices[vertIdxY]);
            points->emplace_back(baseVertices[vertIdxZ]);
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
    }
    else if (num == 4)
    {
        curMaterialType = SILVER;
        MaterialInfo::updateMaterial();
    }
    else if (num == 5)
    {
        curMaterialType = RUBY;
        MaterialInfo::updateMaterial();
    }
    else if (num == 6)
    {
        curMaterialType = JADE;
        MaterialInfo::updateMaterial();
    }
    else if (num == 7)
    {
        curMaterialType = RUBBER;
        MaterialInfo::updateMaterial();
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
    glutAddMenuEntry("Wireframe", 3);
    glutAddMenuEntry("Shading", 4);
    glutAddMenuEntry("Texture", 5);

    int menu_id = glutCreateMenu(menu);

    glutAddSubMenu("Shading", shading_mode_submenu);
    glutAddSubMenu("Material", material_type_submenu);
    glutAddSubMenu("Display Mode", display_mode_submenu);
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
    cubeContext::colorcube();
    sphereContext::tetrahedron(sphereContext::NumTimesToSubdivide);
    bunnyContext::initBunny();
    wallsContext::colorcube();

    // Load shaders and use the resulting shader program
    PROGRAM = InitShader("vshader.glsl", "fshader.glsl");

    GLuint vPosition = glGetAttribLocation(PROGRAM, "vPosition");
    GLuint vColor = glGetAttribLocation(PROGRAM, "vColor");
    GLuint vNormal = glGetAttribLocation(PROGRAM, "vNormal");

    // Retrieve transformation uniform variable locations
    ModelView = glGetUniformLocation(PROGRAM, "ModelView");
    Projection = glGetUniformLocation(PROGRAM, "Projection");

    shadingModeLoc = glGetUniformLocation(PROGRAM, "ShadeMode");

    mat4 projection;
    projection = Perspective(FOV, 1.0, zNear, zFar);

    // Create a vertex array object
    glGenVertexArrays(NUM_SHAPES + 1, vao);

    // Initialization for CUBE
    glBindVertexArray(vao[0]);

    glEnableVertexAttribArray(vPosition);
    glEnableVertexAttribArray(vNormal);

    glGenBuffers(1, &cubeContext::buffer);
    glBindBuffer(GL_ARRAY_BUFFER, cubeContext::buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeContext::points) + sizeof(cubeContext::normals) + sizeof(cubeContext::normals), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(cubeContext::points), cubeContext::points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(cubeContext::points), sizeof(cubeContext::normals), cubeContext::normals);

    glUniform1i(shadingModeLoc, static_cast<int>(curShadeMode));

    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(cubeContext::points)));

    // Initialization for SPHERE
    glBindVertexArray(vao[1]);

    glEnableVertexAttribArray(vPosition);
    glEnableVertexAttribArray(vNormal);

    glGenBuffers(1, &sphereContext::buffer);
    glBindBuffer(GL_ARRAY_BUFFER, sphereContext::buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sphereContext::points) + sizeof(sphereContext::normals) + sizeof(sphereContext::normals), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(sphereContext::points), sphereContext::points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(sphereContext::points), sizeof(sphereContext::normals), sphereContext::normals);

    glUniform1i(shadingModeLoc, static_cast<int>(curShadeMode));

    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(sphereContext::points)));

    // Initialization for BUNNY
    glBindVertexArray(vao[2]);

    glEnableVertexAttribArray(vPosition);
    glEnableVertexAttribArray(vColor);

    glGenBuffers(1, &bunnyContext::buffer);
    glBindBuffer(GL_ARRAY_BUFFER, bunnyContext::buffer);
    glBufferData(GL_ARRAY_BUFFER, bunnyContext::points.size() * sizeof(point4) + bunnyContext::colors.size() * sizeof(point4), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, bunnyContext::points.size() * sizeof(point4), &bunnyContext::points[0]);
    glBufferSubData(GL_ARRAY_BUFFER, bunnyContext::points.size() * sizeof(point4), bunnyContext::colors.size() * sizeof(point4), &bunnyContext::colors[0]);

    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(bunnyContext::points.size() * sizeof(point4)));

    // Initialization for WALLS / ROOM
    glBindVertexArray(vao[3]);

    glEnableVertexAttribArray(vPosition);
    glEnableVertexAttribArray(vColor);

    glGenBuffers(1, &wallsContext::buffer);
    glBindBuffer(GL_ARRAY_BUFFER, wallsContext::buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(wallsContext::points) + sizeof(wallsContext::colors), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(wallsContext::points), wallsContext::points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(wallsContext::points), sizeof(wallsContext::colors), wallsContext::colors);

    glUniform1i(shadingModeLoc, static_cast<int>(curShadeMode));

    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(wallsContext::points)));

    MaterialInfo::updateMaterial();

    // Set current program object
    glUseProgram(PROGRAM);

    // Enable hiddden surface removal
    glEnable(GL_DEPTH_TEST);

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
    glBindVertexArray(vao[3]);
    glBindBuffer(GL_ARRAY_BUFFER, wallsContext::buffer);
    glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);
    glUniform1i(shadingModeLoc, static_cast<int>(wallsContext::shadeMode));
    glDrawArrays(GL_TRIANGLES, 0, wallsContext::NumVertices);

    // Use different matrices for objects other than the room
    model_view = (Translate(displacement) * Scale(SCALE_FACTOR, SCALE_FACTOR, SCALE_FACTOR));

    glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);

    switch (curBallShape)
    {
    case CUBE:
        glBindVertexArray(vao[0]);
        glBindBuffer(GL_ARRAY_BUFFER, cubeContext::buffer);
        glUniform1i(shadingModeLoc, static_cast<int>(curShadeMode));
        glDrawArrays(GL_TRIANGLES, 0, cubeContext::NumVertices);
        break;
    case SPHERE:
        glBindVertexArray(vao[1]);
        glBindBuffer(GL_ARRAY_BUFFER, sphereContext::buffer);
        glUniform1i(shadingModeLoc, static_cast<int>(curShadeMode));
        glDrawArrays(GL_TRIANGLES, 0, sphereContext::NumVertices);
        break;
    case BUNNY:
        glBindVertexArray(vao[2]);
        glBindBuffer(GL_ARRAY_BUFFER, bunnyContext::buffer);

        // Modify and send new ModelView matrix for Bunny
        // Rotate in X-direction
        // Need to do this so BUnny faces camera
        model_view = model_view * RotateX(BUNNY_X_ROTATION_ANGLE);
        glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view);

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
    // Toggle draw mode (SOLID or WIREFRAME)
    if (key == 'D' | key == 'd')
    {
        curDrawMode = DrawMode((curDrawMode + 1) % NUM_DRAW_MODES);

        if (curDrawMode == SOLID)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        else if (curDrawMode == WIREFRAME)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
    }

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

            if (curBallShape == CUBE)
            {
                glBindVertexArray(vao[0]);
                glBindBuffer(GL_ARRAY_BUFFER, cubeContext::buffer);
            }
            else if (curBallShape == SPHERE)
            {
                glBindVertexArray(vao[1]);
                glBindBuffer(GL_ARRAY_BUFFER, sphereContext::buffer);
            }
            else if (curBallShape == BUNNY)
            {
                glBindVertexArray(vao[2]);
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