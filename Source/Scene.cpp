
//
// Scene.cpp
//

#include <stdafx.h>
#include <string.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>
#include <Scene.h>
#include <DirectXMath.h>
#include <DXSystem.h>
#include <DirectXTK\DDSTextureLoader.h>
#include <DirectXTK\WICTextureLoader.h>
#include <CGDClock.h>
#include <Model.h>
#include <LookAtCamera.h>
#include <FirstPersonCamera.h>
#include <Material.h>
#include <Effect.h>
#include <Texture.h>
#include <VertexStructures.h>

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;


// Load the Compiled Shader Object (CSO) file 'filename' and return the bytecode in the blob object **bytecode.  This is used to create shader interfaces that require class linkage interfaces.
// Taken from DXShaderFactory by Paul Angel. This function has been included here for clarity.
uint32_t DXLoadCSO(const char *filename, char **bytecode)
{

	ifstream	*fp = nullptr;
	//char		*memBlock = nullptr;
	//bytecode = nullptr;
	uint32_t shaderBytes = -1;
	cout << "loading shader" << endl;

	try
	{
		// Validate parameters
		if (!filename)
			throw exception("loadCSO: Invalid parameters");

		// Open file
		fp = new ifstream(filename, ios::in | ios::binary);

		if (!fp->is_open())
			throw exception("loadCSO: Cannot open file");

		// Get file size
		fp->seekg(0, ios::end);
		shaderBytes = (uint32_t)fp->tellg();

		// Create blob object to store bytecode (exceptions propagate up if any occur)
		//memBlock = new DXBlob(size);
		cout << "allocating shader memory bytes = " << shaderBytes << endl;
		*bytecode = (char*)malloc(shaderBytes);
		// Read binary data into blob object
		fp->seekg(0, ios::beg);
		fp->read(*bytecode, shaderBytes);


		// Close file and release local resources
		fp->close();
		delete fp;

		// Return DXBlob - ownership implicity passed to caller
		//*bytecode = memBlock;
		cout << "Done: shader memory bytes = " << shaderBytes << endl;
	}
	catch (exception& e)
	{
		cout << e.what() << endl;

		// Cleanup local resources
		if (fp) {

			if (fp->is_open())
				fp->close();

			delete fp;
		}

		if (bytecode)
			delete bytecode;

		// Re-throw exception
		throw;
	}
	return shaderBytes;
}

//
// Private interface implementation
//

// Private constructor
Scene::Scene(const LONG _width, const LONG _height, const wchar_t* wndClassName, const wchar_t* wndTitle, int nCmdShow, HINSTANCE hInstance, WNDPROC WndProc)
{
	try
	{
		// 1. Register window class for main DirectX window
		WNDCLASSEX wcex;

		wcex.cbSize = sizeof(WNDCLASSEX);

		wcex.style = CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wcex.hCursor = LoadCursor(NULL, IDC_CROSS);
		wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = wndClassName;
		wcex.hIconSm = NULL;

		if (!RegisterClassEx(&wcex))
			throw exception("Cannot register window class for Scene HWND");


		// 2. Store instance handle in our global variable
		hInst = hInstance;


		// 3. Setup window rect and resize according to set styles
		RECT		windowRect;

		windowRect.left = 0;
		windowRect.right = _width;
		windowRect.top = 0;
		windowRect.bottom = _height;

		DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		DWORD dwStyle = WS_OVERLAPPEDWINDOW;

		AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

		// 4. Create and validate the main window handle
		wndHandle = CreateWindowEx(dwExStyle, wndClassName, wndTitle, dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 500, 500, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, NULL, NULL, hInst, this);

		if (!wndHandle)
			throw exception("Cannot create main window handle");

		ShowWindow(wndHandle, nCmdShow);
		UpdateWindow(wndHandle);
		SetFocus(wndHandle);


		// 5. Initialise render pipeline model (simply sets up an internal std::vector of pipeline objects)


		// 6. Create DirectX host environment (associated with main application wnd)
		dx = DXSystem::CreateDirectXSystem(wndHandle);

		if (!dx)
			throw exception("Cannot create Direct3D device and context model");

		// 7. Setup application-specific objects
		HRESULT hr = initialiseSceneResources();

		if (!SUCCEEDED(hr))
			throw exception("Cannot initalise scene resources");


		// 8. Create main clock / FPS timer (do this last with deferred start of 3 seconds so min FPS / SPF are not skewed by start-up events firing and taking CPU cycles).
		mainClock = CGDClock::CreateClock(string("mainClock"), 3.0f);

		if (!mainClock)
			throw exception("Cannot create main clock / timer");

	}
	catch (exception &e)
	{
		cout << e.what() << endl;

		// Re-throw exception
		throw;
	}
}


