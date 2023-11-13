#include <algorithm>
#include <stdexcept>
#include <vector>
#include <string>
#include <random>
#include <ctime>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include "Model.hpp"
#include "Window.hpp"
#include "Callbacks.hpp"
#include "Debug.hpp"
#include "Shaders.hpp"
#include "DrawBuffers.hpp"

#define VERSION 20221125

// window, models and settings
Window window;
DrawBuffers draw_buffers;
Model model,model2;
Texture alternative_texture;
struct Instance {
	glm::mat4 matrix;
	glm::vec3 color_var;
	float rot_speed;
	bool is_the_choosen_one;
};
std::vector<Instance> instances(256);
int selected_instance = -1;
double time_to_find_the_one;
Shader shader_texture, shader_silhouette;
float angle_object = 0.f, outline_width  = 0.125f;
int level = 1;
const std::vector<std::string> vlevels = { "Easy", "Medium", "Hard" };

glm::mat4 mti =  {{1.f,0.f,0.f,0.f},{0.f,1.f,0.f,0.f},{0.f,0.f,1.f,0.f},{0.f,0.f,0.f,1.f}};
void drawModel(Model &model, Shader &shader,glm::mat4 mt=mti);

// extra callbacks
void keyboardCallback(GLFWwindow* glfw_win, int key, int scancode, int action, int mods);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);


/// NUEVO
unsigned int depthMapFBO;
const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
unsigned int depthMap;
Shader shader_test;

glm::vec4 lightPosition={-1.0,1.f,1.f,1.f};
glm::vec3 posChookity={0.f,0.f,0.f};
float near_plane = 1.1f, far_plane = 7.5f;
glm::mat4 lightProjection = glm::ortho(-.50f, .50f, -.50f, .50f, near_plane, far_plane); 

int main() {
	
	// initialize window and setup callbacks
	window = Window(win_width,win_height,"Proyecto final",Window::fDefaults|Window::fBlend);
	setCommonCallbacks(window);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetKeyCallback(window, keyboardCallback);
	
	// setup OpenGL state and load shaders
	shader_silhouette = Shader ("shaders/silhouette");
	shader_texture = Shader ("shaders/texture");
	
	shader_test=Shader("shaders/test");
	
	// load model and init instances
	model = Model::loadSingle("chookity",Model::fNoTextures);
	model2 = Model::loadSingle("track",Model::fNoTextures);
	
	alternative_texture = Texture("models/choosen.png",0);
	
	// main loop
	FrameTimer ftime;
	view_target.y = -.5f;
//	view_pos.z *= 1.5;
	view_angle = .35;

	///NUEVO
	glGenFramebuffers(1, &depthMapFBO);
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 
				 SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);  
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	// matriz transformación pollo
	glm::mat4 mt1 =  {{1.f,0.f,0.f,0.f},{0.f,1.f,0.f,0.f},{0.f,0.f,1.f,0.f},{0.f,0.f,0.f,4.f}};
	// matriz transformación piso
	glm::mat4 mt2 =  {{1.f,0.f,0.f,0.f},{0.f,1.f,0.f,0.f},{0.f,0.f,1.f,0.f},{0.f,-0.25f,0.f,1.f}};
	
	do {
		glClearColor(0.8f,0.8f,0.7f,1.f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
		view_angle = std::min(std::max(view_angle,0.01f),1.72f);
				
		
		// 1
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		drawModel(model,shader_test,mt1);
		drawModel(model2,shader_test,mt2);
		
		//glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glViewport(0,0,win_width,win_height);
		// 2
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		shader_texture.use();
		glBindTexture(GL_TEXTURE_2D, depthMap);
		drawModel(model,shader_texture,mt1);
		drawModel(model2,shader_texture,mt2);
		
		draw_buffers.draw(win_width,win_height);
		
		// settings sub-window
		window.ImGuiDialog("Find The Choosen One!",[&](){
			ImGui::LabelText("","Time: %.3f s",time_to_find_the_one);
			ImGui::Combo("Level (L)", &level, vlevels);
			//if (ImGui::Button("Restart (R)")) initInstances();
			ImGui::SliderFloat("outline width",&outline_width,.05,.5);
			draw_buffers.addImGuiSettings(window);
		});
		glfwSwapInterval(1); 
		
		// finish frame
		glfwSwapBuffers(window);
		glfwPollEvents();
		
	} while( glfwGetKey(window,GLFW_KEY_ESCAPE)!=GLFW_PRESS && !glfwWindowShouldClose(window) );
}

void keyboardCallback(GLFWwindow* glfw_win, int key, int scancode, int action, int mods) {
	if (action==GLFW_PRESS) {
		switch (key) {
		//case 'R': initInstances(); break;
		case 'L': level = (level+1)%vlevels.size(); break;
		case 'B': draw_buffers.setNextBuffer(); break;
		}
	}
}


void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	common_callbacks::mouseButtonCallback(window,button,action,mods);
}

void drawModel(Model &model, Shader &shader,glm::mat4 mt) {
	shader.use();
	
	auto mats = common_callbacks::getMatrixes();
	
	shader.setMatrixes(
					   mats[0]*mt,
					   mats[1],
					   mats[2]);
	
	glm::mat4 lightView = glm::lookAt(glm::vec3(mats[0]*lightPosition), 
									  glm::vec3( 0.0f, 0.0f,  0.0f), 
									  glm::vec3( 0.0f, 1.0f,  0.0f));
	
	shader.setUniform("lightViewMatrix",lightView);
	shader.setUniform("lightProjectionMatrix",lightProjection);
	
	shader.setUniform("viewPos",view_pos);
	shader.setUniform("lightPos",lightPosition);
	
	// setup light and material
	shader.setLight(mats[0]*lightPosition, glm::vec3{1.f,1.f,1.f}, 0.4f);
	shader.setMaterial(model.material);
	
	// send geometry
	shader.setBuffers(model.buffers);
	model.buffers.draw();
}
