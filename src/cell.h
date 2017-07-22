#ifndef MAZE_CELL_H
#define MAZE_CELL_H

#include <QVector3D>
#include <vector>
#include <memory>

#include "entity.h"

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
};

#endif // MAZE_CELL_H