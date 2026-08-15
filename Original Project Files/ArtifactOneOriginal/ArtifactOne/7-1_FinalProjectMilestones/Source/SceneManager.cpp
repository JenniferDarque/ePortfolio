///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
// Updated by Jennifer Swinton
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <GLFW/glfw3.h>
#include <glm/gtx/transform.hpp>
#include <algorithm> // for std::min

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
	: m_pShaderManager(pShaderManager),
	m_basicMeshes(new ShapeMeshes()),
	m_loadedTextures(0),          // <-- initialized here
	m_windowWidth(800),
	m_windowHeight(600),
	m_projectionMode(PERSPECTIVE),
	m_cameraPos(0.0f, 2.0f, 10.0f),
	m_cameraFront(0.0f, 0.0f, -1.0f),
	m_cameraUp(0.0f, 1.0f, 0.0f),
	m_yaw(-90.0f),
	m_pitch(0.0f),
	m_lastX(400.0f),
	m_lastY(300.0f),
	m_firstMouse(true),
	m_cameraSpeed(0.05f)
{
	// Clear texture slots
	for (int i = 0; i < 16; ++i) {
		m_textureIDs[i].ID = 0;
		m_textureIDs[i].tag = "";
	}
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}
/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// Always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// Force RGBA (4 channels) to avoid unsupported formats
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		STBI_rgb_alpha // force 4 channels
	);

	if (image && width > 0 && height > 0)
	{
		std::cout << "Successfully loaded image: " << filename
			<< ", width: " << width
			<< ", height: " << height
			<< ", channels: " << 4 << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// Set wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		// Set filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Upload texture data (always RGBA now)
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
			width, height, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, image);

		// Generate mipmaps
		glGenerateMipmap(GL_TEXTURE_2D);

		// Free local image memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind

		// Register texture
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image: " << filename
		<< " (" << stbi_failure_reason() << ")" << std::endl;

	if (image)
		stbi_image_free(image);

	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots. There are up to16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	// Nothing to do if no textures loaded
	if (m_loadedTextures <= 0)
		return;

	// Query the maximum number of combined texture image units supported
	GLint maxUnits = 0;
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxUnits);
	if (maxUnits <= 0)
		maxUnits = 16; // fallback conservative value

	// Ensure we don't exceed our array size or the GL limit
	int arraySize = static_cast<int>(sizeof(m_textureIDs) / sizeof(m_textureIDs[0]));
	int count = std::min(m_loadedTextures, std::min(arraySize, maxUnits));

	for (int i = 0; i < count; ++i)
	{
		const GLuint texID = m_textureIDs[i].ID;
		if (texID == 0)
		{
			// skip uninitialized texture slots
			continue;
		}

		// bind textures on corresponding texture units
		glActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + i));
		glBindTexture(GL_TEXTURE_2D, texID);
	}

	// Restore active texture to unit0 to avoid surprising state for callers
	glActiveTexture(GL_TEXTURE0);
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		if (m_textureIDs[i].ID != 0)
		{
			glDeleteTextures(1, &m_textureIDs[i].ID);
			m_textureIDs[i].ID = 0;
		}
	}
	m_loadedTextures = 0;
}


/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}
/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/
void SceneManager::ProcessMouseMovement(double xpos, double ypos)
{
	// Track mouse movement deltas
	static bool firstMouse = true;
	static double lastX = 0.0;
	static double lastY = 0.0;

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	double xoffset = xpos - lastX;
	double yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	m_yaw += static_cast<float>(xoffset);
	m_pitch += static_cast<float>(yoffset);

	// Constrain pitch
	if (m_pitch > 89.0f) m_pitch = 89.0f;
	if (m_pitch < -89.0f) m_pitch = -89.0f;

	// Update camera front vector
	glm::vec3 front;
	front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	front.y = sin(glm::radians(m_pitch));
	front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
	m_cameraFront = glm::normalize(front);
}
void SceneManager::ProcessMouseScroll(double yoffset)
{
	// Increase or decrease speed based on scroll direction
	m_cameraSpeed += static_cast<float>(yoffset) * 0.01f; // sensitivity factor

	// Clamp to avoid negative or zero speed
	if (m_cameraSpeed < 0.001f)
		m_cameraSpeed = 0.001f;
	if (m_cameraSpeed > 1.0f)
		m_cameraSpeed = 1.0f;
}


