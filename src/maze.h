#ifndef MAZE_MAZE_H
#define MAZE_MAZE_H

#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QElapsedTimer>

#include <qvr/app.hpp>

class Maze : public QVRApp, protected QOpenGLFunctions_3_3_Core
{
private:

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
