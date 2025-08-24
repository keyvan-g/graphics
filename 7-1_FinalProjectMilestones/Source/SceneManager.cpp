///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ================
// This file contains the implementation of the `SceneManager` class, which is 
// responsible for managing the preparation and rendering of 3D scenes. It 
// handles textures, materials, lighting configurations, and object rendering.
//
// AUTHOR: Brian Battersby
// INSTITUTION: Southern New Hampshire University (SNHU)
// COURSE: CS-330 Computational Graphics and Visualization
//
// INITIAL VERSION: November 1, 2023
// LAST REVISED: December 1, 2024
//
// RESPONSIBILITIES:
// - Load, bind, and manage textures in OpenGL.
// - Define materials and lighting properties for 3D objects.
// - Manage transformations and shader configurations.
// - Render complex 3D scenes using basic meshes.
//
// NOTE: This implementation leverages external libraries like `stb_image` for 
// texture loading and GLM for matrix and vector operations.
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

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
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
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

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
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
		glGenTextures(1, &m_textureIDs[i].ID);
	}
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

	modelView = translation * rotationZ * rotationY * rotationX * scale;

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

void SceneManager::DefineObjectMaterials()
{
	/*** STUDENTS - add the code BELOW for defining object materials. ***/
	/*** There is no limit to the number of object materials that can ***/
	/*** be defined. Refer to the code in the OpenGL Sample for help  ***/

	OBJECT_MATERIAL stoneMaterial;
	stoneMaterial.diffuseColor = glm::vec3(0.2f, 0.3f, 0.3f);
	stoneMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	stoneMaterial.shininess = 10.0;
	stoneMaterial.tag = "stone";

	m_objectMaterials.push_back(stoneMaterial);

	OBJECT_MATERIAL waterMaterial;
	waterMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.8f);
	waterMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.6f);
	waterMaterial.shininess = 50.0;
	waterMaterial.tag = "water";

	m_objectMaterials.push_back(waterMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f); 
	glassMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f); 
	glassMaterial.shininess = 80.0; 
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL metalMaterial;
	metalMaterial.diffuseColor = glm::vec3(0.7f, 0.7f, 0.7f);
	metalMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f);
	metalMaterial.shininess = 80.0;
	metalMaterial.tag = "metal";

	m_objectMaterials.push_back(metalMaterial);

}


/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// this line of code is NEEDED for telling the shaders to render 
	// the 3D scene with custom lighting, if no light sources have
	// been added then the display window will be black - to use the 
	// default OpenGL lighting then comment out the following line
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/*** STUDENTS - add the code BELOW for setting up light sources ***/
	/*** Up to four light sources can be defined. Refer to the code ***/
	/*** in the OpenGL Sample for help                              ***/
	m_pShaderManager->setVec3Value("pointLights[0].position", 5.0f, 5.0f, .0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.6f, 0.6f, 0.5f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.5f, 0.3f, 0.0f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	/***********************************************************
	 * Light source inside the light fixture on the back wall
	 ***********************************************************/
	m_pShaderManager->setVec3Value("pointLights[1].position", 4.0f, 4.0f, -6.0f);
	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.8f, 0.8f, 0.6f); // A warm, yellowish light
	m_pShaderManager->setVec3Value("pointLights[1].specular", .5f, .5f, 0.4f);
	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);


	m_pShaderManager->setVec3Value("directionalLight.direction", -1.0f, -2.0f, 2.0f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.5f, 0.5f, 0.5f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.7f, 0.7f, 0.7f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.3f, 0.3f, 0.3f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);


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
	// define the materials for objects in the scene
	DefineObjectMaterials();
	// add and define the light sources for the scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadSphereMesh();


	bool bReturn = false;

	LoadTextures(bReturn);

	BindGLTextures();

}

