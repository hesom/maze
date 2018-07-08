#include <iostream>
#include <queue>

#include <QGuiApplication>
#include <QKeyEvent>
#include <QMessageBox>

#include <qvr/manager.hpp>
#include <qvr/window.hpp>
#include <qvr/observer.hpp>
#include <qvr/device.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "MazeApp.hpp"


MazeApp::MazeApp() :
    _wantExit(false)
{
    _timer.start();
}

bool MazeApp::initProcess(QVRProcess* p)
{
    /*if (!gladLoadGL()) {
        qCritical("Could not load GL functions with glad");
    }*/

    initializeOpenGLFunctions();

    int mazeWidth, mazeHeight, channels;
    // load maze layout
    unsigned char* mazeImage = stbi_load("maze.bmp", &mazeWidth, &mazeHeight, &channels, 0);
    if (!mazeImage) {
        qCritical("Could not load maze layout");
    }
    mazeGrid = new GridCell[mazeWidth * mazeHeight];
    renderQueue.reserve(mazeWidth * mazeHeight);
    gridHeight = mazeHeight;
    gridWidth = mazeWidth;
    for (int cell = 0; cell < gridHeight * gridWidth; cell++) {
        // map bmp color to cell type
        // white => empty, red => wall, green => finish, black => spawn
        bool red, green, blue;
        red = mazeImage[channels*cell + 0];
        green = mazeImage[channels * cell + 1];
        blue = mazeImage[channels * cell + 2];

        if (red && green && blue) {
            mazeGrid[cell] = GridCell::EMPTY;
        } else if (red && !green && !blue) {
            mazeGrid[cell] = GridCell::WALL;
        } else if (!red && green && !blue) {
            mazeGrid[cell] = GridCell::FINISH;
        } else if (!red && !green && !blue) {
            mazeGrid[cell] = GridCell::SPAWN;
        } else if (red && green && !blue) {
            mazeGrid[cell] = GridCell::COIN;
            coinsLeft++;
        } else if (!red && !green && blue) {
            mazeGrid[cell] = GridCell::DOOR;
        }
    }
    stbi_image_free(mazeImage);

    // fill render queue
    for (int row = 0; row < gridWidth; row++) {
        for (int col = 0; col < gridHeight; col++) {
            RenderObject object;
            float x = -((float)gridWidth)+1.0f + 2.0f * col;
            float y = ((float)gridHeight)-1.0f - 2.0f * row;
            object.position = Point(x, y);
            object.type = GetCell(row, col);
            renderQueue.push_back(object);
        }
    }

    kdTreeRoot = kdTree(renderQueue);
    calcBorders(kdTreeRoot);

    // Framebuffer object
    glGenFramebuffers(1, &_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glGenTextures(1, &_fboDepthTex);
    glBindTexture(GL_TEXTURE_2D, _fboDepthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1, 1,
            0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _fboDepthTex, 0);

    // Vertex array object
    static const GLfloat wallVertices[] = {
        -1.0f, +1.0f, +1.0f,   +1.0f, +1.0f, +1.0f,   +1.0f, -1.0f, +1.0f,   -1.0f, -1.0f, +1.0f, // front
        -1.0f, +1.0f, -1.0f,   +1.0f, +1.0f, -1.0f,   +1.0f, -1.0f, -1.0f,   -1.0f, -1.0f, -1.0f, // back
        +1.0f, +1.0f, -1.0f,   +1.0f, +1.0f, +1.0f,   +1.0f, -1.0f, +1.0f,   +1.0f, -1.0f, -1.0f, // right
        -1.0f, +1.0f, -1.0f,   -1.0f, +1.0f, +1.0f,   -1.0f, -1.0f, +1.0f,   -1.0f, -1.0f, -1.0f, // left
        -1.0f, +1.0f, +1.0f,   +1.0f, +1.0f, +1.0f,   +1.0f, +1.0f, -1.0f,   -1.0f, +1.0f, -1.0f, // top
        -1.0f, -1.0f, +1.0f,   +1.0f, -1.0f, +1.0f,   +1.0f, -1.0f, -1.0f,   -1.0f, -1.0f, -1.0f  // bottom
    };

    static const GLfloat wallNormals[] = {
        0.0f, 0.0f, +1.0f,   0.0f, 0.0f, +1.0f,   0.0f, 0.0f, +1.0f,   0.0f, 0.0f, +1.0f, // front
        0.0f, 0.0f, -1.0f,   0.0f, 0.0f, -1.0f,   0.0f, 0.0f, -1.0f,   0.0f, 0.0f, -1.0f, // back
        +1.0f, 0.0f, 0.0f,   +1.0f, 0.0f, 0.0f,   +1.0f, 0.0f, 0.0f,   +1.0f, 0.0f, 0.0f, // right
        -1.0f, 0.0f, 0.0f,   -1.0f, 0.0f, 0.0f,   -1.0f, 0.0f, 0.0f,   -1.0f, 0.0f, 0.0f, // left
        0.0f, +1.0f, 0.0f,   0.0f, +1.0f, 0.0f,   0.0f, +1.0f, 0.0f,   0.0f, +1.0f, 0.0f, // top
        0.0f, -1.0f, 0.0f,   0.0f, -1.0f, 0.0f,   0.0f, -1.0f, 0.0f,   0.0f, -1.0f, 0.0f  // bottom
    };

    static const GLuint wallIndices[] = {
        0, 3, 1, 1, 3, 2, // front face
        4, 5, 7, 5, 6, 7, // back face
        8, 9, 11, 9, 10, 11, // right face
        12, 15, 13, 13, 15, 14, // left face
        16, 17, 19, 17, 18, 19, // top face
        20, 23, 21, 21, 23, 22, // bottom face
    };

    static const GLfloat floorVertices[] = {
        -1.0f, -1.0f, +1.0f,   +1.0f, -1.0f, +1.0f,   +1.0f, -1.0f, -1.0f,   -1.0f, -1.0f, -1.0f  // bottom
    };
    static const GLfloat floorNormals[] = {
        0.0f, 1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f, 0.0f  // bottom
    };
    static const GLuint floorIndices[] = {
        0, 1, 3, 1, 2, 3 // front face
    };


    glGenVertexArrays(1, &_vaoWall);
    glBindVertexArray(_vaoWall);
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
    _vaoIndicesWall = 36;

    glGenVertexArrays(1, &_vaoFloor);
    glBindVertexArray(_vaoFloor);
    GLuint positionBufFloor;
    glGenBuffers(1, &positionBufFloor);
    glBindBuffer(GL_ARRAY_BUFFER, positionBufFloor);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorVertices), floorVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    GLuint normalBufFloor;
    glGenBuffers(1, &normalBufFloor);
    glBindBuffer(GL_ARRAY_BUFFER, normalBufFloor);
    glBufferData(GL_ARRAY_BUFFER, sizeof(floorNormals), floorNormals, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, 0, 0);
    glEnableVertexAttribArray(1);
    GLuint indexBufFloor;
    glGenBuffers(1, &indexBufFloor);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufFloor);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(floorIndices), floorIndices, GL_STATIC_DRAW);
    _vaoIndicesFloor = 6;

    std::string inputfile = "goldCoin.wavefront";
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string err;
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, inputfile.c_str());
    if (!err.empty()) {
        std::cerr << err << std::endl;
    }

    if (!ret) {
        __debugbreak();
    }

    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<unsigned int> indices;

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            float vx = attrib.vertices[3 * index.vertex_index + 0];
            float vy = attrib.vertices[3 * index.vertex_index + 1];
            float vz = attrib.vertices[3 * index.vertex_index + 2];
            float nx = attrib.normals[3 * index.normal_index + 0];
            float ny = attrib.normals[3 * index.normal_index + 1];
            float nz = attrib.normals[3 * index.normal_index + 2];

            vertices.push_back(vx);
            vertices.push_back(vy);
            vertices.push_back(vz);
            normals.push_back(nx);
            normals.push_back(ny);
            normals.push_back(nz);
            
            indices.push_back(indices.size());
        }
    }

    for (const auto& coord : vertices) {
        if (std::abs(coord) > coinBoundingSphere) {
            coinBoundingSphere = std::abs(coord);
        }
    }

    coinBoundingSphere *= 2.0f;

    glGenVertexArrays(1, &_vaoCoin);
    glBindVertexArray(_vaoCoin);
    GLuint coinPosBuffer;
    glGenBuffers(1, &coinPosBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, coinPosBuffer);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    GLuint coinNormalBuffer;
    glGenBuffers(1, &coinNormalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, coinNormalBuffer);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(float), normals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_TRUE, 0, 0);
    glEnableVertexAttribArray(1);

    GLuint coinIndexBuffer;
    glGenBuffers(1, &coinIndexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, coinIndexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    _coinSize = indices.size();

    // Shader program
    _prg.addShaderFromSourceFile(QOpenGLShader::Vertex, ":vertex-shader.glsl");
    _prg.addShaderFromSourceFile(QOpenGLShader::Fragment, ":fragment-shader.glsl");
    if (!_prg.link()) {
        qCritical("Could not link program! Check shaders!");
    }

    mousePosLastFrame = QCursor::pos();

    return true;
}

