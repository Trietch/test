#pragma once

#include "../Shape.h"

class Quad : public Shape {

public:
	Quad(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale);
	~Quad();

private:
};
