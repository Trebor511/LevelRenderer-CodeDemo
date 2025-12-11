//load_object_oriented
// This is a sample of how to load a level in a object oriented fashion.
// Feel free to use this code as a base and tweak it for your needs.

// This reads .h2b files which are optimized binary .obj+.mtl files
#include "h2bParser.h"
#include "../gateware-main/gateware-main/Gateware.h"

// class Model contains everyhting needed to draw a single 3D model
struct SceneData 
{
	GW::MATH::GVECTORF _lightDirection, _lightColor, _sunAmbient, _cameraPos;
	GW::MATH::GMATRIXF viewMatrix, projectionMatrix;
};

struct MeshData
{
	GW::MATH::GMATRIXF worldMatrix;
	H2B::ATTRIBUTES material;

};

void PrintLabeledDebugString(const char* label, const char* toPrint)
{
	std::cout << label << toPrint << std::endl;
#if defined WIN32 //OutputDebugStringA is a windows-only function 
	OutputDebugStringA(label);
	OutputDebugStringA(toPrint);
#endif
}


class Model {
public:
	// Name of the Model in the GameLevel (useful for debugging)
	std::string name;
	// Loads and stores CPU model data from .h2b file
	H2B::Parser cpuModel; // reads the .h2b format
	// Shader variables needed by this model. 
	GW::MATH::GMATRIXF world;// TODO: Add matrix/light/etc vars..
	// TODO: API Rendering vars here (unique to this model)
	// Vertex Buffer
	// Index Buffer
	// Pipeline/State Objects
	// Uniform/ShaderVariable Buffer
	// Vertex/Pixel Shaders
	GW::GRAPHICS::GDirectX11Surface d3d;

	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> meshBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> sceneBuffer;
	
	Microsoft::WRL::ComPtr<ID3D11VertexShader>	vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>	pixelShader;

	Microsoft::WRL::ComPtr<ID3D11InputLayout>	vertexFormat;

	SceneData theScene;
	MeshData theMesh;

	void InitializeVertexBuffer(ID3D11Device* creator)
	{
		CreateVertexBuffer(creator, cpuModel.vertices.data(), sizeof(H2B::VERTEX) * cpuModel.vertices.size());
	}

	void InitializeIndexBuffer(ID3D11Device* creator)
	{
		CreateIndexBuffer(creator, cpuModel.indices.data(), sizeof(unsigned int) * cpuModel.indices.size());
	}