void SceneManager::LoadTextures(bool& bReturn)
{
	bReturn = CreateGLTexture(
		"textures/PavingStones138_1K-JPG_Color.jpg", "moss");
	bReturn = CreateGLTexture(
		"textures/PavingStones142_1K-JPG_Color.jpg", "paver");
	bReturn = CreateGLTexture(
		"textures/Asphalt031_1K-JPG_Color.jpg", "stonetop");
	bReturn = CreateGLTexture(
		"textures/Rocks011_1K-JPG_Color.jpg", "rock");
	bReturn = CreateGLTexture(
		"textures/water.jpg", "water");
	bReturn = CreateGLTexture(
		"textures/stone.jpg", "stone");

}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.6, 0.6, 0.6, 1);
	SetShaderTexture("paver");
	SetTextureUVScale(5.0f, 5.0f);
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();


	/****************************************************************/
	// Pool Water Surface Rendering

	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(16.0f, 0.5f, 8.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.0f, 0.1f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.6, 0.6, 0.6, 1);
	SetShaderTexture("water");
	SetTextureUVScale(1.f, 1.0f);
	SetShaderMaterial("water");

	// draw the mesh with transformation values
	m_basicMeshes->DrawBoxMesh();


	/****************************************************************/
	// Pool Wall Rendering
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 0.4f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.5f, .1f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.6, 0.6, 0.6, 1);
	SetShaderTexture("stone");
	SetTextureUVScale(2.f, 4.0f);
	SetShaderMaterial("stone");
	//m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::left);
	//m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::front);
	//m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::back);
	//m_basicMeshes->DrawBoxMeshSide(ShapeMeshes::BoxSide::right);
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/


	/***********************************************
	* Hot Tub Rendering
	*************************************************/

	
	// ****
	// Cylinder to represent water
	// ****
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.5f, 1.8f, 3.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.7, 0.7, 1.0, 1.0);
	SetShaderTexture("water");
	SetTextureUVScale(.1f, .1f);
	SetShaderMaterial("water");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(true, true, false);

	// *****
	// Cylinder to represent hot tub wall
	// *****
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.5f, 1.8f, 3.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 30.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderTexture("stone");
	SetTextureUVScale(2.0f, 4.f);
	SetShaderMaterial("stone");
	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(false, true, true);

	
	// ****
	// Cylinder to represent hot tub ledge
	// ****
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.50f, 0.3f, 3.5f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 180.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(5.0f, 2.1f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.2, 0.2, 0.2, 1.0);
	SetShaderTexture("stonetop");
	SetShaderMaterial("stone");

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(false, false, true);

	// ****
	// Cylinder to represent hot tub ledge
	// ****
	// 
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(3.51f, 0.3f, 3.51f);

	// draw the mesh with transformation values
	m_basicMeshes->DrawCylinderMesh(false, false, true);
	
		

	// ******
	// Hot tub spillover spout
	// ******

	scaleXYZ = glm::vec3(0.1f, 0.5f, 2.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 25.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f,1.5f,1.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.7, 0.7, 1.0, 1.0);
	SetShaderTexture("water");
	SetTextureUVScale(0.1f, 0.1f);
	SetShaderMaterial("water");
	m_basicMeshes->DrawBoxMesh();

	// ******
	// Hot tub waterfall 
	// ******

	scaleXYZ = glm::vec3(0.10f, 1.5f, 1.8f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 25.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(2.0f, 0.5f, 1.7f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.7, 0.7, 1.0, 1.0);
	SetShaderTexture("water");
	SetTextureUVScale(.1f, .01f);
	SetShaderMaterial("water");
	m_basicMeshes->DrawBoxMesh();


	/***********************************************/
	// Back Wall Rendering
	/***********************************************/
	scaleXYZ = glm::vec3(17.0f, 1.5f, 1.f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(-5.f, 1.1f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.2, 0.2, 0.2, 1.0);
	SetShaderTexture("paver");
	SetShaderMaterial("stone");
	SetTextureUVScale(4.0f, 2.0f);
	m_basicMeshes->DrawBoxMesh();

	/***********************************************/
	// Back Wall Tall - Rendering
	/***********************************************/
	scaleXYZ = glm::vec3(8.0f, 4.f, 1.f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(7.f, 1.1f, -6.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.2, 0.2, 0.2, 1.0);
	SetShaderTexture("paver");
	SetShaderMaterial("stone");
	SetTextureUVScale(4.0f, 2.0f);
	m_basicMeshes->DrawBoxMesh();

	/***********************************************/
	// Back Wall - Behind Spa Rendering
	/***********************************************/
	scaleXYZ = glm::vec3(12.0f, 4.f, 1.f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(11.f, 1.1f, -.5f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.2, 0.2, 0.2, 1.0);
	SetShaderTexture("paver");
	SetShaderMaterial("stone");
	SetTextureUVScale(4.0f, 2.0f);
	m_basicMeshes->DrawBoxMesh();
	
	/***********************************************/
	/* Hot tub platform */
	/*************************/

	scaleXYZ = glm::vec3(10.0f, 1.f, 6.f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 90.0f; // rotate to match texture pattern with background
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(7.5f, 0.1f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.2, 0.2, 0.2, 1.0);
	SetShaderTexture("paver");
	SetShaderMaterial("stone");
	SetTextureUVScale(2.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	/***********************************************/
	//Light Fixture Rendering
	/***********************************************/

	/***************************************************************/
	// Light Bulb Rendering (inside the fixture)
	/***************************************************************/

	m_pShaderManager->setBoolValue(g_UseLightingName, false);

	// set the XYZ scale for the mesh (make it smaller than the fixture)
	scaleXYZ = glm::vec3(.4f, .4f, .4f);
	// use the same position as the light source and fixture
	positionXYZ = glm::vec3(4.f, 4.0f, -6.f);
	// set the transformations for the bulb
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	// Set a bright yellow color for the bulb
	SetShaderColor(.8f, .8f, .7f, .7f);
	SetShaderMaterial("glass");
	// draw the bulb shape
	m_basicMeshes->DrawSphereMesh();
	m_pShaderManager->setBoolValue(g_UseLightingName, false);


	scaleXYZ = glm::vec3(0.2f, 1.7f, 0.2f);
	positionXYZ = glm::vec3(4.0f, 2.f, -6.0f); // Approximate position
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.3f, 0.3f, 0.3f, 1.0f); // Dark color for the post
	m_basicMeshes->DrawCylinderMesh(false, false, true);


	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(1.f, 1.5f, 1.f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(4.f, 4.0f, -6.f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.75, 0.75, 0.6, .5);
	//SetShaderTexture("paver");
	SetShaderMaterial("glass");
	SetTextureUVScale(4.0f, 2.0f);
	m_basicMeshes->DrawBoxMesh();

	

	/***********************************************/
	//  Torch Rendering
	/***********************************************/
	// Post
	scaleXYZ = glm::vec3(0.13f, 6.0f, 0.13f);
	positionXYZ = glm::vec3(10.0f, .1f, -5.0f); // Approximate position
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f); // Dark color for the post
	SetShaderMaterial("metal");
	m_basicMeshes->DrawCylinderMesh(false, false, true);

	// Lamp Head (simplified as a cone)
	scaleXYZ = glm::vec3(.6f, .6f, .6f);
	positionXYZ = glm::vec3(10.0f, 6.2f, -5.0f); // Above the post
	SetTransformations(scaleXYZ, 180.0f, 0.0f, 0.0f, positionXYZ);
	SetShaderColor(0.3f, 0.3f, 0.3f, 1.f);
	SetShaderMaterial("metal"); 
	m_basicMeshes->DrawConeMesh();

}
