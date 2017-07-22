#ifndef MAZE_ENTITY_H
#define MAZE_ENTITY_H

#include <memory>
#include <QVector3D>
#include <QMatrix4x4>
#include <QOpenGLFunctions_3_3_Core>
#include "mesh.h"

//Drawable object
class Entity : QOpenGLFunctions_3_3_Core
{
private:
	std::shared_ptr<Mesh> _mesh;
	QVector3D _position;
	QVector3D _scale;
	QVector3D _rotation;
	QVector3D _color;
public:
	Entity(std::shared_ptr<Mesh> mesh, QVector3D color,  QVector3D position, QVector3D scale, QVector3D rotation);
	Entity(std::shared_ptr<Mesh> mesh, QVector3D color, QVector3D position, float scale, QVector3D rotation);
};

#endif // MAZE_OBJECT_H

