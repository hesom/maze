#ifndef MAZE_CELL_H
#define MAZE_CELL_H

#include <QVector3D>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <vector>
#include <memory>

#include "entity.h"
#include "mesh.h"

class Cell;

struct Portal
{
	QVector3D position;
	std::shared_ptr<Cell> neighbor;
	float size_x;
	float size_y;
};

class Cell
{
private:
	std::vector<Entity> _entities;
	std::vector<Portal> _portals;

public:
	Cell();
	~Cell();
	void addEntity(Entity e);
	void addPortal(Portal p);

	std::vector<Entity> getEntities();
	std::vector<Portal> getPortals();

	void buildWall(std::shared_ptr<Mesh> mesh, QVector3D startPosition, QVector3D color, int blocks, bool up);

	void draw(QOpenGLShaderProgram& program, QMatrix4x4 viewMatrix);
};

#endif // MAZE_CELL_H