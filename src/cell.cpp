
#include "cell.h"

Cell::Cell()
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
