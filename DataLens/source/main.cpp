#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <utility>
#include <optional>
#include <stdexcept>

#include "ResourceManager.h"
#include "Input.h"
#include "Selection.h"
#include "Parser.h"
#include "ColorCommand.h"
#include "bio/Protein.h"
#include "bio/PDBFile.h"
#include "bio/MoleculeData.h"
#include "bio/ConnectorType.h"
#include "math/Vec.h"
#include "math/Mat.h"
#include "math/MathUtils.h"
#include "graphics/Color.h"
#include "graphics/Window.h"
#include "graphics/Shader.h"
#include "graphics/SphereTemplate.h"
#include "graphics/ConnectorTemplate.h"
#include "graphics/Model.h"
#include <FrameCounter.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "nlohmann/json.hpp"
// #include "inspector/BehaviorInspector.h"
// #include "assistant/APIHandler.h"
#include "openai.hpp"

using json = nlohmann::json;
std::vector<std::string> commands;
bool shouldExit = false;
std::string apiKey = "apikey";

/*
 *  Runs the graphics rendering loop in a separate thread.
 *  This function initializes GLFW, GLEW, and OpenGL, creates a window,
 *  loads shaders, prepares the molecular model, and renders it in a loop.
 */
void renderingThread() {
	// Initialize GLFW (windowing library)
	if (!ResourceManager::initGLFW(3, 3)) {
		std::cin.get();
		return;
	}

	// Create a window
	Window window("Datalens", 1400, 960, false);

	// Initialize GLEW (OpenGL extension wrangler)
	if (!ResourceManager::initGLEW()) {
		std::cin.get();
		return;
	}
	
	// Initialize OpenGL
	ResourceManager::initOpenGL();

	// Load default shaders
	Shader::loadDefaultShaders();
		
	//Prepare Model
	SphereTemplate sphereTemplate; // Create a sphere template for atoms
	Model::setSphereTemplate(&sphereTemplate);  // Set the sphere template for the model

	ConnectorTemplate connectorTemplate; // Create a connector template for bonds
	Model::setConnectorTemplate(&connectorTemplate); // Set the connector template for the model

	Model::genBuffers(); // Generate OpenGL buffers for the model
	Model::fillSphereTemplateBuffer(); // Fill the sphere template buffer with data	 
	 
	MoleculeData *moleculeData = nullptr; // Pointer to store the loaded molecule data (initially null)
	Camera camera(Vec3(0.0f, 0.0f, 15.0f));  // Create a camera object
	Selection selection; // Create a selection object
	 
	double prevMouseX = 0.0; // Store the previous mouse X position
	double prevMouseY = 0.0; // Store the previous mouse Y position
	bool prevMousePosSet = false; // Flag to indicate if the previous mouse position has been set
	FrameCounter frameCounter; // Frame counter to calculate delta time

	// Main rendering loop
	while (!window.shouldClose() && !shouldExit) {
		//Handle window input
		Input::pollInput(&window); // Poll for input events (keyboard, mouse)

		camera.zoom((float)Input::getMouseScrollOffsetY()); // Zoom the camera based on the mouse scroll wheel

		double mouseX = Input::getMouseX(); // Get the current mouse X position
		double mouseY = Input::getMouseY(); // Get the current mouse Y position
		
		// If the left mouse button is pressed
		if (prevMousePosSet) {
			if (Input::mouseButtonPressed(&window, MouseButton::LEFT)) {
				if (
					Input::keyPressed(&window, Key::LEFT_CTRL) ||
					Input::keyPressed(&window, Key::RIGHT_CTRL)
				) {
					camera.move( // Move the camera based on the mouse movement
						Vec3(
							float(prevMouseX - mouseX) / 100,
							float(prevMouseY - mouseY) / -100,
							0.0f
						)
					);
				}
				else {
					Model::rotate( // Rotate the model based on the mouse movement
						Vec3(
							float(prevMouseY - mouseY) / 100,
							float(prevMouseX - mouseX) / 100,
							0.0f
						)
					);
				}
			}
			else if (Input::mouseButtonPressed(&window, MouseButton::RIGHT)) { // If the right mouse button is pressed
				Model::rotate(Vec3(0.0f, 0.0f, float(prevMouseX - mouseX) / 100)); // Rotate the model around the Z axis based on the mouse movement
			}
		}
		prevMouseX = mouseX;  // Update the previous mouse X position
		prevMouseY = mouseY;  // Update the previous mouse Y position
		prevMousePosSet = true; // Set the flag to indicate that the previous mouse position has been set

		/* Rotatable attribute check
		*/
		bool isRotatable = false; 

		//Read commands sent from console
		/*
		TODOS: 
		[1] Adding commands for loading CFD
		[2] Adding commands for model rotation/zoom
		*/ 
		for (size_t i = 0; i < commands.size(); ++i) {
			std::vector<std::string> commandWords =
				Parser::split(Parser::lowercase(commands[0]), ' ');

			if (commandWords.size() == 3 && commandWords[0] == "fetch") {
				if (commandWords[1] == "pdb") {
					delete moleculeData;
					moleculeData = nullptr;

					std::string url;
					if (commandWords[2].size() == 4) {
						url = "http://files.rcsb.org/view/" + commandWords[2] + ".pdb";
					}
					else {
						url = commandWords[2];
					}

					moleculeData = new PDBFile(url);
					Model::loadMoleculeData(moleculeData);
					selection.reset();
				}
			}
			else if (commandWords.size() == 2 && commandWords[0] == "rotate") {
				isRotatable = true;
				if (isRotatable) {
					auto const rotationAnlg = frameCounter.deltaTime * 10.0f;
					// std::cout << ("Rotation Angl = ", rotationAnlg) << std::endl;
					const float TOTAL_ROTATION_DEGREES = 20.0f;
					const int NUMBER_OF_STEPS = 10;
					if (commandWords[1] == "x") {
						for (int i = 0; i < NUMBER_OF_STEPS; i++) {
							Model::rotate(Vec3(
								TOTAL_ROTATION_DEGREES / NUMBER_OF_STEPS, // Rotate by 4 degrees each step
								0.0f, // Rotate by 4 degrees each step
								0.0f
							));
						}
					}
					if (commandWords[1] == "y") {
						for (int i = 0; i < NUMBER_OF_STEPS; i++) {
							Model::rotate(Vec3(
								0.0f,
								TOTAL_ROTATION_DEGREES / NUMBER_OF_STEPS, // Rotate by 4 degrees each step
								0.0f
							));
						}
					}
					if (commandWords[1] == "z") {

						for (int i = 0; i < NUMBER_OF_STEPS; i++) {
							Model::rotate(Vec3(
								0.0f,
								0.0f, // Rotate by 4 degrees each step
								TOTAL_ROTATION_DEGREES / NUMBER_OF_STEPS
							));
						}
					}
				}
				
			}
			else if (commandWords.size() == 1 && commandWords[0] == "leave") {
				delete moleculeData;
				moleculeData = nullptr;

				Model::reset();
				camera.reset();
				selection.reset();
			}
			else if (commandWords.size() == 1 && commandWords[0] == "chat") {
				while (commands[commands.size() - 1] != "endchat") {
					
					std::string prompt;
					std::cout << "You: ";
					std::getline(std::cin, prompt);
					
					try {

						auto json_string = R"({
							"model": "gpt-3.5-turbo-instruct",
							"prompt": ")" + commands[commands.size() - 1] + R"(",
							"max_tokens": 500,
							"temperature": 0
						})";
						json json_string2 = json::parse(json_string);
						auto completion = openai::completion().create(json(json_string2));
						std::string response = completion["choices"][0]["text"];
						std::cout << "AI Assistant: " << completion["choices"][0]["text"] << std::endl;

					}
					catch (const std::exception& e) {
						std::cerr << "Error: " << e.what() << std::endl;
					}
					
					
				}
				if (commands[commands.size() - 1] == "endchat") {
					std::cout << "You left the convo" << std::endl;
				}
				
			}
			else if (commandWords.size() == 1 && commandWords[0] == "setdefault") {
				selection.reset();
				Model::setAtomRadius(SphereTemplate::DEFAULT_RADIUS, &selection);
				Model::setConnectorRadius(
					ConnectorTemplate::DEFAULT_RADIUS,
					&selection,
					ConnectorType::BACKBONE
				);
				Model::setConnectorRadius(
					0.0f,
					&selection,
					ConnectorType::DISULFIDE_BOND
				);
				Model::colorAtomsDefault(&selection);
				Model::colorConnectorsDefault(&selection, ConnectorType::BACKBONE);
				Model::colorConnectorsDefault(&selection, ConnectorType::DISULFIDE_BOND);
			}
			else {
				std::cerr << "Err > Invalid command\n\n";
			}

			commands.erase(commands.begin()); // Remove the command from the vector
		}

		Window::clear(0, 0, 0); // Clear the window
		Model::render(Shader::getSphereDefault(), Shader::getConnectorDefault(), &window, &camera); // Render the model
		window.swapBuffers(); // Swap the window buffers
	}

	Shader::freeResources(); // Free the shader resources
	ResourceManager::freeResources(); // Free the resources
}

/*
 *  Main function of the program.
 *  This function initializes the OpenAI API, creates a graphics thread,
 *  reads commands from the console, and waits for the graphics thread to finish.
 *  @return 0 if the program executed successfully.
 */
int main() {
	openai::start(apiKey); // Do not include real API key in production code

	std::thread graphicsThread(renderingThread);

	std::string command;
	while (std::getline(std::cin, command)) {
		if (command == "exit") {
			shouldExit = true;
			std::cout << "====> Exiting....\n" << std::endl;
			break;
		}
		if (command.size() > 0) {
			commands.push_back(command);
		}
	}

	graphicsThread.join();

	return 0;
}