// Return TRUE if the window is in a minimised state, FALSE otherwise
BOOL Scene::isMinimised() {

	WINDOWPLACEMENT				wp;

	ZeroMemory(&wp, sizeof(WINDOWPLACEMENT));
	wp.length = sizeof(WINDOWPLACEMENT);

	return (GetWindowPlacement(wndHandle, &wp) != 0 && wp.showCmd == SW_SHOWMINIMIZED);
}



//
// Public interface implementation
//

// Factory method to create the main Scene instance (singleton)
Scene* Scene::CreateScene(const LONG _width, const LONG _height, const wchar_t* wndClassName, const wchar_t* wndTitle, int nCmdShow, HINSTANCE hInstance, WNDPROC WndProc) {

	static bool _scene_created = false;

	Scene *dxScene = nullptr;

	if (!_scene_created) {

		dxScene = new Scene(_width, _height, wndClassName, wndTitle, nCmdShow, hInstance, WndProc);

		if (dxScene)
			_scene_created = true;
	}

	return dxScene;
}


// Destructor
Scene::~Scene() {


	//free local resources

	if (cBufferExtSrc)
		_aligned_free(cBufferExtSrc);

	if (mainCamera)
		delete(mainCamera);


	if (brickTexture)
		delete(brickTexture);

	if (perPixelLightingEffect)
		delete(perPixelLightingEffect);

	//Clean Up- release local interfaces

	if (mainClock)
		mainClock->release();

	if (cBufferBridge)
		cBufferBridge->Release();

	if (cBufferShadow)
		cBufferShadow->Release();

	//	if (bridge)
	//		bridge->release();

	if (dx) {

		dx->release();
		dx = nullptr;
	}

	if (wndHandle)
		DestroyWindow(wndHandle);
}


// Decouple the encapsulated HWND and call DestoryWindow on the HWND
void Scene::destoryWindow() {

	if (wndHandle != NULL) {

		HWND hWnd = wndHandle;

		wndHandle = NULL;
		DestroyWindow(hWnd);
	}
}


// Resize swap chain buffers and update pipeline viewport configurations in response to a window resize event
HRESULT Scene::resizeResources() {

	if (dx && !isMinimised()) {

		// Only process resize if the DXSystem *dx exists (on initial resize window creation this will not be the case so this branch is ignored)
		HRESULT hr = dx->resizeSwapChainBuffers(wndHandle);
		rebuildViewport(mainCamera);
		RECT clientRect;
		GetClientRect(wndHandle, &clientRect);

		renderScene();
	}

	return S_OK;
}


// Helper function to call updateScene followed by renderScene
HRESULT Scene::updateAndRenderScene() {
	ID3D11DeviceContext *context = dx->getDeviceContext();
	HRESULT hr = updateScene(context);

	if (SUCCEEDED(hr))
		hr = renderScene();

	return hr;
}


// Clock handling methods

void Scene::startClock() {

	mainClock->start();
}

void Scene::stopClock() {

	mainClock->stop();
}

void Scene::reportTimingData() {

	cout << "Actual time elapsed = " << mainClock->actualTimeElapsed() << endl;
	cout << "Game time elapsed = " << mainClock->gameTimeElapsed() << endl << endl;
	mainClock->reportTimingData();
}



//
// Event handling methods
//
// Process mouse move with the left button held down
void Scene::handleMouseLDrag(const POINT &disp) {
	//LookAtCamera
	mainCamera->rotateElevation((float)-disp.y * 0.01f);
	mainCamera->rotateOnYAxis((float)-disp.x * 0.01f);

	//FirstPersonCamera
	//mainCamera->elevate((float)-disp.y * 0.01f);
	//mainCamera->turn((float)-disp.x * 0.01f);
}

