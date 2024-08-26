#include "Model.h"
#include<stb_image.h>
#include<RHI/RenderContext.h>

NAMESPACE_START
Model::Model(string const& path, bool gamma ) : gammaCorrection(gamma)
{
    loadModel(path);
}
Model::~Model() {

}

void Model::loadModel(string const& path)
{
    // read file via ASSIMP
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    // check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
    {
        cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
        string a = importer.GetErrorString();
        return;
    }
    // retrieve the directory path of the filepath
    directory = path.substr(0, path.find_last_of('/'));

    // process ASSIMP's root node recursively
    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
    // process each mesh located at the current node
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        // the node object only contains indices to index the actual objects in the scene. 
        // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(processMesh(mesh, scene));
    }
    // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }

}

Mesh* Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    // data to fill
    vector<glm::vec3> vertices; 
    vector<glm::vec3> normals; 
    vector<glm::vec2> uvs;
    vector<unsigned int> indices;
    vector<Texture2D> textures;
    

    // walk through each of the mesh's vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Mesh::Vertex vertex;
        glm::vec3 position; 
        // positions
        position.x = mesh->mVertices[i].x;
        position.y = mesh->mVertices[i].y;
        position.z = mesh->mVertices[i].z;
        vertices.push_back(position);
        // normals
        if (mesh->HasNormals())
        {
            glm::vec3 normal;
            normal.x = mesh->mNormals[i].x;
            normal.y = mesh->mNormals[i].y;
            normal.z = mesh->mNormals[i].z;
            normals.push_back(normal);
        }
        // texture coordinates
        if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
        {
            glm::vec2 uv;
            uv.x = mesh->mTextureCoords[0][i].x;
            uv.y = mesh->mTextureCoords[0][i].y;
            uvs.push_back(uv);
            // tangent
           /* vector.x = mesh->mTangents[i].x;
            vector.y = mesh->mTangents[i].y;
            vector.z = mesh->mTangents[i].z;
            vertex.tangent = vector;*/
            // bitangent
            /*vector.x = mesh->mBitangents[i].x;
            vector.y = mesh->mBitangents[i].y;
            vector.z = mesh->mBitangents[i].z;
            vertex.Bitangent = vector;*/
        }
        else
            vertex.uv = glm::vec2(0.0f, 0.0f);

    }
    // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        // retrieve all indices of the face and store them in the indices vector
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
   
    // return a mesh object created from the extracted mesh data
    Mesh* newMesh = new Mesh();
    //get mesh name
    newMesh->nowName=mesh->mName.C_Str();

    unsigned int numVertices = (unsigned int)(vertices.size());
    // get  ¡®position¡¯  pointer 
    glm::vec3* positionPtr = vertices.data();

    // get  ¡®normal¡¯  pointer
    glm::vec3* normalPtr = normals.data();

    // get  ¡®uv¡¯  pointer
    glm::vec2* uvPtr = uvs.data();
 

    // µ÷ÓÃ createVertextBuffer
    newMesh->createVertextBuffer(numVertices, positionPtr, normalPtr, uvPtr);
    newMesh->createTriangleIndexBuffer((unsigned int)(indices.size() / 3), indices.data());
    return newMesh;
}

NAMESPACE_END