#ifndef MAZE_MAZE_H
#define MAZE_MAZE_H

#include <memory>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QElapsedTimer>

#include <qvr/app.hpp>

#include "mesh.h"
#include "entity.h"
#include "cell.h"

class Maze : public QVRApp, protected QOpenGLFunctions_3_3_Core
{
private:
	bool _wantExit;             // do we want to exit the app?
	bool _pause;                // are we in pause mode?
	qint64 _elapsedTime;        // used for rotating the box
	QElapsedTimer _timer;
	GLuint _fbo;
	GLuint _fboDepthTex;

	std::shared_ptr<Mesh> _cube;
	std::vector<Cell> cells;

	QOpenGLShaderProgram _prg;
protected:
public:
	Maze();

	bool initProcess(QVRProcess* p) override;

	void render(QVRWindow* w,
				const QVRRenderContext& context,
				int viewPass,
				unsigned int texture)
		override;

	void update(const QList<QVRObserver*>& observers) override;

	bool wantExit() override;

	void serializeDynamicData(QDataStream& ds) const override;
	void deserializeDynamicData(QDataStream& ds) override;

	void keyPressEvent(const QVRRenderContext& context, QKeyEvent* event) override;

};

#endif  // MAZE_MAZE_H