void SceneManager::ProcessInput(GLFWwindow* window)
{
	//set keyboard commands

	// Forward / Backward
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		m_cameraPos += m_cameraSpeed * m_cameraFront;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		m_cameraPos -= m_cameraSpeed * m_cameraFront;

	// Left / Right
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		m_cameraPos -= glm::normalize(glm::cross(m_cameraFront, m_cameraUp)) * m_cameraSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		m_cameraPos += glm::normalize(glm::cross(m_cameraFront, m_cameraUp)) * m_cameraSpeed;

	// Up / Down
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		m_cameraPos += m_cameraSpeed * m_cameraUp;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		m_cameraPos -= m_cameraSpeed * m_cameraUp;

	// --- Projection mode toggle ---
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
		m_projectionMode = PERSPECTIVE;
	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
		m_projectionMode = ORTHOGRAPHIC;


}




/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/

void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();                 // Monument base
	m_basicMeshes->LoadTaperedCylinderMesh();     // Central column
	m_basicMeshes->LoadConeMesh();                // Victory statue body
	m_basicMeshes->LoadSphereMesh();              // Victory statue head
	m_basicMeshes->LoadTorusMesh();				  // Steps
	m_basicMeshes->LoadPyramid4Mesh();			  // House roof

	CreateGLTexture("grass.jpg", "grass"); //https://www.freepik.com/free-vector/green-grass-vector-seamless-texture-lawn-nature-meadow-plant-field-natural-outdoor-illustration_11059458.htm#fromView=search&page=1&position=1&uuid=e66aa587-d6b9-40a2-8751-b3a6154e6c2c&query=seamless+grass
	CreateGLTexture("brick.jpg", "brick"); //https://www.pinterest.com/pin/78461218501785221/
	CreateGLTexture("cement.jpg", "cement"); //https://architextures.org/textures/4621
	CreateGLTexture("bronze.jpg", "bronze"); //https://www.freepik.com/free-photo/red-painted-wall-texture_1037358.htm#fromView=keyword&page=1&position=39&uuid=8948f90e-7595-486f-bdff-56e1cebabab3&query=Bronze+texture+seamless
	CreateGLTexture("cement2.jpg", "cement2"); //https://unsplash.com/photos/a-black-and-white-photo-of-a-concrete-wall-0-hFjFzo5VI
	CreateGLTexture("glass.jpg", "glass"); //https://www.freepik.com/free-vector/spotlight-background-free_711274.htm#fromView=keyword&page=1&position=26&uuid=0a146c70-c310-43e3-8012-beb1ce5516dd&query=Frosted+glass+texture
	CreateGLTexture("blackmetal.jpg", "blackmetal"); //https://cgaxis.com/product/black-scrathed-metal-pbr-texture/
	CreateGLTexture("water.jpg", "water");  //https://www.freepik.com/free-photo/pool-water-background_3573698.htm#fromView=keyword&page=1&position=36&uuid=085e5bbb-ad14-4e06-b05a-8286b84cc38a&query=Swimming+pool+water+texture
	CreateGLTexture("sky.jpg", "sky");  //https://www.freepik.com/free-vector/hand-painted-blue-sky-with-clouds-background_49607710.htm#fromView=keyword&page=1&position=3&uuid=08030079-3196-46ed-bf83-ec663f3a0d82&query=Seamless+sky
	CreateGLTexture("darkconcrete.jpg", "darkconcrete");
	CreateGLTexture("wood.jpg", "wood"); //https://stock.adobe.com/search?k=wood+door+texture
	CreateGLTexture("roof.jpg", "roof"); https://www.dreamstime.com/illustration/seamless-roof-texture.html

	BindGLTextures();

	// Initialize materials
	InitMaterials();


}
/***********************************************************
 *  InitMaterials()
 *
 *  Define reusable materials for the scene. These values
 *  are passed into the shader via SetShaderMaterial().
 ***********************************************************/
