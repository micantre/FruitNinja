#include "ChewyMovementComponent.h"
#include "ChewyEntity.h"
#include <GLFW/glfw3.h>
#include "World.h"
#include <iostream>

using namespace glm;
using namespace std;

vec3 forwardDirection(std::shared_ptr<Camera> camera)
{
	return normalize(camera->cameraFront);
}

vec3 backDirection(std::shared_ptr<Camera> camera)
{
	return normalize(camera->cameraFront) * -1.0f;
}

vec3 rightDirection(std::shared_ptr<Camera> camera)
{
	return normalize(cross(camera->cameraFront, vec3(0.0, 1.0, 0.0)));
}

vec3 leftDirection(std::shared_ptr<Camera> camera)
{
	return normalize(cross(camera->cameraFront, vec3(0.0, 1.0, 0.0))) * -1.0f;
}

float angleDiff(float angle_a, float angle_b)
{
	vec2 a(cos(angle_a), sin(angle_a));
	vec2 b(cos(angle_b), sin(angle_b));

	return acos(dot(a, b));
}

void ChewyMovementComponent::update()
{
	entity.last_position = entity.getPosition();

	vec3 position = entity.getPosition();
	vec3 rotations = entity.getRotations();

	if (dynamic_cast<ChewyEntity&>(entity)._falling)
	{
		entity.velocity.y -= GRAVITY * seconds_passed;
	}
	else
	{
		entity.velocity = vec3(0.f);
	}

	if (player_cam->in_use) 
    {
		direction.x = 0.f;
		direction.y = 0.f;
		direction.z = 0.f;
		
		bool moved = false;

		if (keys[GLFW_KEY_W]) {
			direction += forwardDirection(player_cam);
			moved = true;
		}
		if (keys[GLFW_KEY_S]) {
			direction += backDirection(player_cam);
			moved = true;
		}
		if (keys[GLFW_KEY_A]) {
			direction += leftDirection(player_cam);
			moved = true;
		}
		if (keys[GLFW_KEY_D]) {
			direction += rightDirection(player_cam);
			moved = true;
		}

		entity.moving = moved;

        vec3 pos_offset;

		if (length(direction) > 0) {
            direction = normalize(direction * vec3(1, 0, 1));
                
            pos_offset = direction * CHEWY_MOVE_SPEED * seconds_passed;

			float toAngle = entity.turnAngle(direction).y;
			float fromAngle = rotations.y;
			
			if (toAngle - fromAngle < -M_PI)
			{
				toAngle += 2 * M_PI;
				rotations.y += (toAngle - fromAngle) * CHEWY_ROTATE_SPEED * seconds_passed;
				if (rotations.y > M_PI)
					rotations.y -= 2 * M_PI;
			}
			else if (toAngle - fromAngle > M_PI)
			{
				fromAngle += 2 * M_PI;
				rotations.y += (toAngle - fromAngle) * CHEWY_ROTATE_SPEED * seconds_passed;
				if (rotations.y < -M_PI)
					rotations.y += 2 * M_PI;
			}
			else
				rotations.y += (toAngle - fromAngle) * CHEWY_ROTATE_SPEED * seconds_passed;

            position += pos_offset;
		}
        if ((position.y <= 0 || !dynamic_cast<ChewyEntity&>(entity)._falling) && keys[GLFW_KEY_SPACE])
		{
			entity.velocity.y += 30;
		} 
	}
	else if(archery_cam->in_use) {
		rotations = entity.turnAngle(archery_cam->cameraFront);
	}

	position += entity.velocity * seconds_passed;
	if (position.y < 0)
	{
		position.y = 0;
		entity.velocity.y = 0;
	}
	dynamic_cast<ChewyEntity&>(entity)._falling = true;

	entity.setPosition(position);
	entity.setRotations(rotations);
}