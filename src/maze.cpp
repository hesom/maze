#include <iostream>
#include <QApplication>
#include <QKeyEvent>

#include <qvr/manager.hpp>
#include <qvr/observer.hpp>

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

	_cube = Mesh::cubeMesh();

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
	modelMatrix.translate(0.0f, 0.0f, -10.0f);
	modelMatrix.scale(0.1f);
	modelMatrix.translate(0.0f, 0.5f, 0.0f);
	QMatrix4x4 modelViewMatrix = viewMatrix * modelMatrix;
	_prg.setUniformValue("modelview_matrix", modelViewMatrix);
	_prg.setUniformValue("normal_matrix", modelViewMatrix.normalMatrix());
	_cube->draw();

	modelMatrix.setToIdentity();
	modelMatrix.translate(0.0f, 0.1f, -10.0f);
	modelMatrix.scale(0.1f);
	modelMatrix.translate(0.0f, 0.5f, 0.0f);
	modelViewMatrix = viewMatrix * modelMatrix;
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
	for (auto obs : observers) {
		QVector3D pos = obs->navigationPosition();
		QVector3D eye = obs->trackingPosition();
		std::cout << "Nav: (" << pos.x() << "," << pos.y() <<"," << pos.z() << ")"<< std::endl;
		std::cout << "Tracking: (" << eye.x() << "," << eye.y() << "," << eye.z() << ")" << std::endl;
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