void SceneManager::InitMaterials()
{
	OBJECT_MATERIAL skyMaterial;

	skyMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	skyMaterial.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f); // bright base
	skyMaterial.shininess = 8.0f;
	skyMaterial.tag = "skyMaterial";

	m_objectMaterials.push_back(skyMaterial);

	OBJECT_MATERIAL grassMaterial;

	grassMaterial.diffuseColor = glm::vec3(0.4f, 0.8f, 0.4f);
	grassMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	grassMaterial.shininess = 32.0f; // reflective
	grassMaterial.tag = "grassMaterial";

	m_objectMaterials.push_back(grassMaterial);

	OBJECT_MATERIAL brickMaterial;

	brickMaterial.diffuseColor = glm::vec3(0.6f, 0.2f, 0.2f);
	brickMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	brickMaterial.shininess = 8.0f; // matte
	brickMaterial.tag = "brickMaterial";

	m_objectMaterials.push_back(brickMaterial);

	OBJECT_MATERIAL cementMaterial;
	
	cementMaterial.diffuseColor = glm::vec3(0.6f, 0.6f, 0.6f);
	cementMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	cementMaterial.shininess = 4.0f; // low reflection
	cementMaterial.tag = "cementMaterial";

	m_objectMaterials.push_back(cementMaterial);

	OBJECT_MATERIAL darkConcreteMaterial;
	darkConcreteMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	darkConcreteMaterial.specularColor = glm::vec3(0.2f, 0.2f, 0.2f);
	darkConcreteMaterial.shininess = 2.0f; // very matte
	darkConcreteMaterial.tag = "darkConcreteMaterial";

	m_objectMaterials.push_back(darkConcreteMaterial);

	OBJECT_MATERIAL bronzeMaterial;

	bronzeMaterial.diffuseColor = glm::vec3(0.55f, 0.47f, 0.14f);
	bronzeMaterial.specularColor = glm::vec3(0.8f, 0.8f, 0.6f);
	bronzeMaterial.shininess = 16.0f; // moderate highlights
	bronzeMaterial.tag = "bronzeMaterial";

	m_objectMaterials.push_back(bronzeMaterial);

	// Water (for pools)
	OBJECT_MATERIAL waterMaterial;
	waterMaterial.diffuseColor = glm::vec3(0.2f, 0.4f, 0.8f); // bluish tint
	waterMaterial.specularColor = glm::vec3(0.9f, 0.9f, 1.0f); // strong highlights
	waterMaterial.shininess = 128.0f; // very reflective
	waterMaterial.tag = "waterMaterial";
	m_objectMaterials.push_back(waterMaterial);

	OBJECT_MATERIAL blackmetalMaterial;

	blackmetalMaterial.diffuseColor = glm::vec3(0.05f, 0.05f, 0.05f); // very dark base
	blackmetalMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);    // subtle gray highlights
	blackmetalMaterial.shininess = 64.0f;                          // sharper reflections
	blackmetalMaterial.tag = "blackmetalMaterial";

	m_objectMaterials.push_back(blackmetalMaterial);

	OBJECT_MATERIAL glassMaterial;

	glassMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.diffuseColor = glm::vec3(0.6f, 0.7f, 0.8f);  // bright base
	glassMaterial.shininess = 16.0f;
	glassMaterial.tag = "glassMaterial";

	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL woodMaterial;

	woodMaterial.diffuseColor = glm::vec3(0.55f, 0.27f, 0.07f);
	woodMaterial.specularColor = glm::vec3(0.2f, 0.15f, 0.1f);
	woodMaterial.shininess = 8.0f; 
	woodMaterial.tag = "woodMaterial";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL roofMaterial;

	roofMaterial.diffuseColor = glm::vec3(0.55f, 0.27f, 0.07f);
	roofMaterial.specularColor = glm::vec3(0.2f, 0.15f, 0.1f);
	roofMaterial.shininess = 8.0f;
	roofMaterial.tag = "roofMaterial";

	m_objectMaterials.push_back(roofMaterial);
}



