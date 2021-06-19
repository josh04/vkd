#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vkd {
	static glm::mat4 get_perspective(float fov, float aspect, float znear, float zfar, bool flip_y) {
		glm::mat4 pers = glm::perspective(glm::radians(fov), aspect, znear, zfar);
		if (flip_y) {
			pers[1][1] *= -1.0f;
		}

        return pers;
	};


	static glm::mat4 get_view(glm::vec3 pos, glm::vec3 rot, bool flip_y) {
		glm::mat4 rotM = glm::mat4(1.0f);
		glm::mat4 transM;

		rotM = glm::rotate(rotM, glm::radians(rot.x * (flip_y ? -1.0f : 1.0f)), glm::vec3(1.0f, 0.0f, 0.0f));
		rotM = glm::rotate(rotM, glm::radians(rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
		rotM = glm::rotate(rotM, glm::radians(rot.z), glm::vec3(0.0f, 0.0f, 1.0f));

		glm::vec3 translation = pos;
		if (flip_y) {
			translation.y *= -1.0f;
		}
		transM = glm::translate(glm::mat4(1.0f), translation);

		glm::mat4 view = transM * rotM;

		//viewPos = glm::vec4(position, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);

		return view;
	};
}