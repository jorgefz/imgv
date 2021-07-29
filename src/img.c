
/*

Description

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define IMGV_VERSION "1.1"
#define IMGV_DATE "04/06/2021"

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

double mscroll_y;

typedef struct texture_struct {
	unsigned int height, width, channels, id;
} Texture;

GLFWwindow* WindowInit(GLuint width, GLuint height) {
	GLFWwindow* window;

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

GLuint shader_create() {
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

	return shader_id;
}

Texture* texture_load(const char* filename) {
	GLuint width, height, channels, tex_id;

	stbi_set_flip_vertically_on_load(1);
	unsigned char* buff = stbi_load(filename, &width, &height, &channels, 4);
	if (!buff) {
		fprintf(stderr, "Error loading image '%s'\n", filename);
		return NULL;
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

	Texture *tex = (Texture *)malloc(sizeof(Texture));
	if (!tex) {
		fprintf(stderr, "Fatal error: could not allocate %u bytes\n", (GLuint)sizeof(Texture));
		return NULL;
	}
	tex->height = height;
	tex->width = width;
	tex->channels = channels;
	tex->id = tex_id;
	return tex;
}

void vertices_translate(float* vertex_data, int vertices, float dx, float dy) {
	for (int i = 0; i != vertices; ++i) {
		vertex_data[0 + 4*i] += dx;
		vertex_data[1 + 4*i] += dy;
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	mscroll_y = yoffset;
}


int main(int argc, const char* argv[]) {
	
	printf("IMGV version %s (%s)\n", IMGV_VERSION, IMGV_DATE);

	if (argc != 2) {
		fprintf(stderr, "Error: missing input image\n");
		return 1;
	}

	const char* filename = argv[1];
	const char* buff;
	int width, height, channels;
	GLuint w_height = 720, w_width = 1280;

	GLFWwindow* window = WindowInit(w_width, w_height);
	if (!window) return 1;

	// Shader
	GLuint shader_id = shader_create();
	if (shader_id == 0) return 1;

	// Main image texture
	Texture *tex = texture_load(filename);
	if (!tex) return 1;

	// Vertex Buffer Object
	GLuint vbo_id;
	float fx = (float)tex->width / (float)w_width;
	float fy = (float)tex->height / (float)w_height;
	//Force to fit on screen
	float scale = 1.0f/fx < 1.0f/fy ? 1.0f/fx : 1.0f/fy;
	fx *= scale; fy *= scale;

	float vertices[] = {
		-1.0f * fx, -1.0f * fy,  0.0f, 0.0f,
		 1.0f * fx, -1.0f * fy,  1.0f, 0.0f,
		 1.0f * fx,  1.0f * fy,  1.0f, 1.0f,
		-1.0f * fx,  1.0f * fy,  0.0f, 1.0f
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

	double mouse_cur[2], mouse_prev[2];
	int lmb_state = GLFW_RELEASE, lmb_state_prev = GLFW_RELEASE;

	glfwGetCursorPos(window, &mouse_cur[0], &mouse_cur[1]);
	mouse_cur[0] = 2.0f * (mouse_cur[0] / (float)w_width) - 1.0f;
	mouse_cur[1] = - 2.0f * (mouse_cur[1] / (float)w_height) + 1.0f;

	mouse_prev[0] = mouse_cur[0];
	mouse_prev[1] = mouse_cur[1];
	
	lmb_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
	lmb_state_prev = lmb_state;

	glfwSetScrollCallback(window, scroll_callback);

	float zoom = 1.0f;

	// Rendering Loop
	while (!glfwWindowShouldClose(window)) {
		glClear(GL_COLOR_BUFFER_BIT);

		lmb_state_prev = lmb_state;
		lmb_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);

		mouse_prev[0] = mouse_cur[0];
		mouse_prev[1] = mouse_cur[1];
		glfwGetCursorPos(window, &mouse_cur[0], &mouse_cur[1]);
		mouse_cur[0] = 2.0f * ( mouse_cur[0] / (float)w_width ) - 1.0f;
		mouse_cur[1] = - 2.0f * ( mouse_cur[1] / (float)w_height ) + 1.0f;
		
		// Moving image
		if (lmb_state == GLFW_PRESS && lmb_state_prev == GLFW_PRESS) {
			double dx = mouse_cur[0] - mouse_prev[0];
			double dy = mouse_cur[1] - mouse_prev[1];
			vertices_translate(vertices, 4, (float)dx, (float)dy);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		}

		//update window dimensions
		glfwGetFramebufferSize(window, &w_width, &w_height);
		glViewport(0, 0, w_width, w_height);

		// Zooming in/out
		/*
		if (mscroll_y < 0) {
			zoom *= 0.99f;
			glViewport(0, 0, (GLuint)(w_width * zoom), (GLuint)(w_height * zoom));
		}
		else if (mscroll_y > 0) {
			zoom *= 1.01f;
			glViewport(0, 0, (GLuint)(w_width * zoom), (GLuint)(w_height * zoom));
		}
		else printf("haha yes\n");
		*/

		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
		update_vao_attributes(2, 2, sizeof(float) * 4);

		glDrawElements(GL_TRIANGLES, sizeof(indices), GL_UNSIGNED_INT, 0);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Deleting
	glDeleteBuffers(1, &vbo_id);
	glDeleteBuffers(1, &ibo_id);
	glDeleteVertexArrays(1, &vao_id);
	glDeleteProgram(shader_id);
	glDeleteTextures(1, &tex->id);
	glfwTerminate();

	return 0;
}
