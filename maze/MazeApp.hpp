#pragma once

#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions_4_5_Core>
#include <QElapsedTimer>
#include <vector>
#include <list>
#include <algorithm>

#include <qvr/app.hpp>
#include <qvr/device.hpp>

enum class GridCell : int
{
    EMPTY,
    WALL,
    FINISH,
    SPAWN,
    COIN,
    DOOR
};

struct Point
{
    float x;
    float y;

    Point(float x, float y)
        :x(x), y(y)
    {

    }
};

struct RenderObject
{
    Point position = Point(0.0,0.0);
    GridCell type;
};

struct Node
{
    RenderObject data;
    int axis;
    int depth;
    float border;
    float centerX, centerY;
    float xMin, xMax;
    float yMin, yMax;
    Node* left = nullptr;
    Node* right = nullptr;
    Node* parent = nullptr;
    bool isLeaf;
    bool visible;
    bool renderedThisFrame=false;
};

class OcclusionQuery : protected QOpenGLFunctions_4_5_Core
{
private:
    static constexpr GLfloat wallVertices[] = {
        -1.0f, +1.0f, +1.0f,   +1.0f, +1.0f, +1.0f,   +1.0f, -1.0f, +1.0f,   -1.0f, -1.0f, +1.0f, // front
        -1.0f, +1.0f, -1.0f,   +1.0f, +1.0f, -1.0f,   +1.0f, -1.0f, -1.0f,   -1.0f, -1.0f, -1.0f, // back
        +1.0f, +1.0f, -1.0f,   +1.0f, +1.0f, +1.0f,   +1.0f, -1.0f, +1.0f,   +1.0f, -1.0f, -1.0f, // right
        -1.0f, +1.0f, -1.0f,   -1.0f, +1.0f, +1.0f,   -1.0f, -1.0f, +1.0f,   -1.0f, -1.0f, -1.0f, // left
        -1.0f, +1.0f, +1.0f,   +1.0f, +1.0f, +1.0f,   +1.0f, +1.0f, -1.0f,   -1.0f, +1.0f, -1.0f, // top
        -1.0f, -1.0f, +1.0f,   +1.0f, -1.0f, +1.0f,   +1.0f, -1.0f, -1.0f,   -1.0f, -1.0f, -1.0f  // bottom
    };

    static constexpr GLfloat wallNormals[] = {
        0.0f, 0.0f, +1.0f,   0.0f, 0.0f, +1.0f,   0.0f, 0.0f, +1.0f,   0.0f, 0.0f, +1.0f, // front
        0.0f, 0.0f, -1.0f,   0.0f, 0.0f, -1.0f,   0.0f, 0.0f, -1.0f,   0.0f, 0.0f, -1.0f, // back
        +1.0f, 0.0f, 0.0f,   +1.0f, 0.0f, 0.0f,   +1.0f, 0.0f, 0.0f,   +1.0f, 0.0f, 0.0f, // right
        -1.0f, 0.0f, 0.0f,   -1.0f, 0.0f, 0.0f,   -1.0f, 0.0f, 0.0f,   -1.0f, 0.0f, 0.0f, // left
        0.0f, +1.0f, 0.0f,   0.0f, +1.0f, 0.0f,   0.0f, +1.0f, 0.0f,   0.0f, +1.0f, 0.0f, // top
        0.0f, -1.0f, 0.0f,   0.0f, -1.0f, 0.0f,   0.0f, -1.0f, 0.0f,   0.0f, -1.0f, 0.0f  // bottom
    };

    static constexpr GLuint wallIndices[] = {
        0, 3, 1, 1, 3, 2, // front face
        4, 5, 7, 5, 6, 7, // back face
        8, 9, 11, 9, 10, 11, // right face
        12, 15, 13, 13, 15, 14, // left face
        16, 17, 19, 17, 18, 19, // top face
        20, 23, 21, 21, 23, 22, // bottom face
    };

