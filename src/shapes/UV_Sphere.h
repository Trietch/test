#pragma once

#include "../Shape.h"
#include "../MainWindow.h"

class UV_Sphere : public Shape {

public:
	UV_Sphere(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, glm::vec2 faces, MainWindow * main_window);
	~UV_Sphere();

private:
	static std::vector<GLfloat> generate_vertices(glm::vec2 faces);
	static std::vector<GLuint> generate_indices(glm::vec2 faces);
	static std::tuple<std::vector<GLfloat>, std::vector<GLfloat>, std::vector<GLuint>> get_geometry(glm::vec2 faces);
};