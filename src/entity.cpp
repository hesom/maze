
#include "entity.h"

Entity::Entity(std::shared_ptr<Mesh> mesh, QVector3D color, QVector3D position, QVector3D scale, QVector3D rotation)
{
	this->_mesh = mesh;
	this->_color = color;
	this->_position = position;
	this->_scale = scale;
	this->_rotation = rotation;
}

Entity::Entity(std::shared_ptr<Mesh> mesh, QVector3D color, QVector3D position, float scale, QVector3D rotation)
{
	QVector3D s(scale, scale, scale);
	Entity(mesh, color, position, s, rotation);
}