    static GLuint vao;
    static QOpenGLShaderProgram prg;
    static unsigned int vaoIndices;
    GLuint queryId;
    Node* node;
public:
    OcclusionQuery(Node* node)
    {
        initializeOpenGLFunctions();
        this->node = node;
        glGenQueries(1, &queryId);
        if (vao == 0) {
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);
            GLuint positionBuf;
            glGenBuffers(1, &positionBuf);
            glBindBuffer(GL_ARRAY_BUFFER, positionBuf);
            glBufferData(GL_ARRAY_BUFFER, sizeof(wallVertices), wallVertices, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
            glEnableVertexAttribArray(0);
            GLuint normalBuf;
            glGenBuffers(1, &normalBuf);
            glBindBuffer(GL_ARRAY_BUFFER, normalBuf);
            glBufferData(GL_ARRAY_BUFFER, sizeof(wallNormals), wallNormals, GL_STATIC_DRAW);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, 0, 0);
            glEnableVertexAttribArray(1);
            GLuint indexBuf;
            glGenBuffers(1, &indexBuf);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuf);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(wallIndices), wallIndices, GL_STATIC_DRAW);
            vaoIndices = 36;
            prg.addShaderFromSourceFile(QOpenGLShader::Vertex, ":vertex-shader.glsl");
            prg.addShaderFromSourceFile(QOpenGLShader::Fragment, ":fragment-shader.glsl");
            if (!prg.link()) {
                qCritical("Could not link program! Check shaders!");
            }
        }
    }

    ~OcclusionQuery()
    {
        glDeleteQueries(1, &queryId);
    }

    void start(const QMatrix4x4& projectionMatrix, const QMatrix4x4& viewMatrix)
    {
        glBeginQuery(GL_ANY_SAMPLES_PASSED_CONSERVATIVE, queryId);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        glBindVertexArray(vao);
        glUseProgram(prg.programId());
        QMatrix4x4 modelMatrix;
        if (!node->isLeaf) {
            float scaleX = (node->xMax - node->xMin)/2.0f;
            float scaleY = (node->yMax - node->yMin)/2.0f;
            modelMatrix.translate(node->centerX, 1.0f, node->centerY);
            modelMatrix.scale(scaleX, 1.0f, scaleY);
        } else {
            float x = node->data.position.x;
            float y = node->data.position.y;
            modelMatrix.translate(x, 1.0f, y);
        }
        QMatrix4x4 modelViewMatrix = viewMatrix * modelMatrix;
        prg.setUniformValue("projection_matrix", projectionMatrix);
        prg.setUniformValue("modelview_matrix", modelViewMatrix);
        prg.setUniformValue("view_matrix", viewMatrix);
        prg.setUniformValue("normal_matrix", modelViewMatrix.normalMatrix());
        prg.setUniformValue("color", QVector3D(0.0f, 1.0f, 0.0f));
        glDrawElements(GL_TRIANGLES, vaoIndices, GL_UNSIGNED_INT, 0);
        glEndQuery(GL_ANY_SAMPLES_PASSED_CONSERVATIVE);
        //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    bool isAvailable()
    {
        GLuint available;
        glGetQueryObjectuiv(queryId, GL_QUERY_RESULT_AVAILABLE, &available);
        return available == GL_TRUE;
    }

    bool getResult()
    {
        GLuint visible;
        glGetQueryObjectuiv(queryId, GL_QUERY_RESULT, &visible);
        
        return visible;
    }

    Node* getNode()
    {
        return node;
    }
};

void calcBorders(Node* root);

Node* kdTree(std::vector<RenderObject>& objects, int depth = 0);
void pullUp(Node* node);

template<typename Func>
void frontToBack(Node* root, QVector3D eye, Func f)
{
    if (root == nullptr) return;

    bool stop = f(root);
    int axis = root->axis;
    bool positive;
    if (axis == 0) {
        positive = eye.x() < root->border;
    } else {
        positive = eye.y() < root->border;
    }
    if (positive && !stop) {
        frontToBack<Func>(root->left, eye, f);
        frontToBack<Func>(root->right, eye, f);
    } else if(!positive && !stop){
        frontToBack<Func>(root->right, eye, f);
        frontToBack<Func>(root->left, eye, f);
    }
}

template<typename Func>
void inOrder(Node* root, Func f)
{
    if (root == nullptr) return;

    inOrder<Func>(root->left, f);
    f(root);
    inOrder<Func>(root->right, f);
}

void freeTree(Node* root);

class MazeApp : public QVRApp, protected QOpenGLFunctions_4_5_Core
{
private:
    /* Data not directly relevant for rendering */
    bool _wantExit;             // do we want to exit the app?
    QElapsedTimer _timer;       // used for rotating the box

    /* Static data for rendering, initialized per process. */
    unsigned int _fbo;          // Framebuffer object to render into
    unsigned int _fboDepthTex;  // Depth attachment for the FBO
    unsigned int _vaoWall;          // Vertex array object for the box
    unsigned int _vaoIndicesWall;   // Number of indices to render for the box
    unsigned int _vaoFloor;
    unsigned int _vaoIndicesFloor;
    unsigned int _vaoCoin;
    unsigned int _coinSize;
    QOpenGLShaderProgram _prg;  // Shader program for rendering
    GridCell* mazeGrid;    // 0 = nothing, 1 = wall, 2 = finish, (3 = spawn)
    size_t gridWidth;
    size_t gridHeight;
    int coinsLeft = 0;
    float coinBoundingSphere = 0;
    float coinRotation = 0.0f;
    bool frustumCulling = false;
    bool occlusionCullingCHC = false;
    bool occlusionCulling = false; 
    bool chcDebug = false;
    int debugLevel = 0;
    bool forwardPressed = false;
    bool leftPressed = false;
    bool rightPressed = false;
    bool backwardPressed = false;
    QPoint mousePosLastFrame;
    QVector2D mouseDx;
    std::vector<RenderObject> renderQueue;
    std::vector<OcclusionQuery*> vQueries;
    std::vector<OcclusionQuery*> iQueries;
    Node* kdTreeRoot;

public:
    MazeApp();

    // override functions 
    bool initProcess(QVRProcess* p) override;

    void render(QVRWindow* w, const QVRRenderContext& c, const unsigned int* textures) override;

    void update(const QList<QVRObserver*>& observers) override;

    bool wantExit() override;

    void serializeDynamicData(QDataStream& ds) const override;
    void deserializeDynamicData(QDataStream& ds) override;

    void keyPressEvent(const QVRRenderContext& context, QKeyEvent* event) override;
    void keyReleaseEvent(const QVRRenderContext& context, QKeyEvent* event) override;
    void deviceButtonPressEvent(QVRDeviceEvent* event) override;
    void deviceButtonReleaseEvent(QVRDeviceEvent* event) override;
    void deviceAnalogChangeEvent(QVRDeviceEvent* event) override;
    void mouseMoveEvent(const QVRRenderContext& context, QMouseEvent* event) override;

    void exitProcess(QVRProcess* p) override;

    // custom functions
    GridCell GetCell(int row, int col)
    {
        return mazeGrid[row * gridWidth + col];
    }
};