	void CreateConstBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes, Microsoft::WRL::ComPtr<ID3D11Buffer>& bufferType)
	{
		D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
		CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
		creator->CreateBuffer(&bDesc, &bData, bufferType.GetAddressOf());
	}

	void CreateIndexBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
	{
		D3D11_SUBRESOURCE_DATA iData = { data, 0, 0 };
		CD3D11_BUFFER_DESC iDesc(sizeInBytes, D3D11_BIND_INDEX_BUFFER);
		creator->CreateBuffer(&iDesc, &iData, indexBuffer.GetAddressOf());
	}

	void CreateVertexBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
	{
		D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
		CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_VERTEX_BUFFER);
		creator->CreateBuffer(&bDesc, &bData, vertexBuffer.GetAddressOf());
	}

	void InitializePipeline(ID3D11Device* creator)
	{
		UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
		compilerFlags |= D3DCOMPILE_DEBUG;
#endif
		Microsoft::WRL::ComPtr<ID3DBlob> vsBlob = CompileVertexShader(creator, compilerFlags);
		Microsoft::WRL::ComPtr<ID3DBlob> psBlob = CompilePixelShader(creator, compilerFlags);

		CreateVertexInputLayout(creator, vsBlob);
	}

	Microsoft::WRL::ComPtr<ID3DBlob> CompileVertexShader(ID3D11Device* creator, UINT compilerFlags)
	{
		std::string vertexShaderSource = ReadFileIntoString("../Shaders/VertexShader.hlsl");

		Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, errors;

		HRESULT compilationResult =
			D3DCompile(vertexShaderSource.c_str(), vertexShaderSource.length(),
				nullptr, nullptr, nullptr, "main", "vs_4_0", compilerFlags, 0,
				vsBlob.GetAddressOf(), errors.GetAddressOf());

		if (SUCCEEDED(compilationResult))
		{
			creator->CreateVertexShader(vsBlob->GetBufferPointer(),
				vsBlob->GetBufferSize(), nullptr, vertexShader.GetAddressOf());
		}
		else
		{
			PrintLabeledDebugString("Vertex Shader Errors:\n", (char*)errors->GetBufferPointer());
			abort();
			return nullptr;
		}

		return vsBlob;
	}

	Microsoft::WRL::ComPtr<ID3DBlob> CompilePixelShader(ID3D11Device* creator, UINT compilerFlags)
	{
		std::string pixelShaderSource = ReadFileIntoString("../Shaders/PixelShader.hlsl");

		Microsoft::WRL::ComPtr<ID3DBlob> psBlob, errors;

		HRESULT compilationResult =
			D3DCompile(pixelShaderSource.c_str(), pixelShaderSource.length(),
				nullptr, nullptr, nullptr, "main", "ps_4_0", compilerFlags, 0,
				psBlob.GetAddressOf(), errors.GetAddressOf());

		if (SUCCEEDED(compilationResult))
		{
			creator->CreatePixelShader(psBlob->GetBufferPointer(),
				psBlob->GetBufferSize(), nullptr, pixelShader.GetAddressOf());
		}
		else
		{
			PrintLabeledDebugString("Pixel Shader Errors:\n", (char*)errors->GetBufferPointer());
			abort();
			return nullptr;
		}

		return psBlob;
	}

	void CreateVertexInputLayout(ID3D11Device* creator, Microsoft::WRL::ComPtr<ID3DBlob>& vsBlob)
	{
		// TODO: Part 1E 
		D3D11_INPUT_ELEMENT_DESC attributes[3];

		attributes[0].SemanticName = "POSITION";
		attributes[0].SemanticIndex = 0;
		attributes[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		attributes[0].InputSlot = 0;
		attributes[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		attributes[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		attributes[0].InstanceDataStepRate = 0;

		attributes[1].SemanticName = "UVW";
		attributes[1].SemanticIndex = 0;
		attributes[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		attributes[1].InputSlot = 0;
		attributes[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		attributes[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		attributes[1].InstanceDataStepRate = 0;

		attributes[2].SemanticName = "NORMAL";
		attributes[2].SemanticIndex = 0;
		attributes[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		attributes[2].InputSlot = 0;
		attributes[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		attributes[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		attributes[2].InstanceDataStepRate = 0;

		HRESULT h = creator->CreateInputLayout(attributes, ARRAYSIZE(attributes),
			vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
			vertexFormat.GetAddressOf());
	}

private:
	struct PipelineHandles
	{
		ID3D11DeviceContext* context;
		ID3D11RenderTargetView* targetView;
		ID3D11DepthStencilView* depthStencil;
	};

	PipelineHandles GetCurrentPipelineHandles()
	{
		PipelineHandles retval;
		d3d.GetImmediateContext((void**)&retval.context);
		d3d.GetRenderTargetView((void**)&retval.targetView);
		d3d.GetDepthStencilView((void**)&retval.depthStencil);
		return retval;
	}

	void SetUpPipeline(PipelineHandles handles)
	{
		SetRenderTargets(handles);
		SetVertexBuffers(handles);
		SetShaders(handles);

		handles.context->IASetInputLayout(vertexFormat.Get());
		handles.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	void SetRenderTargets(PipelineHandles handles)
	{
		ID3D11RenderTargetView* const views[] = { handles.targetView };
		handles.context->OMSetRenderTargets(ARRAYSIZE(views), views, handles.depthStencil);
	}

	void SetVertexBuffers(PipelineHandles handles)
	{
		const UINT strides[] = { sizeof(H2B::VERTEX) }; 
		const UINT offsets[] = { 0 };
		ID3D11Buffer* const buffs[] = { vertexBuffer.Get() };
		handles.context->IASetVertexBuffers(0, ARRAYSIZE(buffs), buffs, strides, offsets);
	}

	void SetShaders(PipelineHandles handles)
	{
		handles.context->VSSetShader(vertexShader.Get(), nullptr, 0);
		handles.context->PSSetShader(pixelShader.Get(), nullptr, 0);
	}

	void ReleasePipelineHandles(PipelineHandles toRelease)
	{
		toRelease.depthStencil->Release();
		toRelease.targetView->Release();
		toRelease.context->Release();
	}
	//End Integrated Methods ///////////////////////////////////////

public:
	inline void SetName(std::string modelName) {
		name = modelName;
	}
	inline void SetWorldMatrix(GW::MATH::GMATRIXF worldMatrix) {
		world = worldMatrix;
	}
	bool LoadModelDataFromDisk(const char* h2bPath) {
		// if this succeeds "cpuModel" should now contain all the model's info
		return cpuModel.Parse(h2bPath);
	}
	bool UploadModelData2GPU(GW::GRAPHICS::GDirectX11Surface _d3d, GW::MATH::GMATRIXF worldM, GW::MATH::GMATRIXF vMatrix,
		GW::MATH::GMATRIXF pMatrix, GW::MATH::GVECTORF lightDir, GW::MATH::GVECTORF lightColor) {
		// TODO: Use chosen API to upload this model's graphics data to GPU

		//Takes in many variables to initialize theScene and theMesh, as well as initialize the Vertex,
		// Index, and Constant Buffers. Methods cascade into others to complete the initialization process and
		//  uploads to the GPU.
		d3d = _d3d;

		ID3D11Device* creator;
		_d3d.GetDevice((void**)&creator);

		InitializeVertexBuffer(creator);
		InitializeIndexBuffer(creator);
		
		theScene.projectionMatrix = pMatrix;
		theScene.viewMatrix = vMatrix;
		theScene._lightDirection = lightDir;
		theScene._lightColor = lightColor;
		theMesh.worldMatrix = worldM;
		CreateConstBuffer(creator, &theScene, sizeof(SceneData), sceneBuffer);
		CreateConstBuffer(creator, &theMesh, sizeof(MeshData), meshBuffer);
		InitializePipeline(creator);

		// free temporary handle
		creator->Release();
		return true;
	}

	bool DrawModel(GW::MATH::GMATRIXF view, Model e, GW::MATH::GMATRIXF currView) {
		// TODO: Use chosen API to setup the pipeline for this model and draw it

		PipelineHandles curHandles = GetCurrentPipelineHandles();
		SetUpPipeline(curHandles);
		

		ID3D11Buffer* pBuffs[] = { sceneBuffer.Get(), meshBuffer.Get() };
		curHandles.context->VSSetConstantBuffers(0, ARRAYSIZE(pBuffs), pBuffs);
		curHandles.context->PSSetConstantBuffers(0, ARRAYSIZE(pBuffs), pBuffs);
		curHandles.context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		
		D3D11_MAPPED_SUBRESOURCE subRes{};
		curHandles.context->Map(meshBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subRes);
		theMesh.worldMatrix = e.world;
		memcpy(subRes.pData, &theMesh, sizeof(theMesh));
		curHandles.context->Unmap(meshBuffer.Get(), 0);

		for (int i = 0; i < cpuModel.meshCount; i++)
		{
			curHandles.context->Map(meshBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subRes);
			theMesh.material = cpuModel.materials[i].attrib;
			memcpy(subRes.pData, &theMesh, sizeof(theMesh));
			curHandles.context->Unmap(meshBuffer.Get(), 0);

			curHandles.context->Map(sceneBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subRes);
			theScene.viewMatrix = view;
			theScene._cameraPos = currView.row4;
			memcpy(subRes.pData, &theScene, sizeof(theScene));
			curHandles.context->Unmap(sceneBuffer.Get(), 0);
			curHandles.context->DrawIndexed(cpuModel.meshes[i].drawInfo.indexCount
				,cpuModel.meshes[i].drawInfo.indexOffset, 0);

		}
		ReleasePipelineHandles(curHandles);
		return true;
	}
	bool FreeResources(/*specific API device for unloading*/) { 

		// TODO: Use chosen API to free all GPU resources used by this model
	}
};

// * NOTE: *
// Unlike the DOP version, this class was not designed to reuse data in anyway or process it efficiently.
// You can find ways to make it more efficient by sharing pointers to resources and sorting the models.
// However, this is tricky to implement and can be prone to errors. (OOP data isolation becomes an issue)
// A huge positive is that everything you want to draw is totally self contained and easy to see/find.
// This means updating matricies, adding new objects & removing old ones from the world is a breeze. 
// You can even easily load brand new models from disk at run-time without much trouble.
// The last major downside is trying to do things like dynamic lights, shadows and sorted transparency. 
// Effects like these expect your model set to be processed/traversed in unique ways which can be awkward.   

// class Level_Objects is simply a list of all the Models currently used by the level
class Level_Objects {

	// store all our models
	std::list<Model> allObjectsInLevel;
private:
	GW::MATH::GVECTORF const lightColor = { 0.9f, 0.9f, 1.0f, 1.0f }; // Lights
	GW::MATH::GVECTORF lightDirection = { 3.0f, -3.0, 2.0f, 1 };
	GW::MATH::GVECTORF sunAmbient = { 255 * 0.25f, 255 * 0.25f, 255 * 0.35f, 1.0f }; 

	// TODO: This could be a good spot for any global data like cameras or lights
public:

	// Imports the default level txt format and creates a Model from each .h2b
	bool LoadLevel(const char* gameLevelPath,
		const char* h2bFolderPath,
		GW::SYSTEM::GLog log) {

		// What this does:
		// Parse GameLevel.txt 
		// For each model found in the file...
			// Create a new Model class on the stack.
				// Read matrix transform and add to this model.
				// Load all CPU rendering data for this model from .h2b
			// Move the newly found Model to our list of total models for the level 

		log.LogCategorized("EVENT", "LOADING GAME LEVEL [OBJECT ORIENTED]");
		log.LogCategorized("MESSAGE", "Begin Reading Game Level Text File.");

		UnloadLevel();// clear previous level data if there is any
		GW::SYSTEM::GFile file;
		file.Create();
		if (-file.OpenTextRead(gameLevelPath)) {
			log.LogCategorized(
				"ERROR", (std::string("Game level not found: ") + gameLevelPath).c_str());
			return false;
		}
		char linebuffer[1024];
		while (+file.ReadLine(linebuffer, 1024, '\n'))
		{
			// having to have this is a bug, need to have Read/ReadLine return failure at EOF
			if (linebuffer[0] == '\0')
				break;
			if (std::strcmp(linebuffer, "MESH") == 0)
			{
				Model newModel;
				file.ReadLine(linebuffer, 1024, '\n');
				log.LogCategorized("INFO", (std::string("Model Detected: ") + linebuffer).c_str());
				// create the model file name from this (strip the .001)
				newModel.SetName(linebuffer);
				std::string modelFile = linebuffer;
				modelFile = modelFile.substr(0, modelFile.find_last_of("."));
				modelFile += ".h2b";

				// now read the transform data as we will need that regardless
				GW::MATH::GMATRIXF transform;
				for (int i = 0; i < 4; ++i) {
					file.ReadLine(linebuffer, 1024, '\n');
					// read floats
					std::sscanf(linebuffer + 13, "%f, %f, %f, %f",
						&transform.data[0 + i * 4], &transform.data[1 + i * 4],
						&transform.data[2 + i * 4], &transform.data[3 + i * 4]);
				}
				std::string loc = "Location: X ";
				loc += std::to_string(transform.row4.x) + " Y " +
					std::to_string(transform.row4.y) + " Z " + std::to_string(transform.row4.z);
				log.LogCategorized("INFO", loc.c_str());

				// Add new model to list of all Models
				log.LogCategorized("MESSAGE", "Begin Importing .H2B File Data.");
				modelFile = std::string(h2bFolderPath) + "/" + modelFile;
				newModel.SetWorldMatrix(transform);
				// If we find and load it add it to the level
				if (newModel.LoadModelDataFromDisk(modelFile.c_str())) {
					// add to our level objects, we use std::move since Model::cpuModel is not copy safe.
					allObjectsInLevel.push_back(std::move(newModel));
					log.LogCategorized("INFO", (std::string("H2B Imported: ") + modelFile).c_str());
				}
				else {
					// notify user that a model file is missing but continue loading
					log.LogCategorized("ERROR",
						(std::string("H2B Not Found: ") + modelFile).c_str());
					log.LogCategorized("WARNING", "Loading will continue but model(s) are missing.");
				}
				log.LogCategorized("MESSAGE", "Importing of .H2B File Data Complete.");
			}
		}
		log.LogCategorized("MESSAGE", "Game Level File Reading Complete.");
		// level loaded into CPU ram
		log.LogCategorized("EVENT", "GAME LEVEL WAS LOADED TO CPU [OBJECT ORIENTED]");
		return true;
	}
	// Upload the CPU level to GPU
	void UploadLevelToGPU(GW::GRAPHICS::GDirectX11Surface _d3d, GW::MATH::GMATRIXF worldM, GW::MATH::GMATRIXF vMatrix,
		GW::MATH::GMATRIXF pMatrix) {
		// iterate over each model and tell it to draw itself
		for (auto& e : allObjectsInLevel) {
			e.UploadModelData2GPU(_d3d, worldM, vMatrix, pMatrix, lightDirection, lightColor);
		}
	}
	// Draws all objects in the level
	void RenderLevel(GW::GRAPHICS::GDirectX11Surface _d3d, GW::MATH::GMATRIXF view, GW::MATH::GMATRIXF currView) {
		// iterate over each model and tell it to draw itself
		for (auto& e : allObjectsInLevel) {
			e.DrawModel(view, e, currView);
		}
	}
	// used to wipe CPU & GPU level data between levels
	void UnloadLevel() {
		allObjectsInLevel.clear();
	}
	// *THIS APPROACH COMBINES DATA & LOGIC* 
	// *WITH THIS APPROACH THE CURRENT RENDERER SHOULD BE JUST AN API MANAGER CLASS*
	// *ALL ACTUAL GPU LOADING AND RENDERING SHOULD BE HANDLED BY THE MODEL CLASS* 
	// For example: anything that is not a global API object should be encapsulated.

};