void MazeApp::render(QVRWindow*  w ,
        const QVRRenderContext& context, const unsigned int* textures)
{
    for (int view = 0; view < context.viewCount(); view++) {
        // Get view dimensions
        int width = context.textureSize(view).width();
        int height = context.textureSize(view).height();

        // Set up framebuffer object to render into
        glBindTexture(GL_TEXTURE_2D, _fboDepthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height,
                0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[view], 0);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        QMatrix4x4 projectionMatrix;
        QMatrix4x4 viewMatrix;
        QVector3D eye = context.navigationPosition();

        glUseProgram(_prg.programId());

        if (w->id() == "debug") {
            projectionMatrix.ortho(-40.0f, 40.0f, -40.0f, 40.0f, 0.1f, 100.0f);
            _prg.setUniformValue("projection_matrix", projectionMatrix);
            viewMatrix.lookAt(QVector3D(0.0f, 10.0f, 0.0f), QVector3D(0.0f, 0.0f, 0.0f), QVector3D(-1.0f, 0.0f, 0.0f));
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            glDepthMask(GL_TRUE);
            glEnable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            inOrder(kdTreeRoot, [&](Node* node) {
                if (node->renderedThisFrame) {
                    // immediately render
                    auto cell = node->data.type;
                    float x = node->data.position.x;
                    float y = node->data.position.y;
                    QMatrix4x4 modelMatrix;
                    modelMatrix.translate(x, 1.0f, y);
                    QMatrix4x4 modelViewMatrix = viewMatrix * modelMatrix;
                    _prg.setUniformValue("modelview_matrix", modelViewMatrix);
                    _prg.setUniformValue("view_matrix", viewMatrix);
                    _prg.setUniformValue("normal_matrix", modelViewMatrix.normalMatrix());

                    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                    glDepthMask(GL_TRUE);
                    glEnable(GL_DEPTH_TEST);
                    if (cell == GridCell::WALL) {
                        _prg.setUniformValue("color", QVector3D(1.0f, 0.0f, 0.0f));
                        glBindVertexArray(_vaoWall);
                        glDrawElements(GL_TRIANGLES, _vaoIndicesWall, GL_UNSIGNED_INT, 0);
                    } else if (cell == GridCell::EMPTY) {
                        _prg.setUniformValue("color", QVector3D(0.5f, 0.5f, 0.5f));
                        glBindVertexArray(_vaoFloor);
                        glDrawElements(GL_TRIANGLES, _vaoIndicesFloor, GL_UNSIGNED_INT, 0);
                    } else if (cell == GridCell::FINISH) {
                        _prg.setUniformValue("color", QVector3D(0.0f, 1.0f, 0.0f));
                        glBindVertexArray(_vaoFloor);
                        glDrawElements(GL_TRIANGLES, _vaoIndicesFloor, GL_UNSIGNED_INT, 0);
                    } else if (cell == GridCell::SPAWN) {
                        _prg.setUniformValue("color", QVector3D(0.7f, 0.7f, 0.0f));
                        glBindVertexArray(_vaoFloor);
                        glDrawElements(GL_TRIANGLES, _vaoIndicesFloor, GL_UNSIGNED_INT, 0);
                    } else if (cell == GridCell::COIN) {
                        _prg.setUniformValue("color", QVector3D(0.5f, 0.5f, 0.5f));
                        glBindVertexArray(_vaoFloor);
                        glDrawElements(GL_TRIANGLES, _vaoIndicesFloor, GL_UNSIGNED_INT, 0);
                        modelMatrix.setToIdentity();
                        modelMatrix.translate(x, 1.0f, y);
                        modelMatrix.scale(2.0f);
                        modelViewMatrix = viewMatrix * modelMatrix;
                        _prg.setUniformValue("modelview_matrix", modelViewMatrix);
                        _prg.setUniformValue("view_matrix", viewMatrix);
                        _prg.setUniformValue("normal_matrix", modelViewMatrix.normalMatrix());
                        _prg.setUniformValue("color", QVector3D(1.0f, 1.0f, 0.0f));
                        glBindVertexArray(_vaoCoin);
                        glDrawElements(GL_TRIANGLES, _coinSize, GL_UNSIGNED_INT, 0);
                    }
                }

                if (chcDebug && node->depth == debugLevel && node->visible) {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                    QMatrix4x4 modelMatrix;
                    if (!node->isLeaf) {
                        float scaleX = (node->xMax - node->xMin) / 2.0f;
                        float scaleY = (node->yMax - node->yMin) / 2.0f;
                        modelMatrix.translate(node->centerX, 10.0f, node->centerY);
                        modelMatrix.scale(scaleX, 1.0f, scaleY);
                    } else {
                        float x = node->data.position.x;
                        float y = node->data.position.y;
                        modelMatrix.translate(x, 10.0f, y);
                    }
                    QMatrix4x4 modelViewMatrix = viewMatrix * modelMatrix;
                    _prg.setUniformValue("projection_matrix", projectionMatrix);
                    _prg.setUniformValue("modelview_matrix", modelViewMatrix);
                    _prg.setUniformValue("view_matrix", viewMatrix);
                    _prg.setUniformValue("normal_matrix", modelViewMatrix.normalMatrix());
                    _prg.setUniformValue("color", QVector3D(0.0f, 1.0f, 0.0f));
                    glBindVertexArray(_vaoWall);
                    glDrawElements(GL_TRIANGLES, _vaoIndicesWall, GL_UNSIGNED_INT, 0);
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                }
            });
        } else {
            projectionMatrix = context.frustum(view).toMatrix4x4();
            _prg.setUniformValue("projection_matrix", projectionMatrix);
            viewMatrix = context.viewMatrix(view);

            inOrder(kdTreeRoot, [](Node* node) {
                node->renderedThisFrame = false;
            });

            // check visible nodes of last frame
            for (int i = 0; i < vQueries.size(); i++) {
                while (!vQueries.at(i)->isAvailable()) {

                }
                if (vQueries.at(i)->getResult()) {
                    vQueries.at(i)->getNode()->visible = true;
                } else {
                    vQueries.at(i)->getNode()->visible = false;
                    pullUp(vQueries.at(i)->getNode());
                    inOrder(vQueries.at(i)->getNode(), [](Node* node) {
                        node->visible = false;
                    });
                }
                delete vQueries.at(i);
                vQueries.erase(vQueries.begin() + i);
            }

            // frustum culling
            const QVRFrustum& frustum = context.frustum(view);
            QVector3D nTop = QVector3D(0.0f, frustum.nearPlane(), frustum.topPlane()).normalized();
            QVector3D nBottom = -QVector3D(0.0f, frustum.nearPlane(), frustum.bottomPlane()).normalized();
            QVector3D nRight = QVector3D(frustum.nearPlane(), 0.0f, frustum.rightPlane()).normalized();
            QVector3D nLeft = -QVector3D(frustum.nearPlane(), 0.0f, frustum.leftPlane()).normalized();
            if (frustumCulling) {
                inOrder(kdTreeRoot, [&](Node* root) {
                    if (root->isLeaf) {
                        float x = root->data.position.x;
                        float y = root->data.position.y;
                        QMatrix4x4 modelMatrix;
                        modelMatrix.translate(x, 1.0f, y);
                        QVector3D boundingSphereCenter = (viewMatrix * modelMatrix).column(3).toVector3D();
                        const float boundingSphereRadius = 2.0f;
                        if (boundingSphereCenter.z() > (-frustum.nearPlane() + boundingSphereRadius)) {
                            root->visible = false;
                        } else if (boundingSphereCenter.z() < (-frustum.farPlane() + boundingSphereRadius)) {
                            root->visible = false;
                        } else if (QVector3D::dotProduct(nTop, boundingSphereCenter) > boundingSphereRadius) {
                            root->visible = false;
                        } else if (QVector3D::dotProduct(nBottom, boundingSphereCenter) > boundingSphereRadius) {
                            root->visible = false;
                        } else if (QVector3D::dotProduct(nLeft, boundingSphereCenter) > boundingSphereRadius) {
                            root->visible = false;
                        } else if (QVector3D::dotProduct(nRight, boundingSphereCenter) > boundingSphereRadius) {
                            root->visible = false;
                        } else {
                            root->visible = true;
                        }
                    }
                });
            }
            if (occlusionCullingCHC) {
                // occlusion culling
                frontToBack(kdTreeRoot, eye, [&](Node* node) {
                    if (node->visible && !node->isLeaf) {
                        return false;
                    }
                    if (node->visible && node->isLeaf) {
                        OcclusionQuery* query = new OcclusionQuery(node);
                        query->start(projectionMatrix, viewMatrix);
                        vQueries.push_back(query);

                        // immediately render
                        auto cell = node->data.type;
                        float x = node->data.position.x;
                        float y = node->data.position.y;
                        QMatrix4x4 modelMatrix;
                        modelMatrix.translate(x, 1.0f, y);
                        QMatrix4x4 modelViewMatrix = viewMatrix * modelMatrix;
                        _prg.setUniformValue("modelview_matrix", modelViewMatrix);
                        _prg.setUniformValue("view_matrix", viewMatrix);
                        _prg.setUniformValue("normal_matrix", modelViewMatrix.normalMatrix());

                        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                        glDepthMask(GL_TRUE);
                        glEnable(GL_DEPTH_TEST);
                        if (cell == GridCell::WALL) {
                            _prg.setUniformValue("color", QVector3D(1.0f, 0.0f, 0.0f));
                            glBindVertexArray(_vaoWall);
                            glDrawElements(GL_TRIANGLES, _vaoIndicesWall, GL_UNSIGNED_INT, 0);
                        } else if (cell == GridCell::EMPTY) {
                            _prg.setUniformValue("color", QVector3D(0.5f, 0.5f, 0.5f));
                            glBindVertexArray(_vaoFloor);
                            glDrawElements(GL_TRIANGLES, _vaoIndicesFloor, GL_UNSIGNED_INT, 0);
                        } else if (cell == GridCell::FINISH) {
                            _prg.setUniformValue("color", QVector3D(0.0f, 1.0f, 0.0f));
                            glBindVertexArray(_vaoFloor);
                            glDrawElements(GL_TRIANGLES, _vaoIndicesFloor, GL_UNSIGNED_INT, 0);
                        } else if (cell == GridCell::SPAWN) {
                            _prg.setUniformValue("color", QVector3D(0.7f, 0.7f, 0.0f));
                            glBindVertexArray(_vaoFloor);
                            glDrawElements(GL_TRIANGLES, _vaoIndicesFloor, GL_UNSIGNED_INT, 0);
                        } else if (cell == GridCell::COIN) {
                            _prg.setUniformValue("color", QVector3D(0.5f, 0.5f, 0.5f));
                            glBindVertexArray(_vaoFloor);
                            glDrawElements(GL_TRIANGLES, _vaoIndicesFloor, GL_UNSIGNED_INT, 0);
                            modelMatrix.setToIdentity();
                            modelMatrix.translate(x, 1.0f, y);
                            modelMatrix.rotate(90.0f, 1.0f, 0.0f, 0.0f);
                            modelMatrix.rotate(coinRotation, 0.0f, 0.0f, 1.0f);
                            modelMatrix.scale(2.0f);
                            modelViewMatrix = viewMatrix * modelMatrix;
                            _prg.setUniformValue("modelview_matrix", modelViewMatrix);
                            _prg.setUniformValue("view_matrix", viewMatrix);
                            _prg.setUniformValue("normal_matrix", modelViewMatrix.normalMatrix());
                            _prg.setUniformValue("color", QVector3D(1.0f, 1.0f, 0.0f));
                            glBindVertexArray(_vaoCoin);
                            glDrawElements(GL_TRIANGLES, _coinSize, GL_UNSIGNED_INT, 0);
                        } else if (cell == GridCell::DOOR) {
                            _prg.setUniformValue("color", QVector3D(0.0f, 0.0f, 1.0f));
                            glBindVertexArray(_vaoWall);
                            glDrawElements(GL_TRIANGLES, _vaoIndicesWall, GL_UNSIGNED_INT, 0);
                        }
                        node->renderedThisFrame = true;
                        return true;
                    }
                    if (!node->visible) {
                        OcclusionQuery* query = new OcclusionQuery(node);
                        query->start(projectionMatrix, viewMatrix);
                        iQueries.push_back(query);
                        return true;
                    }
                    return false;
                });

                while (!iQueries.empty()) {
                    for (auto it = iQueries.begin(); it < iQueries.end();) {
                        if ((*it)->isAvailable()) { // available?
                            if ((*it)->getResult()) {   // visible?
                                if ((*it)->getNode()->isLeaf) {
                                    Node* node = (*it)->getNode();
                                    auto cell = node->data.type;
                                    float x = node->data.position.x;
                                    float y = node->data.position.y;
                                    QMatrix4x4 modelMatrix;
                                    modelMatrix.translate(x, 1.0f, y);
                                    QMatrix4x4 modelViewMatrix = viewMatrix * modelMatrix;
                                    _prg.setUniformValue("modelview_matrix", modelViewMatrix);
                                    _prg.setUniformValue("view_matrix", viewMatrix);
                                    _prg.setUniformValue("normal_matrix", modelViewMatrix.normalMatrix());

                                    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                                    glDepthMask(GL_TRUE);
                                    glEnable(GL_DEPTH_TEST);
                                    glEnable(GL_CULL_FACE);
                                    if (cell == GridCell::WALL) {
                                        _prg.setUniformValue("color", QVector3D(1.0f, 0.0f, 0.0f));
                                        glBindVertexArray(_vaoWall);
                                        glDrawElements(GL_TRIANGLES, _vaoIndicesWall, GL_UNSIGNED_INT, 0);
                                    } else if (cell == GridCell::EMPTY) {
                                        _prg.setUniformValue("color", QVector3D(0.5f, 0.5f, 0.5f));
                                        glBindVertexArray(_vaoFloor);
                                        glDrawElements(GL_TRIANGLES, _vaoIndicesFloor, GL_UNSIGNED_INT, 0);
                                    } else if (cell == GridCell::FINISH) {
                                        _prg.setUniformValue("color", QVector3D(0.0f, 1.0f, 0.0f));
                                        glBindVertexArray(_vaoFloor);
                                        glDrawElements(GL_TRIANGLES, _vaoIndicesFloor, GL_UNSIGNED_INT, 0);
                                    } else if (cell == GridCell::SPAWN) {
                                        _prg.setUniformValue("color", QVector3D(0.7f, 0.7f, 0.0f));
                                        glBindVertexArray(_vaoFloor);
                                        glDrawElements(GL_TRIANGLES, _vaoIndicesFloor, GL_UNSIGNED_INT, 0);
                                    } else if (cell == GridCell::COIN) {
                                        _prg.setUniformValue("color", QVector3D(0.5f, 0.5f, 0.5f));
                                        glBindVertexArray(_vaoFloor);
                                        glDrawElements(GL_TRIANGLES, _vaoIndicesFloor, GL_UNSIGNED_INT, 0);
                                        modelMatrix.setToIdentity();
                                        modelMatrix.translate(x, 1.0f, y);
                                        modelMatrix.rotate(90.0f, 1.0f, 0.0f, 0.0f);
                                        modelMatrix.rotate(coinRotation, 0.0f, 0.0f, 1.0f);
                                        modelMatrix.scale(2.0f);
                                        modelViewMatrix = viewMatrix * modelMatrix;
                                        _prg.setUniformValue("modelview_matrix", modelViewMatrix);
                                        _prg.setUniformValue("view_matrix", viewMatrix);
                                        _prg.setUniformValue("normal_matrix", modelViewMatrix.normalMatrix());
                                        _prg.setUniformValue("color", QVector3D(1.0f, 1.0f, 0.0f));
                                        glBindVertexArray(_vaoCoin);
                                        glDrawElements(GL_TRIANGLES, _coinSize, GL_UNSIGNED_INT, 0);
                                    } else if (cell == GridCell::DOOR) {
                                        _prg.setUniformValue("color", QVector3D(0.0f, 0.0f, 1.0f));
                                        glBindVertexArray(_vaoWall);
                                        glDrawElements(GL_TRIANGLES, _vaoIndicesWall, GL_UNSIGNED_INT, 0);
                                    }
                                    node->renderedThisFrame = true;
                                    (*it)->getNode()->visible = true;
                                    delete (*it);
                                    it = iQueries.erase(it);
                                } else {
                                    Node* node =(*it)->getNode();
                                    OcclusionQuery* queryLeft = new OcclusionQuery(node->left);
                                    OcclusionQuery* queryRight = new OcclusionQuery(node->right);
                                    node->visible = true;
                                    queryLeft->start(projectionMatrix, viewMatrix);
                                    queryRight->start(projectionMatrix, viewMatrix);
                                    iQueries.push_back(queryLeft);
                                    iQueries.push_back(queryRight);
                                    delete (*it);
                                    it = iQueries.erase(it);
                                }

                            } else {    // not visible
                                Node* node = (*it)->getNode();
                                node->visible = false;
                                pullUp(node);
                                inOrder(node, [](Node* node) {
                                    node->visible = false;
                                });
                                delete (*it);
                                it = iQueries.erase(it);
                            }
                        } else {    // not available yet
                            it++;
                        }
                    }   // end query loop
                }   // end not empty while loop
            } else if (occlusionCulling) {
                frontToBack(kdTreeRoot, eye, [&](Node* node) {
                    if (node->isLeaf) {
                        GLuint query;
                        glGenQueries(1, &query);
                        glBeginQuery(GL_ANY_SAMPLES_PASSED, query);
                        auto cell = node->data.type;
                        float x = node->data.position.x;
                        float y = node->data.position.y;
                        
                        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
                        glDepthMask(GL_FALSE);
                        glEnable(GL_CULL_FACE);
                        QMatrix4x4 modelMatrix;
                        modelMatrix.translate(x, 1.0f, y);
                        QMatrix4x4 modelViewMatrix = viewMatrix * modelMatrix;
                        _prg.setUniformValue("modelview_matrix", modelViewMatrix);
                        _prg.setUniformValue("view_matrix", viewMatrix);
                        _prg.setUniformValue("normal_matrix", modelViewMatrix.normalMatrix());
                        _prg.setUniformValue("color", QVector3D(1.0f, 0.0f, 0.0f));
                        glBindVertexArray(_vaoWall);
                        glDrawElements(GL_TRIANGLES, _vaoIndicesWall, GL_UNSIGNED_INT, 0);
                        glEndQuery(GL_ANY_SAMPLES_PASSED);

                        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                        glDepthMask(GL_TRUE);

                        GLuint available;
                        do {
                            glGetQueryObjectuiv(query, GL_QUERY_RESULT_AVAILABLE, &available);
                        } while (available != GL_TRUE);
                        GLuint visible;
                        glGetQueryObjectuiv(query, GL_QUERY_RESULT, &visible);
                        glDeleteQueries(1, &query);
                        if (visible == GL_TRUE) {
                            node->renderedThisFrame = true;
                            if (cell == GridCell::WALL) {
                                _prg.setUniformValue("color", QVector3D(1.0f, 0.0f, 0.0f));
                                glBindVertexArray(_vaoWall);
                                glDrawElements(GL_TRIANGLES, _vaoIndicesWall, GL_UNSIGNED_INT, 0);
                            } else if (cell == GridCell::EMPTY) {
                                _prg.setUniformValue("color", QVector3D(0.5f, 0.5f, 0.5f));
                                glBindVertexArray(_vaoFloor);
                                glDrawElements(GL_TRIANGLES, _vaoIndicesFloor, GL_UNSIGNED_INT, 0);
                            } else if (cell == GridCell::FINISH) {
                                _prg.setUniformValue("color", QVector3D(0.0f, 1.0f, 0.0f));
                                glBindVertexArray(_vaoFloor);
                                glDrawElements(GL_TRIANGLES, _vaoIndicesFloor, GL_UNSIGNED_INT, 0);
                            } else if (cell == GridCell::SPAWN) {
                                _prg.setUniformValue("color", QVector3D(0.7f, 0.7f, 0.0f));
                                glBindVertexArray(_vaoFloor);
                                glDrawElements(GL_TRIANGLES, _vaoIndicesFloor, GL_UNSIGNED_INT, 0);
                            } else if (cell == GridCell::COIN) {
                                _prg.setUniformValue("color", QVector3D(0.5f, 0.5f, 0.5f));
                                glBindVertexArray(_vaoFloor);
                                glDrawElements(GL_TRIANGLES, _vaoIndicesFloor, GL_UNSIGNED_INT, 0);
                                modelMatrix.setToIdentity();
                                modelMatrix.translate(x, 1.0f, y);
                                modelMatrix.rotate(90.0f, 1.0f, 0.0f, 0.0f);
                                modelMatrix.rotate(coinRotation, 0.0f, 0.0f, 1.0f);
                                modelMatrix.scale(2.0f);
                                modelViewMatrix = viewMatrix * modelMatrix;
                                _prg.setUniformValue("modelview_matrix", modelViewMatrix);
                                _prg.setUniformValue("view_matrix", viewMatrix);
                                _prg.setUniformValue("normal_matrix", modelViewMatrix.normalMatrix());
                                _prg.setUniformValue("color", QVector3D(1.0f, 1.0f, 0.0f));
                                glBindVertexArray(_vaoCoin);
                                glDrawElements(GL_TRIANGLES, _coinSize, GL_UNSIGNED_INT, 0);
                            } else if (cell == GridCell::DOOR) {
                                _prg.setUniformValue("color", QVector3D(0.0f, 0.0f, 1.0f));
                                glBindVertexArray(_vaoWall);
                                glDrawElements(GL_TRIANGLES, _vaoIndicesWall, GL_UNSIGNED_INT, 0);
                            }
                        }
                    }
                    return false;
                });
            } else {
                frontToBack(kdTreeRoot, eye, [&](Node* root){
                    if (root->isLeaf) {
                        glEnable(GL_CULL_FACE);
                        auto cell = root->data.type;
                        float x = root->data.position.x;
                        float y = root->data.position.y;
                        if (root->visible) {
                            QMatrix4x4 modelMatrix;
                            modelMatrix.translate(x, 1.0f, y);
                            QMatrix4x4 modelViewMatrix = viewMatrix * modelMatrix;
                            _prg.setUniformValue("modelview_matrix", modelViewMatrix);
                            _prg.setUniformValue("view_matrix", viewMatrix);
                            _prg.setUniformValue("normal_matrix", modelViewMatrix.normalMatrix());
                            root->renderedThisFrame = true;
                            if (cell == GridCell::WALL) {
                                _prg.setUniformValue("color", QVector3D(1.0f, 0.0f, 0.0f));
                                glBindVertexArray(_vaoWall);
                                glDrawElements(GL_TRIANGLES, _vaoIndicesWall, GL_UNSIGNED_INT, 0);
                            } else if (cell == GridCell::EMPTY) {
                                _prg.setUniformValue("color", QVector3D(0.5f, 0.5f, 0.5f));
                                glBindVertexArray(_vaoFloor);
                                glDrawElements(GL_TRIANGLES, _vaoIndicesFloor, GL_UNSIGNED_INT, 0);
                            } else if (cell == GridCell::FINISH) {
                                _prg.setUniformValue("color", QVector3D(0.0f, 1.0f, 0.0f));
                                glBindVertexArray(_vaoFloor);
                                glDrawElements(GL_TRIANGLES, _vaoIndicesFloor, GL_UNSIGNED_INT, 0);
                            } else if (cell == GridCell::SPAWN) {
                                _prg.setUniformValue("color", QVector3D(0.7f, 0.7f, 0.0f));
                                glBindVertexArray(_vaoFloor);
                                glDrawElements(GL_TRIANGLES, _vaoIndicesFloor, GL_UNSIGNED_INT, 0);
                                
                            } else if (cell == GridCell::COIN) {
                                _prg.setUniformValue("color", QVector3D(0.5f, 0.5f, 0.5f));
                                glBindVertexArray(_vaoFloor);
                                glDrawElements(GL_TRIANGLES, _vaoIndicesFloor, GL_UNSIGNED_INT, 0);
                                modelMatrix.setToIdentity();
                                modelMatrix.translate(x, 1.0f, y);
                                modelMatrix.rotate(90.0f, 1.0f, 0.0f, 0.0f);
                                modelMatrix.rotate(coinRotation, 0.0f, 0.0f, 1.0f);
                                modelMatrix.scale(2.0f);
                                modelViewMatrix = viewMatrix * modelMatrix;
                                _prg.setUniformValue("modelview_matrix", modelViewMatrix);
                                _prg.setUniformValue("view_matrix", viewMatrix);
                                _prg.setUniformValue("normal_matrix", modelViewMatrix.normalMatrix());
                                _prg.setUniformValue("color", QVector3D(1.0f, 1.0f, 0.0f));
                                glBindVertexArray(_vaoCoin);
                                glDrawElements(GL_TRIANGLES, _coinSize, GL_UNSIGNED_INT, 0);
                            } else if (cell == GridCell::DOOR) {
                                _prg.setUniformValue("color", QVector3D(0.0f, 0.0f, 1.0f));
                                glBindVertexArray(_vaoWall);
                                glDrawElements(GL_TRIANGLES, _vaoIndicesWall, GL_UNSIGNED_INT, 0);
                            }
                        }
                    }
                    return false;
                });
            }
        }
        
    }
}

