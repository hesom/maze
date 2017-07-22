
#include "cell.h"

Cell::Cell()
{
	
	_entities = std::vector<Entity>();
	_portals = std::vector<Portal>();
}

Cell::~Cell()
{
}

void Cell::addEntity(Entity e)
{
	_entities.push_back(e);
}

void Cell::addPortal(Portal p)
{
	_portals.push_back(p);
}

std::vector<Entity> Cell::getEntities()
{
	return _entities;
}

std::vector<Portal> Cell::getPortals()
{
	return _portals;
}

void Cell::buildWall(std::shared_ptr<Mesh> mesh, QVector3D startPosition, QVector3D color, int blocks, bool up)
{
	QVector3D horizontalOffset(0,0,0);
	QVector3D verticalOffset(0, 0.5f, 0);
	int height = 10;
	if (up) {
		horizontalOffset.setZ(-0.5f);
	}
	else {
		horizontalOffset.setX(0.5f);
	}
	for (int i = 0; i < blocks; ++i) {
		for (int j = 0; j < height; ++j) {
			Entity e(mesh, color, startPosition + i*horizontalOffset + j*verticalOffset, 0.5f);
			_entities.push_back(e);
		}
	}
}

void Cell::draw(QOpenGLShaderProgram& prg, QMatrix4x4 viewMatrix)
{
	for (auto e : _entities) {
		e.draw(prg, viewMatrix);
	}
}
