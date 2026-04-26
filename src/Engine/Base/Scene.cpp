#include"Scene.h"
#include<iostream>
#include <algorithm>
NAMESPACE_START
void Scene::addRenderable(Renderable* newRenderable) {
	
}

float Scene::calculateDistance(glm::vec3 cameraPosition,glm::vec3 meshPosition) {
	return glm::length(cameraPosition - meshPosition);
}

// sort translucent meshes by their distance to the camera (far to near)
void Scene::sortTranslucentMeshes(glm::vec3 cameraPosition) {
	
}

void Scene::storeObjectMeshes(GameObject* go) {
	if (!go->meshes.empty()) renderableObjects.emplace_back(go);
	for (auto children : go->child) {
		storeObjectMeshes(children);
	}
}

void Scene::createModel(const std::string& modelPath){
	Model* nowModel= new Model(modelPath);
	addRootChild(nowModel->model_go);
	storeObjectMeshes(nowModel->model_go);
}

void Scene::loadFloorTexture(const std::string& TexturePath) {
	if (meshRenderer->floor) {
		delete meshRenderer->floor;
		meshRenderer->floor = nullptr;  // Avoid dangling pointer problems
	}
	meshRenderer->floor= RenderContext::getInstance()->loadTexture2D(TexturePath.c_str());
}

void Scene::updateMeshTransform() {
	
}

DirectionLight* Scene::GetMainDirectionalLight()const {
	return mainDirectionalLight;
}

Light* Scene::AddLight(LightType type, const glm::vec3& param, const glm::vec3& color, float intensity) {
	Light* newLight = nullptr;

	switch (type) {
		case LightType::Direction: {
			DirectionLight* dirLight = new DirectionLight(param, color, intensity);
			newLight = dirLight;

			if (!mainDirectionalLight) {
				mainDirectionalLight = static_cast<DirectionLight*>(newLight);
			}
			break;
		}
		case LightType::Point: {
			PointLight* pointLight = new PointLight(param, color, intensity);
			newLight = pointLight;
			break;
		}
		case LightType::Spot: {
			// ´ýÌí¼Ó
			break;
		}
	}
	if (newLight) {
		lights.push_back(newLight);
	}
	return newLight;
}