void MazeApp::update(const QList<QVRObserver*>& observers)
{
    constexpr float runSpeed = 5.0f;
    constexpr float hitbox = 0.1f;  // you are a 20 cm wide cylinder
    constexpr float collectionRange = 0.3f;
    constexpr float sensitivity = 0.5f; // mouse sensitivity
    constexpr float wallRadius = 1.0f;
    constexpr float coinSpeed = 100.0f;
    float seconds = 0.0f;
    if (_timer.isValid()) {
        seconds = _timer.nsecsElapsed() / 1e9f;
        _timer.restart();
    } else {
        _timer.start();
    }
    coinRotation += seconds * coinSpeed;

    auto deviceCount = QVRManager::deviceCount();
    auto observer = observers.at(0);    // only support one observer
    if (observer->config().navigationType() == QVRNavigationType::QVR_Navigation_Custom) {
        QVector3D posUpdate;
        auto orientation = observer->navigationOrientation();
        float pitch, yaw, roll;
        orientation.getEulerAngles(&pitch, &yaw, &roll);
        QVector3D forward = QQuaternion::fromEulerAngles(0, yaw, roll) * QVector3D(0.0f, 0.0f, -1.0f);
        QVector3D right = QQuaternion::fromEulerAngles(0, yaw, roll) * QVector3D(1.0f, 0.0f, 0.0f);

        if (forwardPressed) {
            posUpdate += runSpeed * seconds * forward;
        }
        if (backwardPressed) {
            posUpdate -= runSpeed * seconds * forward;
        }
        if (rightPressed) {
            posUpdate += runSpeed * seconds * right;
        }
        if (leftPressed) {
            posUpdate -= runSpeed * seconds * right;
        }
        
        yaw += mouseDx.x()*sensitivity;
        pitch += mouseDx.y()*sensitivity;
        if (pitch > 89.0f) {
            pitch = 89.0f;
        }
        if (pitch < -89.0f) {
            pitch = -89.0f;
        }
        auto newOrientation = QQuaternion::fromEulerAngles(pitch, yaw, roll);

        auto position = observer->navigationPosition();
        bool collision = false;
        position += posUpdate;
        frontToBack(kdTreeRoot, position, [&](Node* node) {
            if (!node->isLeaf) return false;
            auto& object = node->data;
            if (object.type == GridCell::WALL || object.type == GridCell::DOOR || object.type == GridCell::FINISH) {
                auto wallCenter = object.position;
                auto circleDistanceX = std::abs(position.x() - wallCenter.x);
                auto circleDistanceY = std::abs(position.z() - wallCenter.y);
                if (circleDistanceX > (wallRadius + hitbox)) return false;
                if (circleDistanceY > (wallRadius + hitbox)) return false;
                auto cornerDistance_sq = (circleDistanceX - wallRadius)*(circleDistanceX - wallRadius) +
                    (circleDistanceY - wallRadius)*(circleDistanceY - wallRadius);
                if (circleDistanceX <= wallRadius || circleDistanceY <= wallRadius ||
                    cornerDistance_sq <= (hitbox*hitbox)) {
                    // handle collision
                    if (object.type == GridCell::FINISH) {
                        _wantExit = true;
                        return true;
                    }
                    collision = true;
                    return true;
                }
            }
            if (object.type == GridCell::COIN) {
                auto coinPos = object.position;
                auto dist = (coinPos.x - position.x())*(coinPos.x - position.x()) + (coinPos.y - position.z())*(coinPos.y - position.z());
                if (dist < (coinBoundingSphere + collectionRange) * (coinBoundingSphere + collectionRange)) {
                    // collect coin
                    object.type = GridCell::EMPTY;
                    coinsLeft--;
                }
            }
            return false;
        });
        if (collision) {
            observer->setNavigation(observer->navigationPosition(), newOrientation);
        } else {
            observer->setNavigation(position, newOrientation);
        }
    }

    if (coinsLeft == 0) {
        inOrder(kdTreeRoot, [](Node* node){
            if (node->isLeaf) {
                if (node->data.type == GridCell::DOOR) {
                    node->data.type = GridCell::EMPTY;
                }
            }
        });
    }

    mouseDx = QVector2D(0.0f, 0.0f);
}

