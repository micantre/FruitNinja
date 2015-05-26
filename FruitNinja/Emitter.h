#pragma once

#include "Particle.h"

#define POS_ATTRIB 0
#define FRAME_ATTRIB 1

//I think an SOA is better than AOS for gpu...
struct Particles {
	std::vector<glm::vec3> positions;
	std::vector<int> frame;
};

class Emitter {
	glm::vec3 center;
	Particles particles;
	GLuint pos_VBO;
	GLuint frame_VBO;
public:
	Emitter();
	virtual ~Emitter();

	virtual void update(double deltaTime);
	int getNumParticles();
	tdogl::Texture* texture;

	GLuint VAO;
};