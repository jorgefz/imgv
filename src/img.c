
/*

Description

*/

#include <stdio.h>
#include <stdlib.h>

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

const char* vshader_src = " "
"#version 330 core \n"
"layout(location = 0) in vec4 position; \n"
"layout(location = 1) in vec2 texCoords; \n"
"out vec2 v_texCoord; \n"
"void main() { \n"
"	gl_Position = position; \n"
"	v_texCoord = texCoords; \n"
"}; \n";

const char* fshader_src = " "
"#version 330 core \n"
"layout(location = 0) out vec4 color; \n"
"in vec2 v_texCoord; \n"
"uniform sampler2D u_Texture; \n"
"uniform vec4 u_Color; \n"
"void main() { \n"
"	vec4 texColor = texture(u_Texture, v_texCoord); \n"
"	if (texColor.a < 0.1) \n"
"		discard; \n"
"	color = texColor; \n"
"}; \n";

GLFWwindow* WindowInit(GLuint width, GLuint height) {
	GLFWwindow* window;
	GLFWmonitor* monitor;

	if (!glfwInit()) {
		fprintf(stderr, "[GLFW] Fatal error: failed to load!\n");
		return NULL;
	}
	if ((width % 16 != 0) || (height % 9 != 0)) {
		fprintf(stderr, " Error: screen resolution must have 16:9 aspect ratio\n");
		return NULL;
	}
	window = glfwCreateWindow(width, height, "Command Line Image Viewer", NULL, NULL);
	if (!window) {
		fprintf(stderr, "[GLFW] Fatal error: failed to create window\n");
		glfwTerminate();
		return NULL;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // activate VSYNC, problems with NVIDIA cards
	glViewport(0, 0, width, height);
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "[GLEW] Fatal error: failed to load!\n");
		return NULL;
	}
	int glfw_major, glfw_minor, glfw_rev;
	glfwGetVersion(&glfw_major, &glfw_minor, &glfw_rev);
	printf("[GLEW] Version %s\n", glewGetString(GLEW_VERSION));
	printf("[GLFW] Version %d.%d.%d\n", glfw_major, glfw_minor, glfw_rev);
	return window;
}


void update_vao_attributes(GLuint sdims, GLuint tdims, GLuint stride) {
	unsigned int offset = 0;

	glEnableVertexAttribArray(0); // x,y
	glVertexAttribPointer(0, sdims, GL_FLOAT, GL_FALSE, stride, (const void*)offset);
	offset += sdims * sizeof(float);

	glEnableVertexAttribArray(1); // s,t
	glVertexAttribPointer(1, tdims, GL_FLOAT, GL_FALSE, stride, (const void*)offset);
}

GLuint shader_compile(unsigned int type, const char* src) {
	GLuint id;
	id = glCreateShader(type);
	glShaderSource(id, 1, &src, NULL);
	glCompileShader(id);
	// Error handling
	int result;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	if (result == GL_FALSE) {
		fprintf(stderr, "Error compiling shader!");
		return 0;
	}
	return id;
}

GLuint texture_load(const char* filename) {
	GLuint width, height, channels, tex_id;

	stbi_set_flip_vertically_on_load(1);
	const char* buff = stbi_load(filename, &width, &height, &channels, 4);
	if (!buff) {
		fprintf(stderr, "Error loading image '%s'\n", filename);
		return 0;
	}
	printf(" Loaded '%s'\n", filename);
	printf(" %dx%d (%d channels)\n", width, height, channels);
	
	glGenTextures(1, &tex_id);
	glBindTexture(GL_TEXTURE_2D, tex_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	unsigned int format = channels == 4 ? GL_RGBA : GL_RGB;
	//                          -> Internal format            -> Input format
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buff);
	
	glBindTexture(GL_TEXTURE_2D, tex_id);
	if (buff) stbi_image_free(buff);
	
	return tex_id;
}

int main(int argc, const char* argv[]) {

	if (argc != 2) {
		fprintf(stderr, "Error: missing input image\n");
		return 1;
	}

	const char* filename = argv[1];
	const char* buff;
	int width, height, channels;

	GLFWwindow* window = WindowInit(1280, 720);
	if (!window) return 1;

	// Shader
	GLuint shader_id = glCreateProgram();
	GLuint vs_id = shader_compile(GL_VERTEX_SHADER, vshader_src);
	GLuint fs_id = shader_compile(GL_FRAGMENT_SHADER, fshader_src);
	if (vs_id == 0 || fs_id == 0) return 1;

	glAttachShader(shader_id, vs_id);
	glAttachShader(shader_id, fs_id);
	glLinkProgram(shader_id);
	glValidateProgram(shader_id);
	// shader data has been copied to program and is no longer necessary.
	glDeleteShader(vs_id);
	glDeleteShader(fs_id);
	glUseProgram(shader_id);

	// Main image texture
	GLuint tex_id = texture_load(filename);
	if (tex_id == 0) return 1;

	// Vertex Buffer Object
	GLuint vbo_id;
	float vertices[] = {
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		 1.0f, 1.0f,   1.0f, 1.0f,
		-1.0f, 1.0f,   0.0f, 1.0f
	};

	glGenBuffers(1, &vbo_id);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Index Buffer Object
	GLuint ibo_id;
	unsigned int indices[] = {0, 1, 2, 2, 3, 0};
	glGenBuffers(1, &ibo_id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Vertex Array Object
	GLuint vao_id;
	glCreateVertexArrays(1, &vao_id);
	update_vao_attributes(2, 2, sizeof(float) * 4);

	// Rendering Loop
	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawElements(GL_TRIANGLES, sizeof(indices), GL_UNSIGNED_INT, 0);
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Deleting
	glDeleteBuffers(1, &vbo_id);
	glDeleteBuffers(1, &ibo_id);
	glDeleteVertexArrays(1, &vao_id);
	glDeleteProgram(&shader_id);
	glDeleteTextures(1, &tex_id);
	glfwTerminate();

	return 0;
}