bool MazeApp::wantExit()
{
    return _wantExit;
}

void MazeApp::serializeDynamicData(QDataStream& ds) const
{
}

void MazeApp::deserializeDynamicData(QDataStream& ds)
{
}

void MazeApp::keyPressEvent(const QVRRenderContext& /* context */, QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Escape:
        _wantExit = true;
        break;
    case Qt::Key_W:
        forwardPressed = true;
        break;
    case Qt::Key_S:
        backwardPressed = true;
        break;
    case Qt::Key_A:
        leftPressed = true;
        break;
    case Qt::Key_D:
        rightPressed = true;
        break;
    case Qt::Key_P:
        if (occlusionCulling) occlusionCulling = false;
        occlusionCullingCHC = !occlusionCullingCHC;
        inOrder(kdTreeRoot, [](Node* node) {
            node->visible = true;
        });
        break;
    case Qt::Key_O:
        if (occlusionCullingCHC) occlusionCullingCHC = false;
        occlusionCulling = !occlusionCulling;
        inOrder(kdTreeRoot, [](Node* node) {
            node->visible = true;
        });
        break;
    case Qt::Key_F:
        frustumCulling = !frustumCulling;
        inOrder(kdTreeRoot, [](Node* node) {
            node->visible = true;
        });
        break;
    case Qt::Key_G:
        chcDebug = false;
        break;
    case Qt::Key_Plus:
        debugLevel++;
        if (debugLevel > 10)
            debugLevel = 10;
        chcDebug = true;
        break;
    case Qt::Key_Minus:
        debugLevel--;
        if (debugLevel < 0)
            debugLevel = 0;
        chcDebug = true;
        break;
    }

    if (event->key() > Qt::Key_0 && event->key() <= Qt::Key_9) {
        debugLevel = event->key() - Qt::Key_0;
        chcDebug = true;
    }
    if (event->key() == Qt::Key_0) {
        debugLevel = 10;
        chcDebug = true;
    }
}