// Process mouse wheel movement
void Scene::handleMouseWheel(const short zDelta) {

	//LookAtCamera
	if (zDelta < 0)
		mainCamera->zoomCamera(1.2f);
	else if (zDelta>0)
		mainCamera->zoomCamera(0.9f);
	//FirstPersonCamera
	//mainCamera->move(zDelta*0.01);
}


// Process key down event.  keyCode indicates the key pressed while extKeyFlags indicates the extended key status at the time of the key down event (see http://msdn.microsoft.com/en-gb/library/windows/desktop/ms646280%28v=vs.85%29.aspx).
void Scene::handleKeyDown(const WPARAM keyCode, const LPARAM extKeyFlags)
{

}


// Process key up event.  keyCode indicates the key released while extKeyFlags indicates the extended key status at the time of the key up event (see http://msdn.microsoft.com/en-us/library/windows/desktop/ms646281%28v=vs.85%29.aspx).
void Scene::handleKeyUp(const WPARAM keyCode, const LPARAM extKeyFlags) {

	// Add key up handler here...
}



//
// Methods to handle initialisation, update and rendering of the scene
//

HRESULT Scene::rebuildViewport(Camera *camera){
	// Binds the render target view and depth/stencil view to the pipeline.
	// Sets up viewport for the main window (wndHandle) 
	// Called at initialisation or in response to window resize

	ID3D11DeviceContext *context = dx->getDeviceContext();

	if (!context)
		return E_FAIL;

	// Bind the render target view and depth/stencil view to the pipeline.
	ID3D11RenderTargetView* renderTargetView = dx->getBackBufferRTV();
	context->OMSetRenderTargets(1, &renderTargetView, dx->getDepthStencil());
	// Setup viewport for the main window (wndHandle)
	RECT clientRect;
	GetClientRect(wndHandle, &clientRect);

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<FLOAT>(clientRect.right - clientRect.left);
	viewport.Height = static_cast<FLOAT>(clientRect.bottom - clientRect.top);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;
	//Set Viewport
	context->RSSetViewports(1, &viewport);

	// Compute the projection matrix.

	camera->setProjMatrix(XMMatrixPerspectiveFovLH(0.25f*3.14, viewport.Width / viewport.Height, 1.0f, 1000.0f));
	return S_OK;
}





HRESULT Scene::LoadShader(ID3D11Device *device, const char *filename, char **PSBytecode, ID3D11PixelShader **pixelShader){

	char *PSBytecodeLocal;

	//Load the compiled shader byte code.
	uint32_t shaderBytes = DXLoadCSO(filename, &PSBytecodeLocal);

	PSBytecode = &PSBytecodeLocal;
	cout << "Done: PShader memory bytes = " << shaderBytes << endl;
	// Create shader object
	HRESULT hr = device->CreatePixelShader(PSBytecodeLocal, shaderBytes, NULL, pixelShader);

	if (!SUCCEEDED(hr))
		throw std::exception("Cannot create PixelShader interface");
	return hr;
}


uint32_t Scene::LoadShader(ID3D11Device *device, const char *filename, char **VSBytecode, ID3D11VertexShader **vertexShader){

	char *VSBytecodeLocal;

	//Load the compiled shader byte code.
	uint32_t shaderBytes = DXLoadCSO(filename, &VSBytecodeLocal);

	cout << "Done: VShader memory bytes = " << shaderBytes << endl;

	*VSBytecode = VSBytecodeLocal;
	cout << "Done: VShader writting = " << shaderBytes << endl;
	HRESULT hr = device->CreateVertexShader(VSBytecodeLocal, shaderBytes, NULL, vertexShader);
	cout << "Done: VShader return = " << hr << endl;
	if (!SUCCEEDED(hr))
		throw std::exception("Cannot create VertexShader interface");
	return shaderBytes;
}


