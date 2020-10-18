#pragma once

#include <GL/gl.h>
#include <QOpenGLContext>
#include <QOpenGLWidget>
#include <cstdint>
#include <memory>

#include "Shader.h"
#include "src/ECS.h"


class DrawableComponent;
class CameraComponent;
class LightComponent;

class Renderer
{
public:
	void initGl() const;
	void resizeGl(int __w, int __h) const;

	void drawDrawable(DrawableComponent* __drawableComponent);
	void initDrawable(DrawableComponent* __drawableComponent);
	void freeDrawable(DrawableComponent* __drawableComponent);
	void clear() const;

	void setActiveCamera(CameraComponent* __cameraComponent);
	void addLight(LightComponent* __lightComponent);


private:
	CameraComponent* _activeCamera = nullptr;
	GLsizei _sceneIndices = 0;
	std::vector<LightComponent*> _lights;

	void _useShader(DrawableComponent* __drawableComponent);
};