void MazeApp::keyReleaseEvent(const QVRRenderContext& context, QKeyEvent* event)
{
    switch (event->key()) {
    case Qt::Key_W:
        forwardPressed = false;
        break;
    case Qt::Key_S:
        backwardPressed = false;
        break;
    case Qt::Key_A:
        leftPressed = false;
        break;
    case Qt::Key_D:
        rightPressed = false;
        break;
    }
}
void MazeApp::deviceButtonPressEvent(QVRDeviceEvent* event)
{
    switch (event->button()) {
    case QVRButton::QVR_Button_Up:
        forwardPressed = true;
    case QVRButton::QVR_Button_Left:
        leftPressed = true;
    case QVRButton::QVR_Button_Right:
        rightPressed = true;
    case QVRButton::QVR_Button_Down:
        backwardPressed = true;
    }
}

void MazeApp::deviceButtonReleaseEvent(QVRDeviceEvent* event)
{
    switch (event->button()) {
    case QVRButton::QVR_Button_Up:
        forwardPressed = false;
    case QVRButton::QVR_Button_Left:
        leftPressed = false;
    case QVRButton::QVR_Button_Right:
        rightPressed = false;
    case QVRButton::QVR_Button_Down:
        backwardPressed = false;
    }
}

void MazeApp::mouseMoveEvent(const QVRRenderContext& context, QMouseEvent* event)
{
    QPoint mousePos = event->globalPos();
    mouseDx = QVector2D(mousePosLastFrame.x() - mousePos.x(), mousePosLastFrame.y() - mousePos.y());
    mousePosLastFrame = mousePos;
}

