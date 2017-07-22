
#include "entity.h"


Entity::Entity(std::shared_ptr<Mesh> mesh, QVector3D color, QVector3D position, QVector3D scale)
{
	
	this->_mesh = mesh;
	this->_color = color;
	this->_position = position;
	this->_scale = scale;
}

Entity::Entity(std::shared_ptr<Mesh> mesh, QVector3D color, QVector3D position, float scale)
{
	
	this->_mesh = mesh;
	this->_color = color;
	this->_position = position;
	this->_scale = QVector3D(scale, scale, scale);
}

Entity::~Entity()
{
	
}

QVector3D Entity::getPosition()
{
	return this->_position;
}

void Entity::draw(QOpenGLShaderProgram& prg, QMatrix4x4 viewMatrix)
{
	prg.setUniformValue("color", this->_color);
	QMatrix4x4 modelMatrix;
	modelMatrix.translate(this->_position);
	modelMatrix.scale(this->_scale);

	QMatrix4x4 modelViewMatrix = viewMatrix * modelMatrix;
	prg.setUniformValue("modelview_matrix", modelViewMatrix);
	prg.setUniformValue("normal_matrix", modelViewMatrix.normalMatrix());
	this->_mesh->draw();
}
