#include <iostream>
#include <QApplication>
#include <QKeyEvent>

#include <qvr/manager.hpp>

#include "maze.h"

Maze::Maze()
{

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

	return true;
}

void Maze::render(QVRWindow* w,
				  const QVRRenderContext& context,
				  int viewPass,
				  unsigned int texture)
{

}

void Maze::update(const QList<QVRObserver*>& observers)
{

}

bool Maze::wantExit()
{
	return false;
}

void Maze::serializeDynamicData(QDataStream& ds) const
{

}

void Maze::deserializeDynamicData(QDataStream& ds)
{

}

void Maze::keyPressEvent(const QVRRenderContext& context, QKeyEvent* event)
{

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