void Scene::Start() {
	meshRenderer = new MeshRenderer();
	meshRenderer->scene = this;

	meshRenderer->floor = RenderContext::getInstance()->loadTexture2D (((std::string("D:/ProgrammingTools/VS2022/Project/rendering-engine") + std::string("/resources/textures/wood.png")).c_str()));       //Initialize the texture of the floor

	meshRenderer->depthStencilState.depthTest = true;
	meshRenderer->depthStencilState.depthWrite = true;

	//Shader
	meshRenderer->depthMapShader = TRefCountPtr<Shader>(new Shader(Vert_depth_map, Frag_depth_map));
	meshRenderer->lightingShader = TRefCountPtr<Shader>(new Shader(Vertmodel_lighting, Fragmodel_lighting));
	meshRenderer->lightingShader_cube = TRefCountPtr<Shader>(new Shader(Vertmodel_lighting, Fragmodel_cube));

	//set  Originframebuffer's Texture Attachments
	SamplerInfo depthSampler;
	depthSampler.mipmapMode = MipmapMode::None;
	meshRenderer->fboColorTexture = RenderContext::getInstance()->createTexture2D(TextureUsage::RenderTarget, TextureFormat::RGBA32F, RenderContext::getInstance()->windowsWidth,
		RenderContext::getInstance()->windowsHeight);
	meshRenderer->fboDepthTexture = RenderContext::getInstance()->createTexture2D(TextureUsage::DepthStencil, TextureFormat::Depth24_Stencil8, RenderContext::getInstance()->windowsWidth,
		RenderContext::getInstance()->windowsHeight, depthSampler);
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
	meshRenderer->graphicsPipeline_DepthMap.rasterizationState.cullMode = CullMode::Back;

	AddLight(LightType::Direction, glm::vec3(-0.5f, -0.8f, -0.5f), glm::vec3(2.0f, 2.0f, 2.0f), 1.0f);
	AddLight(LightType::Point, glm::vec3(0.0f, 6.0f, 5.0f), glm::vec3(6.0f, 0.0f, 0.0f), 0.4f);
	AddLight(LightType::Point, glm::vec3(-2.0f, 1.0f, -3.0f), glm::vec3(0.0f, 9.0f, 0.0f), 0.4f);
	AddLight(LightType::Point, glm::vec3(3.0f, 8.5f, 0.0f), glm::vec3(0.0f, 0.0f, 25.0f), 0.2f);
	AddLight(LightType::Point, glm::vec3(-8.0f, 3.0f, -1.0f), glm::vec3(6.0f, 6.0f, 6.0f), 0.3f);

	if (!meshRenderer->cubeVBO) {
		meshRenderer->cubeVBO = RenderContext::getInstance()->createVertexBuffer(meshRenderer->cubeVertices, sizeof(meshRenderer->cubeVertices));
	}
	if (!meshRenderer->cubeVAO) {
		meshRenderer->cubeVAO = RenderContext::getInstance()->createVertexArray(meshRenderer->cubeVBO);
		RenderContext::getInstance()->setUpVertexBufferLayoutInfo(meshRenderer->cubeVBO, meshRenderer->cubeVAO, 3, 8 * sizeof(float), 0, 0);
		RenderContext::getInstance()->setUpVertexBufferLayoutInfo(meshRenderer->cubeVBO, meshRenderer->cubeVAO, 3, 8 * sizeof(float), 1, 3);
		RenderContext::getInstance()->setUpVertexBufferLayoutInfo(meshRenderer->cubeVBO, meshRenderer->cubeVAO, 2, 8 * sizeof(float), 2, 6);
	}
	if (!meshRenderer->planeVBO) {
		meshRenderer->planeVBO = RenderContext::getInstance()->createVertexBuffer(meshRenderer->planeVertices, sizeof(meshRenderer->planeVertices));
	}
	if (!meshRenderer->planeVAO) {
		meshRenderer->planeVAO = RenderContext::getInstance()->createVertexArray(meshRenderer->planeVBO);
		RenderContext::getInstance()->setUpVertexBufferLayoutInfo(meshRenderer->planeVBO, meshRenderer->planeVAO, 3, 8 * sizeof(float), 0, 0);
		RenderContext::getInstance()->setUpVertexBufferLayoutInfo(meshRenderer->planeVBO, meshRenderer->planeVAO, 3, 8 * sizeof(float), 1, 3);
		RenderContext::getInstance()->setUpVertexBufferLayoutInfo(meshRenderer->planeVBO, meshRenderer->planeVAO, 2, 8 * sizeof(float), 2, 6);
	}
}

void Scene::Update() {
	UpdateTransform(root);
	//updateMeshTransform();
}

void Scene::UpdateTransform(GameObject* go) {
	if (!go->GetTransform()) return;
	glm::mat4 parentWorldMaterix = glm::mat4(1.0);
	if (go->parent) parentWorldMaterix = go->parent->GetTransform()->worldMaterix;
	go->GetTransform()->worldMaterix = parentWorldMaterix * go->GetTransform()->getLocalMatrix();
	for (auto children : go->child) {
		UpdateTransform(children);
	}
	//updateMeshTransform();
}

void Scene::RenderObject() {
	for (auto go : renderableObjects) {
		for (auto mesh : go->meshes) {
			mesh->material->setUniform();
			mesh->material->shader.setMat4("model", go->GetTransform()->worldMaterix);
			mesh->draw();
		}
	}
}

void Scene::Render(Camera* camera, RenderGraph& rg) {
	meshRenderer->render(camera, rg);
	
}

NAMESPACE_END