void MazeApp::deviceAnalogChangeEvent(QVRDeviceEvent* event)
{
    constexpr float multiplier = 30.0f;
    auto analog = event->analog();
    if (analog == QVRAnalog::QVR_Analog_Axis_X) {
        auto device = event->device();
        float value = device.analogValue(device.analogIndex(analog));
        mouseDx.setX(value * multiplier);
    }
    if (analog == QVRAnalog::QVR_Analog_Axis_Y) {
        auto device = event->device();
        float value = device.analogValue(device.analogIndex(analog));
        mouseDx.setY(value * multiplier);
    }
}

void MazeApp::exitProcess(QVRProcess* process)
{
    freeTree(kdTreeRoot);
    delete[] mazeGrid;
}

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QVRManager manager(argc, argv);
    QSurfaceFormat format;
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setVersion(4,5);
    QSurfaceFormat::setDefaultFormat(format);

    /* Then start QVR with the app */
    MazeApp qvrapp;
    if (!manager.init(&qvrapp)) {
        qCritical("Cannot initialize QVR manager");
        return 1;
    }

    /* Enter the standard Qt loop */
    return app.exec();
}

Node* kdTree(std::vector<RenderObject>& objects, int depth)
{
    if (objects.size() == 0) return nullptr;
    if (objects.size() == 1)
    {
        Node* root = new Node;
        root->data = objects.at(0);
        root->isLeaf = true;
        root->visible = true;
        root->depth = depth;
        root->left = nullptr;
        root->right = nullptr;
        return root;
    }
    int axis = depth % 2;

    std::sort(objects.begin(), objects.end(), [&axis](RenderObject a, RenderObject b) {
        if (axis == 0) {
            return a.position.x < b.position.x;
        } else {
            return a.position.y < b.position.y;
        }
    });
    RenderObject median = objects.at(objects.size() / 2);
    Node* root = new Node;
    root->axis = axis;
    root->depth = depth;
    root->isLeaf = false;
    root->visible = false;
    if (objects.size() % 2 != 0) {
        if (axis == 0) {
            root->border = median.position.x;
        } else {
            root->border = median.position.y;
        }
    } else {
        RenderObject median2 = objects.at(objects.size() / 2 - 1);
        if (axis == 0) {
            root->border = (median.position.x + median2.position.x) / 2.0f;
        } else {
            root->border = (median.position.y + median2.position.y) / 2.0f;
        }
    }
    
    std::vector<RenderObject> left(&objects[0], &objects[objects.size() / 2]);
    std::vector<RenderObject> right(&objects[objects.size() / 2], &objects.back() + 1);
    root->left = kdTree(left, depth + 1);
    if (root->left) {
        root->left->parent = root;
    }
    root->right = kdTree(right, depth + 1);
    if (root->right) {
        root->right->parent = root;
    }
    return root;
}