// Main resource setup for the application.  These are setup around a given Direct3D device.
HRESULT Scene::initialiseSceneResources() {
	//ID3D11DeviceContext *context = dx->getDeviceContext();
	ID3D11Device *device = dx->getDevice();
	if (!device)
		return E_FAIL;

	//
	// Setup main pipeline objects
	//

	// Setup objects for fixed function pipeline stages
	// Rasterizer Stage
	// Bind the render target view and depth/stencil view to the pipeline
	// and sets up viewport for the main window (wndHandle) 


	// Create main camera
	//
	mainCamera = new LookAtCamera();
	mainCamera->setPos(XMVectorSet(25, 2, -14.5, 1));

	rebuildViewport(mainCamera);

	ID3D11DeviceContext *context = dx->getDeviceContext();
	if (!context)
		return E_FAIL;

	static const int S_MAP_SIZE = 5120;
	shadowMap = new ShadowMap(device, S_MAP_SIZE, S_MAP_SIZE);

	// Setup objects for the programmable (shader) stages of the pipeline

	perPixelLightingEffect = new Effect(device, "Shaders\\cso\\per_pixel_lighting_vs.cso", "Shaders\\cso\\per_pixel_lighting_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));

	skyBoxEffect = new Effect(device, "Shaders\\cso\\sky_box_vs.cso", "Shaders\\cso\\sky_box_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));
	basicEffect = new Effect(device, "Shaders\\cso\\basic_texture_vs.cso", "Shaders\\cso\\basic_texture_ps.cso", basicVertexDesc, ARRAYSIZE(basicVertexDesc));
	particleEffect = new Effect(device, "Shaders\\cso\\fire_vs.cso", "Shaders\\cso\\fire_ps.cso", "Shaders\\cso\\fire_gs.cso", particleVertexDesc, ARRAYSIZE(particleVertexDesc));
	particleUpdateEffect = new Effect(device, "Shaders\\cso\\fire_vs.cso", "Shaders\\cso\\fire_ps.cso", particleVertexDesc, ARRAYSIZE(particleVertexDesc));


	char *tmpShaderBytecode = nullptr;
	ID3D11GeometryShader* tmpGS = nullptr;
	particleUpdateEffect->CreateGeometryShaderWithSO(device, "Shaders\\cso\\fire_update_gs.cso", &tmpShaderBytecode, &tmpGS);

	// get current blendState and blend description of particleEffect (alpha blending off by default)
	ID3D11BlendState *partBS = particleEffect->getBlendState();
	D3D11_BLEND_DESC partBD;
	partBS->GetDesc(&partBD);

	// Modify blend description - alpha blending on
	partBD.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	partBD.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	partBD.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	partBD.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
	partBD.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	partBD.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

	// Create new blendState
	partBS->Release();
	device->CreateBlendState(&partBD, &partBS);
	particleEffect->setBlendState(partBS);


	// get current depthStencil State and depthStencil description of particleEffect (depth read and write by default)
	ID3D11DepthStencilState *partDSS = particleEffect->getDepthStencilState();
	D3D11_DEPTH_STENCIL_DESC partDSD;
	partDSS->GetDesc(&partDSD);

	//Disable Depth Writing for particles
	partDSD.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	// Create custom fire depth-stencil state object
	partDSS->Release();
	device->CreateDepthStencilState(&partDSD, &partDSS);
	particleEffect->setDepthStencilState(partDSS);
	//perPixelLightingEffect->setDepthStencilState(partDSS);


	partDSD.DepthEnable = FALSE;
	partDSD.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	partDSD.DepthFunc = D3D11_COMPARISON_ALWAYS;
	partDSD.StencilEnable = FALSE;
	partDSD.StencilReadMask = 0xFF;
	partDSD.StencilWriteMask = 0xFF;

	device->CreateDepthStencilState(&partDSD, &partDSS);
	particleUpdateEffect->setDepthStencilState(partDSS);
	particleEffect->setDepthStencilState(partDSS);

	refMapEffect = new Effect(device, "Shaders\\cso\\reflection_map_vs.cso", "Shaders\\cso\\reflection_map_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));
	shadowEffect = new Effect(device, "Shaders\\cso\\shadow_vs.cso", "Shaders\\cso\\shadow_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));
	//shadowMapEffect = new Effect(device, "Shaders\\cso\\shadow_map_vs.cso", "Shaders\\cso\\shadow_map_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));
	buildShadowMapEffect = new Effect(device, "Shaders\\cso\\build_shadow_map_vs.cso", "Shaders\\cso\\build_shadow_map_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));

	// Setup CBuffer
	cBufferExtSrc = (CBufferExt*)_aligned_malloc(sizeof(CBufferExt), 16);
	// Initialise CBuffer
	cBufferExtSrc->worldMatrix = XMMatrixIdentity();
	cBufferExtSrc->worldITMatrix = XMMatrixIdentity();
	cBufferExtSrc->WVPMatrix = mainCamera->getViewMatrix()*mainCamera->getProjMatrix();
	cBufferExtSrc->lightVec = lightVec; // Positional light
	cBufferExtSrc->lightAmbient = XMFLOAT4(0.3, 0.3, 0.3, 1.0);
	cBufferExtSrc->lightDiffuse = XMFLOAT4(0.8, 0.8, 0.8, 1.0);
	cBufferExtSrc->lightSpecular = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
	cBufferExtSrc->windDir = XMFLOAT4(0.0, 0.0, -1.0, 1.0);

	lightCamera = new LookAtCamera();
	//lightCamera = new FirstPersonCamera(XMLoadFloat4(&lightVec), XMVectorSet(0, 1, 0, 0), XMLoadFloat4(&reverseLightVec));
	lightCamera->setPos(XMLoadFloat4(&lightVec) * 0.5);
	lightCamera->setProjMatrix(XMMatrixPerspectiveFovLH(0.25f * 3.14, 1, 1.0f, 1000.0f));

	cBufferExtSrc->shadowMatrix = lightCamera->getViewMatrix() * lightCamera->getProjMatrix();

	XMStoreFloat4(&cBufferExtSrc->eyePos, mainCamera->getPos());


	//planar shadows
	//vector represents the normal to the shadow plane - in this case, shadow plane is the ground, and the normal is facing directly upwards
	//XMVECTOR shadowPlane = XMVectorSet(0, 1, 0, 0);

	//planar shadow setup
	//XMVECTOR lightPos = XMVectorSet(-250.0, 130.0, 145.0, 1.0);
	//shadowMatrix = XMMatrixShadow(shadowPlane, lightPos);
	//planar shadow setup

	D3D11_BUFFER_DESC cbufferDesc;
	D3D11_SUBRESOURCE_DATA cbufferInitData;
	ZeroMemory(&cbufferDesc, sizeof(D3D11_BUFFER_DESC));
	ZeroMemory(&cbufferInitData, sizeof(D3D11_SUBRESOURCE_DATA));
	cbufferDesc.ByteWidth = sizeof(CBufferExt);
	cbufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbufferInitData.pSysMem = cBufferExtSrc;


	//Create bridge CBuffer
	HRESULT hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData, &cBufferBridge);
	hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData, &cBufferShadow);
	hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData, &cBufferSkyBox);
	hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData, &cBufferSphere);
	hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData, &cBufferParticles);
	hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData, &cBufferFloor);
	hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData, &cBufferDropship);
	for (int i = 0; i < 40; i++)
	{
		hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData, &cBufferBush[i]);
	}

	// Setup example objects
	//


	mattWhite.setSpecular(XMCOLOR(0, 0, 0, 0));
	glossWhite.setSpecular(XMCOLOR(1, 1, 1, 1));


	brickTexture = new Texture(device, L"Resources\\Textures\\brick_DIFFUSE.jpg");
	envMapTexture = new Texture(device, L"Resources\\Textures\\grassenvmap1024.dds");
	rustDiffTexture = new Texture(device, L"Resources\\Textures\\rustDiff.jpg");
	rustSpecTexture = new Texture(device, L"Resources\\Textures\\rustSpec.jpg");
	fireTexture = new Texture(device, L"Resources\\Textures\\Fire.tif");
	dropshipTex = new Texture(device, L"Resources\\Textures\\dropship_texture.bmp");
	grassTex = new Texture(device, L"Resources\\Textures\\grass.png");
	furTex = new Texture(device, L"Resources\\Textures\\fur.png");

	//ID3D11ShaderResourceView *sphereTextureArray[] = { rustDiffTexture->SRV, brickTexture->SRV, rustSpecTexture->SRV };

	//load bridge
	bridge = new Model(device, perPixelLightingEffect, wstring(L"Resources\\Models\\bridge.3ds"), brickTexture->SRV, &mattWhite);
	sphere = new Model(device, perPixelLightingEffect, wstring(L"Resources\\Models\\sphere.3ds"), furTex->SRV, &mattWhite);
	dropship = new Model(device, perPixelLightingEffect, wstring(L"Resources\\Models\\dropship.gsf"), dropshipTex->SRV, &glossWhite);
	bush = new Model(device, perPixelLightingEffect, wstring(L"Resources\\Models\\Bush.3ds"), grassTex->SRV, &mattWhite);
	particles = new GPUParticles(device, particleEffect, particleUpdateEffect, fireTexture->SRV, &mattWhite);
	box = new Box(device, skyBoxEffect, envMapTexture->SRV);
	triangle = new Quad(device, basicEffect->getVSInputLayout());
	shadowMapTexturedQuad = new Quad(device, basicEffect->getVSInputLayout());
	floor = new Box(device, perPixelLightingEffect, grassTex->SRV);

	//bush positions
	for (int i = 0; i < 40; i++)
	{
		if (i < 10)
		{
			bushPosX[i] = 50;
			bushPosZ[i] = 50 - (i * 10);
		}
		else if (i < 20)
		{
			bushPosX[i] = 50 - ((i - 10) * 10);
			bushPosZ[i] = -50;
		}
		else if (i < 30)
		{
			bushPosX[i] = -50;
			bushPosZ[i] = -50 + ((i - 20) * 10);
		}
		else if (i < 40)
		{
			bushPosX[i] = -50 + ((i - 30) * 10);
			bushPosZ[i] = 50;
		}
	}

	return S_OK;
}

