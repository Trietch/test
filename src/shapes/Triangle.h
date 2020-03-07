#pragma once

#include "../Shape.h"

class Triangle : public Shape {

public:
	Triangle(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale);
	~Triangle();

	void draw(glm::vec3 view_position, glm::mat4 projection, float delta_time, std::vector<std::shared_ptr<Entity>> lights) override;


private:
	glm::vec4 _color;
	void apply_color();
};