void calcBorders(Node* root)
{
    if (root == nullptr || root->isLeaf) return;
    if (root->parent == nullptr) {
        root->xMin = -32.0f;
        root->xMax = 32.0f;
        root->yMin = -32.0f;
        root->yMax = 32.0f;
        root->centerX = root->xMin + (root->xMax - root->xMin) / 2.0f;
        root->centerY = root->yMin + (root->yMax - root->yMin) / 2.0f;
    } else {
        Node* parent = root->parent;
        if (parent->axis == 0) {
            if (parent->left == root) {
                root->xMax = parent->border;
                root->xMin = parent->xMin;
                root->yMax = parent->yMax;
                root->yMin = parent->yMin;
            } else {
                root->xMax = parent->xMax;
                root->xMin = parent->border;
                root->yMax = parent->yMax;
                root->yMin = parent->yMin;
            }
        } else {
            if (parent->left == root) {
                root->xMax = parent->xMax;
                root->xMin = parent->xMin;
                root->yMax = parent->border;
                root->yMin = parent->yMin;
            } else {
                root->xMax = parent->xMax;
                root->xMin = parent->xMin;
                root->yMax = parent->yMax;
                root->yMin = parent->border;
            }
        }
        root->centerX = root->xMin + (root->xMax - root->xMin) / 2.0f;
        root->centerY = root->yMin + (root->yMax - root->yMin) / 2.0f;
    }
    calcBorders(root->left);
    calcBorders(root->right);
}

void freeTree(Node* root)
{
    if (root == nullptr) {
        return;
    }
    freeTree(root->left);
    freeTree(root->right);
    delete root;
    root = nullptr;
}

void pullUp(Node* node)
{
    if (node == nullptr || node->parent == nullptr) return;
    if (!node->visible) {
        if (node->parent->left == node) {
            if (!node->parent->right->visible) {
                node->parent->visible = false;
                pullUp(node->parent);
            }
        } else {
            if (!node->parent->left->visible) {
                node->parent->visible = false;
                pullUp(node->parent);
            }
        }
    }
}

GLuint OcclusionQuery::vao;
QOpenGLShaderProgram OcclusionQuery::prg;
unsigned int OcclusionQuery::vaoIndices;