HRESULT Scene::updateScene(ID3D11DeviceContext *context) {

	static gu_seconds lastFrameTime = 0;
	mainClock->tick();
	gu_seconds tDelta = mainClock->gameTimeElapsed();

	lastFrameTime = mainClock->gameTimeElapsed();
	cBufferExtSrc->Timer = (FLOAT)mainClock->gameTimeElapsed();

	XMStoreFloat4(&cBufferExtSrc->eyePos, mainCamera->getPos());

	//cBufferExtSrc->shadowMatrix = S;

	cBufferExtSrc->worldMatrix = XMMatrixScaling(0.05, 0.05, 0.05)*XMMatrixTranslation(4.5, -0.0, 4);
	cBufferExtSrc->worldITMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, cBufferExtSrc->worldMatrix));
	cBufferExtSrc->WVPMatrix = cBufferExtSrc->worldMatrix*mainCamera->getViewMatrix()*mainCamera->getProjMatrix();
	mapCbuffer(cBufferExtSrc, cBufferBridge);

	cBufferExtSrc->worldMatrix = XMMatrixScaling(50, 0.5, 50)*XMMatrixTranslation(0, -0.8, 0);
	cBufferExtSrc->worldITMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, cBufferExtSrc->worldMatrix));
	cBufferExtSrc->WVPMatrix = cBufferExtSrc->worldMatrix*mainCamera->getViewMatrix()*mainCamera->getProjMatrix();
	mapCbuffer(cBufferExtSrc, cBufferFloor);

	cBufferExtSrc->worldMatrix = XMMatrixScaling(1000.0, 1000, 1000)*XMMatrixTranslation(0, 0, 0);
	cBufferExtSrc->worldITMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, cBufferExtSrc->worldMatrix));
	cBufferExtSrc->WVPMatrix = cBufferExtSrc->worldMatrix*mainCamera->getViewMatrix()*mainCamera->getProjMatrix();
	mapCbuffer(cBufferExtSrc, cBufferSkyBox);

	cBufferExtSrc->worldMatrix = XMMatrixScaling(1, 1, 1) * XMMatrixTranslation(0, 4 + sin(tDelta), 0);
	cBufferExtSrc->worldITMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, cBufferExtSrc->worldMatrix));
	cBufferExtSrc->WVPMatrix = cBufferExtSrc->worldMatrix*mainCamera->getViewMatrix()*mainCamera->getProjMatrix();
	mapCbuffer(cBufferExtSrc, cBufferSphere);

	cBufferExtSrc->worldMatrix = XMMatrixScaling(2, 2, 2)*XMMatrixTranslation(20, 10, 0)*XMMatrixRotationY(-tDelta / 4);
	cBufferExtSrc->worldITMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, cBufferExtSrc->worldMatrix));
	cBufferExtSrc->WVPMatrix = cBufferExtSrc->worldMatrix*mainCamera->getViewMatrix()*mainCamera->getProjMatrix();
	mapCbuffer(cBufferExtSrc, cBufferDropship);

	for (int i = 0; i < 40; i++)
	{
		cBufferExtSrc->worldMatrix = XMMatrixScaling(0.5, 0.5, 0.5)*XMMatrixTranslation(bushPosX[i] + 50, 0, bushPosZ[i]);
		cBufferExtSrc->worldITMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, cBufferExtSrc->worldMatrix));
		cBufferExtSrc->WVPMatrix = cBufferExtSrc->worldMatrix*mainCamera->getViewMatrix()*mainCamera->getProjMatrix();
		mapCbuffer(cBufferExtSrc, cBufferBush[i]);
	}

	return S_OK;
}

