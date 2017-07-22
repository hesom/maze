#ifndef MAZE_MESH_H
#define MAZE_MESH_H

#include <QVector3D>
#include <QOpenGLFunctions_3_3_Core>
#include <vector>

struct Vertex
{
	QVector3D position;
	QVector3D normal;
	
};

class Mesh : QOpenGLFunctions_3_3_Core
{
private:
	std::vector<Vertex> _vertices;
	std::vector<unsigned int> _indices;
	GLuint _vao;
	GLuint _vbo;
	GLuint _ebo;

	void setupMesh();
public:
	Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices);
	void draw();
};

#endif // MAZE_MESH_H

