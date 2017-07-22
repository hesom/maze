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