HRESULT Scene::updateSceneShadow(ID3D11DeviceContext* context)
{
	buildShadowTransform();
	
	static gu_seconds lastFrameTime = 0;
	mainClock->tick();
	gu_seconds tDelta = mainClock->gameTimeElapsed();

	lastFrameTime = mainClock->gameTimeElapsed();
	cBufferExtSrc->Timer = (FLOAT)mainClock->gameTimeElapsed();

	XMStoreFloat4(&cBufferExtSrc->eyePos, mainCamera->getPos());

	cBufferExtSrc->shadowMatrix = S;

	cBufferExtSrc->worldMatrix = XMMatrixScaling(0.05, 0.05, 0.05)*XMMatrixTranslation(4.5, -0.0, 4);
	cBufferExtSrc->worldITMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, cBufferExtSrc->worldMatrix));
	cBufferExtSrc->WVPMatrix = cBufferExtSrc->worldMatrix*lightCamera->getViewMatrix()*lightCamera->getProjMatrix();
	mapCbuffer(cBufferExtSrc, cBufferBridge);

	cBufferExtSrc->worldMatrix = XMMatrixScaling(50, 0.5, 50)*XMMatrixTranslation(0, -0.8, 0);
	cBufferExtSrc->worldITMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, cBufferExtSrc->worldMatrix));
	cBufferExtSrc->WVPMatrix = cBufferExtSrc->worldMatrix*lightCamera->getViewMatrix()*lightCamera->getProjMatrix();
	mapCbuffer(cBufferExtSrc, cBufferFloor);

	cBufferExtSrc->worldMatrix = XMMatrixScaling(1000.0, 1000, 1000)*XMMatrixTranslation(0, 0, 0);
	cBufferExtSrc->worldITMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, cBufferExtSrc->worldMatrix));
	cBufferExtSrc->WVPMatrix = cBufferExtSrc->worldMatrix*lightCamera->getViewMatrix()*lightCamera->getProjMatrix();
	mapCbuffer(cBufferExtSrc, cBufferSkyBox);

	cBufferExtSrc->worldMatrix = XMMatrixScaling(1, 1, 1) * XMMatrixTranslation(0, 4 + sin(tDelta), 0);
	cBufferExtSrc->worldITMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, cBufferExtSrc->worldMatrix));
	cBufferExtSrc->WVPMatrix = cBufferExtSrc->worldMatrix*lightCamera->getViewMatrix()*lightCamera->getProjMatrix();
	mapCbuffer(cBufferExtSrc, cBufferSphere);

	cBufferExtSrc->worldMatrix = XMMatrixScaling(2, 2, 2)*XMMatrixTranslation(20, 10, 0)*XMMatrixRotationY(-tDelta / 4);
	cBufferExtSrc->worldITMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, cBufferExtSrc->worldMatrix));
	cBufferExtSrc->WVPMatrix = cBufferExtSrc->worldMatrix*lightCamera->getViewMatrix()*lightCamera->getProjMatrix();
	mapCbuffer(cBufferExtSrc, cBufferDropship);

	for (int i = 0; i < 40; i++)
	{
		cBufferExtSrc->worldMatrix = XMMatrixScaling(0.5, 0.5, 0.5)*XMMatrixTranslation(bushPosX[i] + 50, 0, bushPosZ[i]);
		cBufferExtSrc->worldITMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, cBufferExtSrc->worldMatrix));
		cBufferExtSrc->WVPMatrix = cBufferExtSrc->worldMatrix*lightCamera->getViewMatrix()*lightCamera->getProjMatrix();
		mapCbuffer(cBufferExtSrc, cBufferBush[i]);
	}

	return S_OK;
}

