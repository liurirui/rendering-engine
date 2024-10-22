#include"Scene.h"
#include<iostream>
#include <algorithm>
NAMESPACE_START

std::string Scene::rootPath = "";
Scene:: Scene() {
	createModel(rootPath + "/resources/objects/nanosuit/nanosuit.obj");
}
void Scene::addRenderable(Renderable* newRenderable) {
	if (newRenderable == nullptr) {
		std::cout << "Cannot add a null Renderable to the scene." << std::endl;
		return; 
	}
	if (newRenderable->isTranslucent) {
		Translucent.push_back(newRenderable);
	}
	else if(!newRenderable->isTranslucent)Opaque.push_back(newRenderable);
}


float Scene::calculateDistance(glm::vec3 cameraPosition,glm::vec3 meshPosition) {
	return glm::length(cameraPosition - meshPosition);
}

// sort translucent meshes by their distance to the camera (far to near)
void Scene::sortTranslucentMeshes(glm::vec3 cameraPosition) {
	std::sort(Translucent.begin(), Translucent.end(),
		[cameraPosition, this](Renderable* a, Renderable* b) {
			return calculateDistance(cameraPosition, a->getWorldCenter()) > calculateDistance(cameraPosition, b->getWorldCenter());
		}
	);
}

void Scene::storeMeshes(Model* newModel) {
	for(Mesh* mesh:newModel->meshes){
		//Storing the model's mesh
		Renderable* renderable = nullptr;
		string meshName = mesh->nowName;
		if (mesh->nowName == "frame" || mesh->nowName == "glass_2") {
			newModel->modelNumber = 1;
			if (mesh->nowName == "frame")  renderable = new Renderable(mesh, false);
			else  renderable = new Renderable(mesh, true);
		}
		else if (mesh->nowName == "mask") {
			newModel->modelNumber = 2;
			renderable = new Renderable(mesh, true);
		}
		else if (mesh->nowName == "hands" || mesh->nowName == "Visor" || mesh->nowName == "Body" ||
			mesh->nowName == "Helmet" || mesh->nowName == "Legs" || mesh->nowName == "Arms"|| mesh->nowName == "Lights") {
			newModel->modelNumber = 3;
			renderable = new Renderable(mesh, false);
		}
		else if (mesh->nowName=="Mars_Cube.002") {
			newModel->modelNumber = 4;
			renderable = new Renderable(mesh, false);
		}
		else if (mesh->nowName == "Cube") {
			newModel->modelNumber = 5;
			renderable = new Renderable(mesh, false);
		}
		else if (mesh->nowName == "Cyborg") {
			newModel->modelNumber = 6;
			renderable = new Renderable(mesh, false);
		}
		else if (mesh->nowName == "VampireMesh") {
			newModel->modelNumber = 7;
			renderable = new Renderable(mesh, false);
		}
		else {
			newModel->modelNumber = 8;
			renderable = new Renderable(mesh, false);
		}
		if (renderable) {
			renderable->setTransformFromModel(newModel);
			renderable->modelNumber = newModel->modelNumber;
			addRenderable(renderable);
		}
	}
	model.push_back(newModel);
}

void Scene::createModel(const std::string& modelPath){
	Model* nowModel= new Model(modelPath);
	storeMeshes(nowModel);
}

void Scene::loadFloorTexture(const std::string& TexturePath) {
	if (meshRenderer->floor) {
		delete meshRenderer->floor;
		meshRenderer->floor = nullptr;  // Avoid dangling pointer problems
	}
	meshRenderer->floor= RenderContext::getInstance()->loadTexture2D(TexturePath.c_str());
}

void Scene::updateMeshTransform() {
	for (Model* singleModel : model) {
		if (singleModel->isTransformDirty) {
			singleModel->transform->calculateMatrix();
			for (Renderable* renderable : Translucent) {
				if (renderable->modelNumber == singleModel->modelNumber) renderable->setTransformFromModel(singleModel);
			}
			for (Renderable* renderable : Opaque) {
				if (renderable->modelNumber == singleModel->modelNumber) renderable->setTransformFromModel(singleModel);
			}
			singleModel->isTransformDirty = false;
		}
	}
}

