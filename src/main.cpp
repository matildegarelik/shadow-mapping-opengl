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
Model model,model2,model3,model4,model5,model6;
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

glm::mat4 mti =  {{1.f,0.f,0.f,0.f},{0.f,1.f,0.f,0.f},{0.f,0.f,1.f,0.f},{0.f,0.f,0.f,1.f}};
void drawModel(Model &model, Shader &shader,glm::mat4 mt=mti);

// extra callbacks
void keyboardCallback(GLFWwindow* glfw_win, int key, int scancode, int action, int mods);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);


/// NUEVO
unsigned int depthMapFBO;
const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
unsigned int depthMap;
Shader shader_test,shader_phong;

//glm::vec4 lightPosition={-1.0,1.f,1.f,1.f};
glm::vec3 posChookity={0.f,0.f,0.f};
float near_plane = .6f, far_plane = 7.5f;
//glm::mat4 lightProjection = glm::ortho(-.50f, .50f, -.50f, .50f, near_plane, far_plane); 

/// NUEVO 2
bool rotate_autom=0;
float lightRotationAngle = /*glfwGetTime() * */0.5f;
float lightDistance = 2.0f;
float rotate, width=2.0f;
float bias= 0.f;
bool pcf=0, shadow_active=0;

int current_escena = 0;
const std::vector<std::string> escena_names = {"Chookity","Cubes","Teapot"};