HRESULT Scene::buildShadowTransform()
{
	XMMATRIX V = lightCamera->getViewMatrix();
	XMMATRIX P = lightCamera->getProjMatrix();

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	S = V*P*T;

	return S_OK;
}

// Helper function to copy cbuffer data from cpu to gpu
HRESULT Scene::mapCbuffer(void *cBufferExtSrcL, ID3D11Buffer *cBufferExtL)
{
	ID3D11DeviceContext *context = dx->getDeviceContext();
	// Map cBuffer
	D3D11_MAPPED_SUBRESOURCE res;
	HRESULT hr = context->Map(cBufferExtL, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);

	if (SUCCEEDED(hr)) {
		memcpy(res.pData, cBufferExtSrcL, sizeof(CBufferExt));
		context->Unmap(cBufferExtL, 0);
	}
	return hr;
}


// Render scene
HRESULT Scene::renderScene()
{
	ID3D11DeviceContext *context = dx->getDeviceContext();

	// Validate window and D3D context
	if (isMinimised() || !context)	return E_FAIL;

	// Clear the screen
	static const FLOAT clearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

	//Get render target
	context->OMGetRenderTargets(1, &defaultRenderTargetView, &defaultDepthStencilView);


	//1st pass
	//Set a NULL render target and bind correct depth stencil
	shadowMap->BindDSVAndSetNullRenderTarget(context);

	//Update light camera
	updateSceneShadow(context);

	//Render objects
	renderObjects(context);

	// Restore the back and depth buffer
	context->OMSetRenderTargets(1, &defaultRenderTargetView, defaultDepthStencilView);



	//2nd pass
	//clear render/depth views
	context->ClearRenderTargetView(dx->getBackBufferRTV(), clearColor);
	context->ClearDepthStencilView(dx->getDepthStencil(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	//Update scene for main camera
	updateScene(context);

	//Reset viewport
	context->RSSetViewports(1, &viewport);

	//Set SRV for shadow map texture in shader
	context->PSSetShaderResources(1, 1, &shadowMap->mDepthMapSRV);

	//Draw objects normally
	renderObjects(context);

	//set to screen
	HRESULT hr = dx->presentBackBuffer();

	return S_OK;
}

HRESULT Scene::renderObjects(ID3D11DeviceContext* context)
{
	if (box) {
		// Apply the box cBuffer.
		context->VSSetConstantBuffers(0, 1, &cBufferSkyBox);
		context->PSSetConstantBuffers(0, 1, &cBufferSkyBox);
		// Render
		box->render(context);
	}

	if (bridge) {

		// Setup pipeline for effect
		// Apply the bridge cBuffer.
		context->VSSetConstantBuffers(0, 1, &cBufferBridge);
		context->PSSetConstantBuffers(0, 1, &cBufferBridge);
		// Render
		bridge->render(context);


		//Planar shadows
		/*shadowEffect->bindPipeline(context);

		context->VSSetConstantBuffers(0, 1, &cBufferShadow);
		context->PSSetConstantBuffers(0, 1, &cBufferShadow);

		bridge->renderSimp(context);*/
	}
	if (floor)
	{
		context->VSSetConstantBuffers(0, 1, &cBufferFloor);
		context->PSSetConstantBuffers(0, 1, &cBufferFloor);
		// Render
		floor->render(context);
	}

	if (dropship) {
		// Apply the box cBuffer.
		context->VSSetConstantBuffers(0, 1, &cBufferDropship);
		context->PSSetConstantBuffers(0, 1, &cBufferDropship);
		// Render
		dropship->render(context);
	}


	if (sphere) {
		// Apply the sphere cBuffer.
		context->VSSetConstantBuffers(0, 1, &cBufferSphere);
		context->PSSetConstantBuffers(0, 1, &cBufferSphere);
		// Render
		sphere->render(context);
	}

	for (int i = 0; i < 40; i++)
	{
		if (bush) {
			// Apply the box cBuffer.
			context->VSSetConstantBuffers(0, 1, &cBufferBush[i]);
			context->PSSetConstantBuffers(0, 1, &cBufferBush[i]);
			// Render
			bush->render(context);
		}
	}

	return S_OK;
}
