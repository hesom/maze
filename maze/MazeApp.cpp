/*
 * Copyright (C) 2016, 2017 Computer Graphics Group, University of Siegen
 * Written by Martin Lambers <martin.lambers@uni-siegen.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <iostream>

#include <QGuiApplication>
#include <QKeyEvent>
#include <QMessageBox>

#include <qvr/manager.hpp>
#include <qvr/window.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "MazeApp.hpp"


MazeApp::MazeApp() :
    _wantExit(false),
    _rotationAngle(0.0f)
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
    renderMask = new bool[mazeWidth * mazeHeight];
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
        }

        renderMask[cell] = true;
    }
    stbi_image_free(mazeImage);

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

    // Shader program
    _prg.addShaderFromSourceFile(QOpenGLShader::Vertex, ":vertex-shader.glsl");
    _prg.addShaderFromSourceFile(QOpenGLShader::Fragment, ":fragment-shader.glsl");
    if (!_prg.link()) {
        qCritical("Could not link program! Check shaders!");
    }

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
        // Set up view
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        QMatrix4x4 projectionMatrix;
        QMatrix4x4 viewMatrix;

        if (w->id() == "debug") {
            projectionMatrix.ortho(-40.0f, 40.0f, -40.0f, 40.0f, 0.1f, 100.0f);
            viewMatrix.lookAt(QVector3D(0.0f, 10.0f, 0.0f), QVector3D(0.0f, 0.0f, 0.0f), QVector3D(-1.0f, 0.0f, 0.0f));
        } else {
            projectionMatrix = context.frustum(view).toMatrix4x4();
            viewMatrix = context.viewMatrix(view);
            const QVRFrustum& frustum = context.frustum(view);
            QVector3D nTop = QVector3D(0.0f, frustum.nearPlane(), frustum.topPlane()).normalized();
            QVector3D nBottom = -QVector3D(0.0f, frustum.nearPlane(), frustum.bottomPlane()).normalized();
            QVector3D nRight = QVector3D(frustum.nearPlane(), 0.0f, frustum.rightPlane()).normalized();
            QVector3D nLeft = -QVector3D(frustum.nearPlane(), 0.0f, frustum.leftPlane()).normalized();
            for (int col = 0; col < gridWidth; col++) {
                for (int row = 0; row < gridHeight; row++) {
                    float x = -((float)gridWidth) + 2.0f * col;
                    float y = ((float)gridHeight) - 2.0f * row;
                    QMatrix4x4 modelMatrix;
                    modelMatrix.translate(x, 1.0f, y);
                    QVector3D boundingSphereCenter = (viewMatrix * modelMatrix).column(3).toVector3D();
                    const float boundingSphereRadius = 2.0f;
                    if (boundingSphereCenter.z() > (-frustum.nearPlane() + boundingSphereRadius)) {
                        renderMask[row * gridWidth + col] = false;
                    } else if (boundingSphereCenter.z() < (-frustum.farPlane() + boundingSphereRadius)) {
                        renderMask[row * gridWidth + col] = false;
                    } else if (QVector3D::dotProduct(nTop, boundingSphereCenter) > boundingSphereRadius) {
                        renderMask[row * gridWidth + col] = false;
                    } else if (QVector3D::dotProduct(nBottom, boundingSphereCenter) > boundingSphereRadius) {
                        renderMask[row * gridWidth + col] = false;
                    } else if (QVector3D::dotProduct(nLeft, boundingSphereCenter) > boundingSphereRadius) {
                        renderMask[row * gridWidth + col] = false;
                    } else if (QVector3D::dotProduct(nRight, boundingSphereCenter) > boundingSphereRadius) {
                        renderMask[row * gridWidth + col] = false;
                    }
                    
                    else {
                        renderMask[row * gridWidth + col] = true;
                    }
                }
            }
        }

        // Set up shader program
        glUseProgram(_prg.programId());
        _prg.setUniformValue("projection_matrix", projectionMatrix);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        // Render
        for (int col = 0; col < gridWidth; col++) {
            for (int row = 0; row < gridHeight; row++) {
                auto cell = GetCell(col, row);
                float x = -((float)gridWidth) + 2.0f * col;
                float y = ((float)gridHeight) - 2.0f * row;
                if (renderMask[row * gridWidth + col]) {
                    QMatrix4x4 modelMatrix;
                    modelMatrix.translate(x, 1.0f, y);
                    QMatrix4x4 modelViewMatrix = viewMatrix * modelMatrix;
                    _prg.setUniformValue("modelview_matrix", modelViewMatrix);
                    _prg.setUniformValue("view_matrix", viewMatrix);
                    _prg.setUniformValue("normal_matrix", modelViewMatrix.normalMatrix());

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
                    }
                } 
            }
        }
    }
}

void MazeApp::update(const QList<QVRObserver*>& observers)
{
    float seconds = _timer.elapsed() / 1000.0f;
    _rotationAngle = seconds * 20.0f;
}

bool MazeApp::wantExit()
{
    return _wantExit;
}

void MazeApp::serializeDynamicData(QDataStream& ds) const
{
    ds << _rotationAngle;
}

void MazeApp::deserializeDynamicData(QDataStream& ds)
{
    ds >> _rotationAngle;
}

void MazeApp::keyPressEvent(const QVRRenderContext& /* context */, QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Escape:
        _wantExit = true;
        break;
    }
}

void MazeApp::exitProcess(QVRProcess* process)
{
    delete[] mazeGrid;
}

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QVRManager manager(argc, argv);
    QSurfaceFormat format;
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setVersion(3,3);
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
