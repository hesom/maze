#include <iostream>
#include <QApplication>
#include <QKeyEvent>

#include <qvr/manager.hpp>

#include "maze.h"

Maze::Maze() :
	_wantExit(false),
	_pause(false)
{
	_timer.start();
}

bool Maze::initProcess(QVRProcess* p)
{
	initializeOpenGLFunctions();

	float pos[] = {
		-1.0f, +1.0f, +1.0f,   +1.0f, +1.0f, +1.0f,   +1.0f, -1.0f, +1.0f,   -1.0f, -1.0f, +1.0f, // front
		-1.0f, +1.0f, -1.0f,   +1.0f, +1.0f, -1.0f,   +1.0f, -1.0f, -1.0f,   -1.0f, -1.0f, -1.0f, // back
		+1.0f, +1.0f, -1.0f,   +1.0f, +1.0f, +1.0f,   +1.0f, -1.0f, +1.0f,   +1.0f, -1.0f, -1.0f, // right
		-1.0f, +1.0f, -1.0f,   -1.0f, +1.0f, +1.0f,   -1.0f, -1.0f, +1.0f,   -1.0f, -1.0f, -1.0f, // left
		-1.0f, +1.0f, +1.0f,   +1.0f, +1.0f, +1.0f,   +1.0f, +1.0f, -1.0f,   -1.0f, +1.0f, -1.0f, // top
		-1.0f, -1.0f, +1.0f,   +1.0f, -1.0f, +1.0f,   +1.0f, -1.0f, -1.0f,   -1.0f, -1.0f, -1.0f  // bottom
	};

	float n[] = {
		0.0f, 0.0f, +1.0f,   0.0f, 0.0f, +1.0f,   0.0f, 0.0f, +1.0f,   0.0f, 0.0f, +1.0f, // front
		0.0f, 0.0f, -1.0f,   0.0f, 0.0f, -1.0f,   0.0f, 0.0f, -1.0f,   0.0f, 0.0f, -1.0f, // back
		+1.0f, 0.0f, 0.0f,   +1.0f, 0.0f, 0.0f,   +1.0f, 0.0f, 0.0f,   +1.0f, 0.0f, 0.0f, // right
		-1.0f, 0.0f, 0.0f,   -1.0f, 0.0f, 0.0f,   -1.0f, 0.0f, 0.0f,   -1.0f, 0.0f, 0.0f, // left
		0.0f, +1.0f, 0.0f,   0.0f, +1.0f, 0.0f,   0.0f, +1.0f, 0.0f,   0.0f, +1.0f, 0.0f, // top
		0.0f, -1.0f, 0.0f,   0.0f, -1.0f, 0.0f,   0.0f, -1.0f, 0.0f,   0.0f, -1.0f, 0.0f  // bottom
	};

	unsigned int index[] = {
		0, 3, 1, 1, 3, 2, // front face
		4, 5, 7, 5, 6, 7, // back face
		8, 9, 11, 9, 10, 11, // right face
		12, 15, 13, 13, 15, 14, // left face
		16, 17, 19, 17, 18, 19, // top face
		20, 23, 21, 21, 23, 22, // bottom face
	};

	std::vector<QVector3D> positions;
	std::vector<QVector3D> normals;
	std::vector<unsigned int> indices;

	for (int i = 0; i < 72; i+=3) {
		positions.push_back(QVector3D(pos[i], pos[i + 1], pos[i + 2]));
		normals.push_back(QVector3D(n[i], n[i + 1], n[i + 2]));
	}

	for (int i = 0; i < 36; ++i) {
		indices.push_back(index[i]);
	}

	std::vector<Vertex> vertices;
	for (int i = 0; i < positions.size(); ++i) {
		Vertex v;
		v.position = positions[i];
		v.normal = normals[i];
		vertices.push_back(v);
	}

	_cube = std::make_shared<Mesh>(vertices, indices);


	glGenFramebuffers(1, &_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
	glGenTextures(1, &_fboDepthTex);
	glBindTexture(GL_TEXTURE_2D, _fboDepthTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
				 1, 1, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _fboDepthTex, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	_prg.addShaderFromSourceFile(QOpenGLShader::Vertex, ":vertexshader.glsl");
	_prg.addShaderFromSourceFile(QOpenGLShader::Fragment, ":fragmentshader.glsl");
	_prg.link();

	return true;
}

void Maze::render(QVRWindow* w,
				  const QVRRenderContext& context,
				  int viewPass,
				  unsigned int texture)
{
	GLint width, height;
	glBindTexture(GL_TEXTURE_2D, texture);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
	glBindTexture(GL_TEXTURE_2D, _fboDepthTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height,
				 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);

	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	QMatrix4x4 projectionMatrix = context.frustum(viewPass).toMatrix4x4();
	QMatrix4x4 viewMatrix = context.viewMatrix(viewPass);

	glUseProgram(_prg.programId());
	_prg.setUniformValue("projection_matrix", projectionMatrix);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	QVector3D color = QVector3D(1.0f, 0.0f, 0.0f);
	_prg.setUniformValue("color", color);

	QMatrix4x4 modelMatrix;
	modelMatrix.scale(0.5);
	modelMatrix.translate(2.0f, 0.0f, -25.0f);
	QMatrix4x4 modelViewMatrix = viewMatrix * modelMatrix;
	_prg.setUniformValue("modelview_matrix", modelViewMatrix);
	_prg.setUniformValue("normal_matrix", modelViewMatrix.normalMatrix());
	_cube->draw();
}

void Maze::update(const QList<QVRObserver*>& observers)
{
	if (_pause) {
		if (_timer.isValid()) {
			_elapsedTime += _timer.elapsed();
			_timer.invalidate();
		}
		else {
			if (!_timer.isValid()) {
				_timer.start();
			}
			float seconds = (_elapsedTime + _timer.elapsed()) / 1000.0f;
			//Animation here
		}
	}
}

bool Maze::wantExit()
{
	return _wantExit;
}

void Maze::serializeDynamicData(QDataStream& ds) const
{

}

void Maze::deserializeDynamicData(QDataStream& ds)
{

}

void Maze::keyPressEvent(const QVRRenderContext& context, QKeyEvent* event)
{
	switch (event->key())
	{
	case Qt::Key_Escape:
		_wantExit = true;
		break;
	}
}
int main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	QVRManager manager(argc, argv);

	QSurfaceFormat format;
	format.setProfile(QSurfaceFormat::CoreProfile);
	format.setVersion(3, 3);
	QSurfaceFormat::setDefaultFormat(format);

	Maze qvrapp;
	if (!manager.init(&qvrapp)) {
		qCritical("Cannot initialize QVR manager");
		return 1;
	}

	return app.exec();
}