int main() {
	
	// initialize window and setup callbacks
	window = Window(win_width,win_height,"Proyecto final",Window::fDefaults|Window::fBlend);
	setCommonCallbacks(window);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetKeyCallback(window, keyboardCallback);
	
	// setup OpenGL state and load shaders
	shader_silhouette = Shader ("shaders/silhouette");
	shader_texture = Shader ("shaders/texture");
	shader_phong = Shader("shaders/phong");
	
	shader_test=Shader("shaders/test");
	
	// load model and init instances
	model = Model::loadSingle("chookity");
	model2 = Model::loadSingle("track",Model::fNoTextures);
	model3 = Model::loadSingle("cubo1",Model::fNoTextures);
	model4 = Model::loadSingle("teapot",Model::fNoTextures);
	model5 = Model::loadSingle("fish",Model::fNoTextures);
	model6 = Model::loadSingle("esfera",Model::fNoTextures);
	
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
	
	// matriz transformacion pollo
	glm::mat4 mt1 =  {{.25f,0.f,0.f,0.f},{0.f,.25f,0.f,0.f},{0.f,0.f,.25f,0.f},{0.f,0.f,0.f,1.f}};
	// matriz transformacion piso
	glm::mat4 mt2 =  {{1.f,0.f,0.f,0.f},{0.f,1.f,0.f,0.f},{0.f,0.f,1.f,0.f},{0.f,-0.25f,0.f,1.f}};
	// matriz transformacion cubo 1
	glm::mat4 mt3 = {{0.2f, 0.f, 0.f, 0.f},{0.f, 0.2f, 0.f, 0.f},{0.f, 0.f, 0.2f, 0.f},{0.5f, 0.2f, 0.0f, 1.0f}};
	// matriz transformacion cubo 2
	glm::mat4 mt4 = {{0.15f, 0.f, 0.f, 0.f},{0.f, 0.15f, 0.f, 0.f},{0.f, 0.f, 0.15f, 0.f},{0.f, -0.1f, 0.0f, 1.0f}};
	// matriz transformacion cubo 3
	glm::mat4 mt5 = {{0.05f, 0.f, 0.f, 0.f},{0.f, 0.05f, 0.f, 0.f},{0.f, 0.f, 0.05f, 0.f},{0.1f, -0.17f, 0.5f, 1.0f}};
	float angle = glm::radians(45.0f);
	mt5 = glm::rotate(mt5, angle, {1.f,0.f,1.f});
	// matriz transformacion teapot
	glm::mat4 mt6 = {{.5f,0.f,0.f,0.f},{0.f,.5f,0.f,0.f},{0.f,0.f,.5f,0.f},{0.f,0.3f,0.f,1.f}};
	// matriz transformacion fish
	glm::mat4 mt7 =  {{.25f,0.f,0.f,0.f},{0.f,.25f,0.f,0.f},{0.f,0.f,.25f,0.f},{0.5f,0.1f,0.5f,1.f}};
	// matriz transformacion esfera
	glm::mat4 mt8 =  {{.25f,0.f,0.f,0.f},{0.f,.25f,0.f,0.f},{0.f,0.f,.25f,0.f},{-0.5f,0.f,0.5f,1.f}};
	// matriz transformacion cubo 4
	glm::mat4 mt9 = {{0.2f, 0.f, 0.f, 0.f},{0.f, 0.2f, 0.f, 0.f},{0.f, 0.f, 0.2f, 0.f},{0.1f, -0.17f, 0.5f, 1.0f}};
	angle = glm::radians(45.0f);
	mt9 = glm::rotate(mt9, angle, {1.f,0.f,1.f});
	// matriz transformacion pollo 2
	glm::mat4 mt10 =  {{.25f,0.f,0.f,0.f},{0.f,.25f,0.f,0.f},{0.f,0.f,.25f,0.f},{0.5f,0.f,0.3f,1.f}};
	// matriz transformacion esfera
	glm::mat4 mt11 =  {{.4f,0.f,0.f,0.f},{0.f,.4f,0.f,0.f},{0.f,0.f,.4f,0.f},{0.5f,0.15f,-0.5f,1.f}};
	// matriz transformacion cubo 4
	glm::mat4 mt12 = {{0.2f, 0.f, 0.f, 0.f},{0.f, 0.2f, 0.f, 0.f},{0.f, 0.f, 0.2f, 0.f},{-0.15f, -0.17f, -0.5f, 1.0f}};
	angle = glm::radians(45.0f);
	mt12 = glm::rotate(mt12, angle, {1.f,0.f,1.f});
	
	do {
		view_angle = std::min(std::max(view_angle,0.01f),1.72f);
				
		
		// 1
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glViewport(0,0,SHADOW_WIDTH,SHADOW_HEIGHT);
		glClear(GL_DEPTH_BUFFER_BIT);

		if (current_escena == 0){
			drawModel(model,shader_test,mt1);
		}
		if (current_escena == 1){
			drawModel(model3,shader_test,mt3);
			drawModel(model3,shader_test,mt4);
			drawModel(model3,shader_test,mt5);
		}
		if (current_escena == 2){
			drawModel(model,shader_test,mt10);
			drawModel(model3,shader_test,mt9);
			drawModel(model3,shader_test,mt12);
			drawModel(model4,shader_test,mt6);
//			drawModel(model5,shader_test,mt7);
			drawModel(model6,shader_test,mt8);
			drawModel(model6,shader_test,mt11);
		}
		drawModel(model2,shader_test,mt2);
		
		
		//glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		
		// 2
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0.8f,0.8f,0.7f,1.f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
		glViewport(0,0,win_width,win_height);
		shader_texture.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		
		if (current_escena == 0){
			if(model.texture.isOk()){
				drawModel(model,shader_texture,mt1);
			}else{
				drawModel(model,shader_phong,mt1);
			}
		}
		if (current_escena == 1){
			drawModel(model3,shader_phong,mt3);
			drawModel(model3,shader_phong,mt4);
			drawModel(model3,shader_phong,mt5);
		}
		if (current_escena == 2){
			if(model.texture.isOk()){
				drawModel(model,shader_texture,mt10);
			}else{
				drawModel(model,shader_phong,mt10);
			}
			drawModel(model3,shader_phong,mt9);
			drawModel(model3,shader_phong,mt12);
			drawModel(model4,shader_phong,mt6);
//			drawModel(model5,shader_phong,mt7);
			drawModel(model6,shader_phong,mt8);
			drawModel(model6,shader_phong,mt11);
		}
		if(model2.texture.isOk()){
			drawModel(model2,shader_texture,mt2);
		}else{
			drawModel(model2,shader_phong,mt2);
		}		
		
		draw_buffers.draw(win_width,win_height);
		
		// settings sub-window
		window.ImGuiDialog("Shadow Mapping",[&](){
			ImGui::Combo("Escena", &current_escena,escena_names);
			ImGui::Checkbox("Activate shadow",&shadow_active);
			ImGui::SliderFloat("LightProjection Width",&width,-1.5f,2.5f);
			ImGui::SliderFloat("Near plane",&near_plane,0.f,2.f);
			ImGui::SliderFloat("Far plane",&far_plane,5.f,15.f);
			ImGui::SliderFloat("Rotate manual",&rotate,0,8);
			ImGui::Checkbox("Rotate autom",&rotate_autom);
			ImGui::SliderFloat("Bias",&bias,0,0.2);
			ImGui::Checkbox("PCF",&pcf);
			
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

	/// NUEVO 2
	lightRotationAngle = rotate;
	if(rotate_autom) lightRotationAngle = glfwGetTime()*0.5f;
	
	lightDistance = 1.0f;
	float lightPosX = cos(lightRotationAngle) * lightDistance;
	float lightPosY = 1.f;
	float lightPosZ = sin(lightRotationAngle) * lightDistance;
	glm::vec4 lightPosition={lightPosX,lightPosY,lightPosZ,1.f};
	
	glm::mat4 lightView = glm::lookAt(glm::vec3(mats[0]*lightPosition), 
									  glm::vec3( 0.0f, 0.0f,  0.0f), 
									  glm::vec3( 0.0f, 1.0f,  0.0f));
	
	glm::mat4 lightProjection = glm::ortho(-width, width, -width, width, near_plane, far_plane); 
	
	shader.setUniform("lightViewMatrix",lightView);
	shader.setUniform("lightProjectionMatrix",lightProjection);
	
	shader.setUniform("bias",bias);
	shader.setUniform("pcf",pcf);
	shader.setUniform("shadow_active",shadow_active);
	
	shader.setUniform("depthTexture",0);
	
	// setup light and material
	shader.setLight(mats[0]*lightPosition, glm::vec3{1.f,1.f,1.f}, 0.4f);
	shader.setMaterial(model.material);
	if(model.texture.isOk()) {
		model.texture.bind(1);
		shader.setUniform("colorTexture",1);
	};
	
	// send geometry
	shader.setBuffers(model.buffers);
	model.buffers.draw();
}