void Scene::Start() {
	meshRenderer = new MeshRenderer(Translucent, Opaque);

	//loadTexture
	meshRenderer->ColorTextureMap["hands"] = RenderContext::getInstance()->loadTexture2D((rootPath + "/resources/objects/nanosuit/hand_dif.png").c_str());
	meshRenderer->ColorTextureMap["Visor"] = RenderContext::getInstance()->loadTexture2D((rootPath + "/resources/objects/nanosuit/glass_dif.png").c_str());
	meshRenderer->ColorTextureMap["Body"] = RenderContext::getInstance()->loadTexture2D((rootPath + "/resources/objects/nanosuit/body_dif.png").c_str());
	meshRenderer->ColorTextureMap["Helmet"] = RenderContext::getInstance()->loadTexture2D((rootPath + "/resources/objects/nanosuit/helmet_diff.png").c_str());
	meshRenderer->ColorTextureMap["Legs"] = RenderContext::getInstance()->loadTexture2D((rootPath + "/resources/objects/nanosuit/leg_dif.png").c_str());
	meshRenderer->ColorTextureMap["Arms"] = RenderContext::getInstance()->loadTexture2D((rootPath + "/resources/objects/nanosuit/arm_dif.png").c_str());
	meshRenderer->ColorTextureMap["glass"] = RenderContext::getInstance()->loadTexture2D((rootPath + "/resources/textures/skybox/back.jpg").c_str());
	meshRenderer->ColorTextureMap["app"] = RenderContext::getInstance()->loadTexture2D((rootPath + "/resources/textures/background.jpg").c_str());
	meshRenderer->ColorTextureMap["plane"] = RenderContext::getInstance()->loadTexture2D((Scene::rootPath + "/resources/textures/wood.png").c_str());
	meshRenderer->ColorTextureMap["Mars_Cube.002"] = RenderContext::getInstance()->loadTexture2D((Scene::rootPath + "/resources/objects/planet/mars.png").c_str());
	meshRenderer->ColorTextureMap["Cube"] = RenderContext::getInstance()->loadTexture2D((Scene::rootPath + "/resources/objects/rock/rock.png").c_str());
	meshRenderer->ColorTextureMap["Backpack"] = RenderContext::getInstance()->loadTexture2D((Scene::rootPath + "/resources/objects/backpack/diffuse.jpg").c_str());
	meshRenderer->ColorTextureMap["Cyborg"] = RenderContext::getInstance()->loadTexture2D((Scene::rootPath + "/resources/objects/cyborg/cyborg_diffuse.png").c_str());
	meshRenderer->ColorTextureMap["VampireMesh"] = RenderContext::getInstance()->loadTexture2D((Scene::rootPath + "/resources/objects/vampire/textures/Vampire_diffuse.png").c_str());
	meshRenderer->floor = RenderContext::getInstance()->loadTexture2D((Scene::rootPath + "/resources/textures/wood.png").c_str());       //Initialize the texture of the floor

	meshRenderer->depthStencilState.depthTest = true;
	meshRenderer->depthStencilState.depthWrite = true;

	//Shader
	meshRenderer->depthMapShader = TRefCountPtr<Shader>(new Shader(Vert_depth_map, Frag_depth_map));
	meshRenderer->lightingShader = TRefCountPtr<Shader>(new Shader(Vertmodel_lighting, Fragmodel_lighting));
	meshRenderer->lightingShader_cube = TRefCountPtr<Shader>(new Shader(Vertmodel_lighting, Fragmodel_cube));

	//set  Originframebuffer's Texture Attachments
	meshRenderer->fboColorTexture = RenderContext::getInstance()->createTexture2D(TextureUsage::RenderTarget, TextureFormat::RGBA32F, RenderContext::getInstance()->windowsWidth,
		RenderContext::getInstance()->windowsHeight);
	meshRenderer->fboDepthTexture = RenderContext::getInstance()->createTexture2D(TextureUsage::DepthStencil, TextureFormat::Depth24_Stencil8, RenderContext::getInstance()->windowsWidth,
		RenderContext::getInstance()->windowsHeight);
	ColorAttachment colorAttachment;
	colorAttachment.attachment = 0;
	colorAttachment.texture = meshRenderer->fboColorTexture;
	colorAttachment.clearColor = glm::vec4(0.1, 0.05, 0.15, 1);
	meshRenderer->OriginFramebuffer.colorAttachments.emplace_back(std::move(colorAttachment));
	meshRenderer->OriginFramebuffer.depthStencilAttachment.texture = meshRenderer->fboDepthTexture;
	meshRenderer->graphicsPipeline.shader = meshRenderer->lightingShader.getPtr();
	PipelineColorBlendAttachment pipelineColorBlendAttachment;
	pipelineColorBlendAttachment.blendState.enabled = true;
	meshRenderer->graphicsPipeline.rasterizationState.blendState.attachmentsBlendState.push_back(pipelineColorBlendAttachment);


	meshRenderer->graphicsPipeline_DepthMap.shader = meshRenderer->depthMapShader.getPtr();
	PipelineColorBlendAttachment pipelineColorBlendAttachment_DepthMap;
	pipelineColorBlendAttachment_DepthMap.blendState.enabled = true;
	meshRenderer->graphicsPipeline_DepthMap.rasterizationState.blendState.attachmentsBlendState.push_back(pipelineColorBlendAttachment);
	if (!meshRenderer->cubeVBO) {
		meshRenderer->cubeVBO = RenderContext::getInstance()->createVertexBuffer(meshRenderer->cubeVertices, sizeof(meshRenderer->cubeVertices));
	}
	if (!meshRenderer->cubeVAO) {
		meshRenderer->cubeVAO = RenderContext::getInstance()->createVertexBufferLayoutInfo(meshRenderer->cubeVBO);
		RenderContext::getInstance()->setUpVertexBufferLayoutInfo(meshRenderer->cubeVBO, meshRenderer->cubeVAO, 3, 8 * sizeof(float), 0, 0);
		RenderContext::getInstance()->setUpVertexBufferLayoutInfo(meshRenderer->cubeVBO, meshRenderer->cubeVAO, 3, 8 * sizeof(float), 1, 3);
		RenderContext::getInstance()->setUpVertexBufferLayoutInfo(meshRenderer->cubeVBO, meshRenderer->cubeVAO, 2, 8 * sizeof(float), 2, 6);
	}
	if (!meshRenderer->planeVBO) {
		meshRenderer->planeVBO = RenderContext::getInstance()->createVertexBuffer(meshRenderer->planeVertices, sizeof(meshRenderer->planeVertices));
	}
	if (!meshRenderer->planeVAO) {
		meshRenderer->planeVAO = RenderContext::getInstance()->createVertexBufferLayoutInfo(meshRenderer->planeVBO);
		RenderContext::getInstance()->setUpVertexBufferLayoutInfo(meshRenderer->planeVBO, meshRenderer->planeVAO, 3, 8 * sizeof(float), 0, 0);
		RenderContext::getInstance()->setUpVertexBufferLayoutInfo(meshRenderer->planeVBO, meshRenderer->planeVAO, 3, 8 * sizeof(float), 1, 3);
		RenderContext::getInstance()->setUpVertexBufferLayoutInfo(meshRenderer->planeVBO, meshRenderer->planeVAO, 2, 8 * sizeof(float), 2, 6);
	}
	meshRenderer->directionLight = new DirectionLight(glm::vec3(-0.5f, -0.8f, -0.5f), glm::vec3(2.0f, 2.0f, 2.0f), 1.0f);
	meshRenderer->pointLights.push_back(new PointLight(glm::vec3(0.0f, 6.0f, 5.0f), glm::vec3(15.0f, 0.0f, 0.0f), 0.4f));
	meshRenderer->pointLights.push_back(new PointLight(glm::vec3(-2.0f, 1.0f, -3.0f), glm::vec3(0.0f, 15.0f, 0.0f), 0.4f));
	meshRenderer->pointLights.push_back(new PointLight(glm::vec3(3.0f, 8.5f, 0.0f), glm::vec3(0.0f, 0.0f, 25.0f), 0.4f));
	meshRenderer->pointLights.push_back(new PointLight(glm::vec3(-8.0f, 3.0f, -1.0f), glm::vec3(6.0f, 6.0f, 6.0f), 0.3f));
}

void Scene::Update() {
	updateMeshTransform();
}

void Scene::Render(Camera* camera, RenderGraph& rg) {
	meshRenderer->render(camera, rg);
}

Scene::~Scene() {
	if(meshRenderer) delete meshRenderer;
	for (Renderable* renderable : Translucent) {
		delete renderable;
		renderable = nullptr;
	}
	for (Renderable* renderable : Opaque) {
		delete renderable;
		renderable = nullptr;
	}
	for (Model* singleModel : model) {
		delete singleModel;
	}
}

NAMESPACE_END