/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// --- Projection setup ---
	glm::mat4 projection;

	if (m_projectionMode == PERSPECTIVE)
	{
		// Perspective projection
		projection = glm::perspective(
			glm::radians(45.0f),          // field of view
			800.0f / 600.0f,              // aspect ratio (replace with actual window size)
			0.1f,                         // near plane
			100.0f                        // far plane
		);
	}
	else
	{
		// Orthographic projection
		float orthoHeight = 14.0f;   // smaller vertical extent
		float aspect = static_cast<float>(m_windowWidth) / static_cast<float>(m_windowHeight);

		projection = glm::ortho(
			-orthoHeight * aspect, orthoHeight * aspect,  // left, right
			-orthoHeight, orthoHeight,        // bottom, top (start at 0 so ground plane is excluded)
			0.1f, 100.0f
		);

	}

	if (m_pShaderManager)
		m_pShaderManager->setMat4Value("projection", projection);

	// --- View setup ---
	glm::mat4 view;
	if (m_projectionMode == PERSPECTIVE)
	{
		// Free camera
		view = glm::lookAt(m_cameraPos, m_cameraPos + m_cameraFront, m_cameraUp);
	}
	else
	{
		// Fixed orthographic view: look directly at the object
		view = glm::lookAt(
			glm::vec3(0.0f, 5.0f, 10.0f), // eye position
			glm::vec3(0.0f, 0.0f, 0.0f),  // look at origin
			glm::vec3(0.0f, 1.0f, 0.0f)   // up vector
		);
	}

	if (m_pShaderManager)
		m_pShaderManager->setMat4Value("view", view);

	// --- Lighting setup ---
	
	if (m_pShaderManager)
	{
		// Directional light (sunlight from above/right)
		m_pShaderManager->setVec3Value("directionalLight.direction", glm::vec3(0.3f, -5.0f, -0.2f));
		m_pShaderManager->setVec3Value("directionalLight.ambient", glm::vec3(0.15f, 0.15f, 0.15f));
		m_pShaderManager->setVec3Value("directionalLight.diffuse", glm::vec3(1.0f, 1.0f, 1.0f));
		m_pShaderManager->setVec3Value("directionalLight.specular", glm::vec3(1.2f, 1.2f, 1.0f));
		m_pShaderManager->setBoolValue("directionalLight.bActive", true);


		// Point light 0 – left round pool (blue uplight)
		m_pShaderManager->setVec3Value("pointLights[0].position", glm::vec3(-11.0f, 0.2f, 0.0f));
		m_pShaderManager->setVec3Value("pointLights[0].ambient", glm::vec3(0.02f, 0.02f, 0.05f));
		m_pShaderManager->setVec3Value("pointLights[0].diffuse", glm::vec3(0.3f, 0.3f, 0.9f));
		m_pShaderManager->setVec3Value("pointLights[0].specular", glm::vec3(0.4f, 0.4f, 1.0f));
		m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

		// Point light 1 – right round pool (blue uplight)
		m_pShaderManager->setVec3Value("pointLights[1].position", glm::vec3(11.0f, 0.2f, 0.0f));
		m_pShaderManager->setVec3Value("pointLights[1].ambient", glm::vec3(0.02f, 0.02f, 0.05f));
		m_pShaderManager->setVec3Value("pointLights[1].diffuse", glm::vec3(0.3f, 0.3f, 0.9f));
		m_pShaderManager->setVec3Value("pointLights[1].specular", glm::vec3(0.4f, 0.4f, 1.0f));
		m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

		// Point light 2 – left rectangular pool (green glow, moved outward)
		m_pShaderManager->setVec3Value("pointLights[2].position", glm::vec3(-7.5f, 0.3f, 0.0f)); // moved left
		m_pShaderManager->setVec3Value("pointLights[2].ambient", glm::vec3(0.02f, 0.05f, 0.02f));
		m_pShaderManager->setVec3Value("pointLights[2].diffuse", glm::vec3(0.2f, 0.8f, 0.2f));
		m_pShaderManager->setVec3Value("pointLights[2].specular", glm::vec3(0.3f, 1.0f, 0.3f));
		m_pShaderManager->setBoolValue("pointLights[2].bActive", true);

		// Point light 3 – right rectangular pool (green glow, moved outward)
		m_pShaderManager->setVec3Value("pointLights[3].position", glm::vec3(7.5f, 0.3f, 0.0f)); // moved right
		m_pShaderManager->setVec3Value("pointLights[3].ambient", glm::vec3(0.02f, 0.05f, 0.02f));
		m_pShaderManager->setVec3Value("pointLights[3].diffuse", glm::vec3(0.2f, 0.8f, 0.2f));
		m_pShaderManager->setVec3Value("pointLights[3].specular", glm::vec3(0.3f, 1.0f, 0.3f));
		m_pShaderManager->setBoolValue("pointLights[3].bActive", true);

		// Point light 4 – left lamppost head (bright red glow)
		m_pShaderManager->setVec3Value("pointLights[4].position", glm::vec3(-3.5f, 3.5f, 0.0f));
		m_pShaderManager->setVec3Value("pointLights[4].ambient", glm::vec3(0.25f, 0.05f, 0.05f)); // red ambient glow
		m_pShaderManager->setVec3Value("pointLights[4].diffuse", glm::vec3(2.0f, 0.2f, 0.2f));   // strong red illumination
		m_pShaderManager->setVec3Value("pointLights[4].specular", glm::vec3(2.5f, 0.3f, 0.3f));   // bright red highlights
		m_pShaderManager->setBoolValue("pointLights[4].bActive", true);

		// Point light 5 – right lamppost head (bright red glow)
		m_pShaderManager->setVec3Value("pointLights[5].position", glm::vec3(3.5f, 3.5f, 0.0f));
		m_pShaderManager->setVec3Value("pointLights[5].ambient", glm::vec3(0.25f, 0.05f, 0.05f));
		m_pShaderManager->setVec3Value("pointLights[5].diffuse", glm::vec3(2.0f, 0.2f, 0.2f));
		m_pShaderManager->setVec3Value("pointLights[5].specular", glm::vec3(2.5f, 0.3f, 0.3f));
		m_pShaderManager->setBoolValue("pointLights[5].bActive", true);


		// Enable lighting globally
		m_pShaderManager->setIntValue("bUseLighting", true);
	}


	// --- Transformation variables ---
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;


	/****************************************************************/
	// --- Skybox Setup ---
	glDepthMask(GL_FALSE); // prevent sky from blocking other objects

	scaleXYZ = glm::vec3(-100.0f, -100.0f, -100.0f); // flip normals inward
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("sky");                  // binds sky.jpg
	m_basicMeshes->DrawBoxMesh();         // draw skydome
	glDepthMask(GL_TRUE); // re-enable depth writes

	// Ground Plane
	scaleXYZ = glm::vec3(33.0f, 1.0f, 33.0f);
	positionXYZ = glm::vec3(0.0f, -0.1f, 0.0f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("grass");
	SetTextureUVScale(52.0f, 52.0f);
	SetShaderMaterial("grassMaterial");
	m_basicMeshes->DrawPlaneMesh();

	// Monument Base (raised to sit on step)
	scaleXYZ = glm::vec3(5.0f, 1.2f, 4.0f);
	positionXYZ = glm::vec3(0.0f, 0.95f, 0.0f); // was 0.5f, now lifted by ~0.45
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("brick");
	SetShaderMaterial("brickMaterial");
	m_basicMeshes->DrawBoxMesh();

	// Quarters (cement/dark concrete) — lifted by same offset
	scaleXYZ = glm::vec3(0.75f, 3.0f, 3.0f);
	positionXYZ = glm::vec3(-1.125f, 2.7f, 0.0f); // was 2.25f
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("cement");
	SetShaderMaterial("cementMaterial");
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(-0.375f, 2.7f, 0.0f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("darkconcrete");
	SetShaderMaterial("darkConcreteMaterial");
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(0.375f, 2.7f, 0.0f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("cement");
	SetShaderMaterial("cementMaterial");
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(1.125f, 2.7f, 0.0f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("darkconcrete");
	SetShaderMaterial("darkConcreteMaterial");
	m_basicMeshes->DrawBoxMesh();

	// Central Column (lifted)
	scaleXYZ = glm::vec3(1.5f, 8.0f, 1.5f);
	positionXYZ = glm::vec3(0.0f, 4.2f, 0.0f); // was 3.75f
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("cement");
	SetShaderMaterial("cementMaterial");
	m_basicMeshes->DrawTaperedCylinderMesh();

	// Statue Body (Cone) — lifted
	SetTransformations(glm::vec3(0.65f, 5.5f, 0.65f), 0, 0, 0, glm::vec3(0.0f, 10.95f, 0.0f)); // was 10.5f
	SetShaderColor(0.55f, 0.47f, 0.14f, 1.0f);
	SetShaderMaterial("bronzeMaterial");
	SetShaderTexture("bronze");
	m_basicMeshes->DrawConeMesh();

	// Statue Head (Sphere) — lifted
	SetTransformations(glm::vec3(0.1f, 0.1f, 0.1f), 0, 0, 0, glm::vec3(0.0f, 16.45f, 0.0f)); // was 16.0f
	SetShaderColor(0.55f, 0.47f, 0.14f, 1.0f);
	SetShaderMaterial("bronzeMaterial");
	SetShaderTexture("bronze"); 
	m_basicMeshes->DrawSphereMesh();


	// Left Pool (rounded end, horizontal)
	scaleXYZ = glm::vec3(3.0f, 0.2f, 3.0f);   // thin, wide, flattened
	positionXYZ = glm::vec3(-11.0f, 0.05f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 90.0f, 0.0f, positionXYZ);
	SetShaderTexture("water");
	SetShaderMaterial("waterMaterial");
	m_basicMeshes->DrawTaperedCylinderMesh();

	// Left Pool (rectangle part)
	scaleXYZ = glm::vec3(12.0f, 0.2f, 4.0f);   // thin, wide pool
	positionXYZ = glm::vec3(-4.5f, 0.05f, 0.0f); // left side of monument
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("water");
	SetShaderMaterial("waterMaterial"); 
	m_basicMeshes->DrawBoxMesh();

	// Right Pool (rounded end, horizontal)
	scaleXYZ = glm::vec3(3.0f, 0.2f, 3.0f);   // thin, wide, flattened
	positionXYZ = glm::vec3(11.0f, 0.01f, 0.0f);
	SetTransformations(scaleXYZ, 0.0f, 90.0f, 0.0f, positionXYZ);
	SetShaderTexture("water");
	SetShaderMaterial("waterMaterial");
	m_basicMeshes->DrawTaperedCylinderMesh();

	// Right Pool (rectangle part)
	scaleXYZ = glm::vec3(12.0f, 0.2f, 4.0f);   // thin, wide pool
	positionXYZ = glm::vec3(4.5f, 0.05f, 0.0f); // left side of monument
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("water");
	SetShaderMaterial("waterMaterial"); 
	m_basicMeshes->DrawBoxMesh();

	// --- Round Step around the base ---
	scaleXYZ = glm::vec3(7.0f, 0.3f, 7.0f);   // wide, thin cylinder
	positionXYZ = glm::vec3(0.0f, 0.01f, 0.0f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("cement");
	SetShaderMaterial("cementMaterial");
	SetTextureUVScale(15.0f, 15.0f);
	m_basicMeshes->DrawTaperedCylinderMesh();

	// --- Front Walkway ---
	scaleXYZ = glm::vec3(2.0f, 0.1f, 22.50f);   // narrow, thin, long path
	positionXYZ = glm::vec3(0.0f, 0.1f, -6.0f); // in front of monument, aligned with step height
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("brick");
	SetShaderMaterial("brickMaterial");
	m_basicMeshes->DrawBoxMesh();

	// --- Back Walkway ---
	scaleXYZ = glm::vec3(2.0f, 0.1f, 22.50f);   // same size
	positionXYZ = glm::vec3(0.0f, 0.1f, 6.0f); // behind monument
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("brick");
	SetShaderMaterial("brickMaterial");
	m_basicMeshes->DrawBoxMesh();

	// --- Outer Ring (Cement Street) ---
	scaleXYZ = glm::vec3(22.0f, 0.1f, 22.0f);   // large enough to surround pools + walkways
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("cement");
	SetShaderMaterial("cementMaterial");
	m_basicMeshes->DrawTaperedCylinderMesh();

	// --- Inner Cutout (Grass Plaza) ---
	scaleXYZ = glm::vec3(21.0f, 0.12f, 21.0f);   // slightly smaller than outer ring
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);  // just above cement
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("grass");
	SetShaderMaterial("grassMaterial");
	SetTextureUVScale(52.0f, 52.0f);
	m_basicMeshes->DrawTaperedCylinderMesh();



	// --- Left Lamppost ---
	scaleXYZ = glm::vec3(0.1f, 3.0f, 0.1f);   // thin, tall cylinder
	positionXYZ = glm::vec3(-3.5f, 0.35f, 0.0f); // left side of steps
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("blackmetal");
	SetShaderMaterial("blackmetalMaterial");
	m_basicMeshes->DrawTaperedCylinderMesh();

	// Lamp head (sphere)
	scaleXYZ = glm::vec3(0.20f, 0.20f, 0.20f);
	positionXYZ = glm::vec3(-3.5f, 3.5f, 0.0f); // top of pole
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("glass");
	SetShaderMaterial("glassMaterial");
	m_basicMeshes->DrawSphereMesh();

	// --- Right Lamppost ---
	scaleXYZ = glm::vec3(0.1f, 3.0f, 0.1f);
	positionXYZ = glm::vec3(3.5f, 0.35f, 0.0f); // right side of steps
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("blackmetal");
	SetShaderMaterial("blackmetalMaterial");
	m_basicMeshes->DrawTaperedCylinderMesh();

	// Lamp head (sphere)
	scaleXYZ = glm::vec3(0.20f, 0.20f, 0.20f);
	positionXYZ = glm::vec3(3.5f, 3.5f, 0.0f); // top of pole
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("glass");
	SetShaderMaterial("glassMaterial");
	m_basicMeshes->DrawSphereMesh();

	// --- Building Base ---
	scaleXYZ = glm::vec3(6.0f, 3.0f, 10.0f);   // width, height, depth
	positionXYZ = glm::vec3(-25.0f, 1.5f, 0.0f); // offset to the left of plaza
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("brick");
	SetShaderMaterial("brickMaterial");
	m_basicMeshes->DrawBoxMesh();

	// --- Door on right wall (side facing monument) ---
	scaleXYZ = glm::vec3(1.5f, 2.0f, 0.1f);   // wider and taller than window
	positionXYZ = glm::vec3(-22.0f, 1.0f, 0.0f); // right wall center
	SetTransformations(scaleXYZ, 0, 90.0f, 0, positionXYZ); // rotate to face outward
	SetShaderTexture("wood");
	SetShaderMaterial("woodMaterial");
	m_basicMeshes->DrawBoxMesh();

	// --- Window to the left of door (forward along Z) ---
	scaleXYZ = glm::vec3(1.0f, 1.0f, 0.1f);
	positionXYZ = glm::vec3(-22.0f, 1.5f, 2.5f); // right wall, forward
	SetTransformations(scaleXYZ, 0, 90.0f, 0, positionXYZ);
	SetShaderTexture("glass");
	SetShaderMaterial("glassMaterial");
	m_basicMeshes->DrawBoxMesh();

	// --- Window to the right of door (back along Z) ---
	scaleXYZ = glm::vec3(1.0f, 1.0f, 0.1f);
	positionXYZ = glm::vec3(-22.0f, 1.5f, -2.5f); // right wall, back
	SetTransformations(scaleXYZ, 0, 90.0f, 0, positionXYZ);
	SetShaderTexture("glass");
	SetShaderMaterial("glassMaterial");
	m_basicMeshes->DrawBoxMesh();

	// --- Single window on back face (far wall) ---
	scaleXYZ = glm::vec3(1.0f, 1.0f, 0.1f);
	positionXYZ = glm::vec3(-25.0f, 1.5f, -5.1f); // centered on back wall
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("glass");
	SetShaderMaterial("glassMaterial");
	m_basicMeshes->DrawBoxMesh();

	// --- Window on front wall ---
	scaleXYZ = glm::vec3(1.0f, 1.0f, 0.1f);   // same size as other windows
	positionXYZ = glm::vec3(-25.0f, 1.5f, 5.1f); // front wall, shifted right
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("glass");
	SetShaderMaterial("glassMaterial");
	m_basicMeshes->DrawBoxMesh();


	// --- Three windows on left wall ---
	for (int i = -2; i <= 2; i += 2) {
		scaleXYZ = glm::vec3(1.0f, 1.0f, 0.1f);
		positionXYZ = glm::vec3(-28.0f, 1.5f, i * 2.0f); // spaced along Z
		SetTransformations(scaleXYZ, 0, 90.0f, 0, positionXYZ);
		SetShaderTexture("glass");
		SetShaderMaterial("glassMaterial");
		m_basicMeshes->DrawBoxMesh();
	}


	// --- Roof ---
	scaleXYZ = glm::vec3(6.5f, 2.5f, 10.5f);   // slightly larger than building footprint
	positionXYZ = glm::vec3(-25.0f, 4.15f, 0.0f); // sits on top 
	SetTransformations(scaleXYZ, 0, 0, 0, positionXYZ);
	SetShaderTexture("roof");          // or define "roofMaterial"
	SetShaderMaterial("roofMaterial");
	m_basicMeshes->DrawPyramid4Mesh();         // pyramid-style roof


}
