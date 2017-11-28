
//
// Scene.h

#pragma once

#include <GUObject.h>
#include <Windows.h>
#include <Box.h>
#include <Triangle.h>
#include <Quad.h>
#include <GPUParticles.h>
#include <CBufferStructures.h>
#include <Material.h>
#include "ShadowMap.h"

class DXSystem;
class CGDClock;
class Model;
class Camera;
class LookAtCamera;
//class FirstPersonCamera;
class Texture;
class Effect;
class ShadowMap;


class Scene : public GUObject {

	HINSTANCE								hInst = NULL;
	HWND									wndHandle = NULL;

	// Strong reference to associated Direct3D device and rendering context.
	DXSystem								*dx = nullptr;

	D3D11_VIEWPORT							viewport;

	Material								mattWhite;
	Material								glossWhite;

	ShadowMap								*mSmap;

	// Direct3D scene textures and resource views

	//Effects
	Effect									*defaultEffect;
	Effect									*perPixelLightingEffect;
	Effect									*skyBoxEffect;
	Effect									*basicEffect;
	Effect									*refMapEffect;
	Effect									*particleEffect;
	Effect									*particleUpdateEffect;
	Effect									*planarShadowEffect;
	ID3D11Buffer							*cBufferSkyBox = nullptr;
	ID3D11Buffer							*cBufferBridge = nullptr;
	ID3D11Buffer							*cBufferFloor = nullptr;
	ID3D11Buffer							*cBufferSphere = nullptr;
	ID3D11Buffer							*cBufferParticles = nullptr;
	ID3D11Buffer							*cBufferShadow = nullptr;
	CBufferExt								*cBufferExtSrc = nullptr;

	XMMATRIX								shadowMatrix;

	Texture									*brickTexture = nullptr;
	Texture									*rustDiffTexture = nullptr;
	Texture									*rustSpecTexture = nullptr;
	Texture									*envMapTexture = nullptr;
	Texture									*fireTexture = nullptr;

	// Tutorial 04
	ID3D11ShaderResourceView				*renderTargetSRV;
	ID3D11RenderTargetView					*renderTargetRTV;
	
	//Models
	Model									*bridge = nullptr;
	Box										*box = nullptr;
	Box										*floor = nullptr;
	Model									*sphere = nullptr;
	Quad									*triangle = nullptr;
	GPUParticles							*particles = nullptr;
	// Main FPS clock
	CGDClock								*mainClock = nullptr;

	//Camera
	//FirstPersonCamera						*mainCamera = nullptr;
	LookAtCamera							*mainCamera = nullptr;

	float mLightRotationAngle;

	DirectX::XMFLOAT4X4 mLightView;
	DirectX::XMFLOAT4X4 mLightProj;
	DirectX::XMFLOAT4X4 mShadowTransform;
	
	//
	// Private interface
	//

	// Private constructor
	Scene(const LONG _width, const LONG _height, const wchar_t* wndClassName, const wchar_t* wndTitle, int nCmdShow, HINSTANCE hInstance, WNDPROC WndProc);

	// Return TRUE if the window is in a minimised state, FALSE otherwise
	BOOL isMinimised();

public:

	//
	// Public interface
	//
	// Factory method to create the main Scene instance (singleton)
	static Scene* CreateScene(const LONG _width, const LONG _height, const wchar_t* wndClassName, const wchar_t* wndTitle, int nCmdShow, HINSTANCE hInstance, WNDPROC WndProc);

	// Destructor
	~Scene();

	// Decouple the encapsulated HWND and call DestoryWindow on the HWND
	void destoryWindow();

	// Resize swap chain buffers and update pipeline viewport configurations in response to a window resize event
	HRESULT resizeResources();

	// Helper function to call updateScene followed by renderScene
	HRESULT updateAndRenderScene();
	HRESULT mapCbuffer(void *cBufferExtSrcL, ID3D11Buffer *cBufferExtL);
	// Clock handling methods
	void startClock();
	void stopClock();
	void reportTimingData();


	//
	// Event handling methods
	//

	// Process mouse move with the left button held down
	void handleMouseLDrag(const POINT &disp);

	// Process mouse wheel movement
	void handleMouseWheel(const short zDelta);

	// Process key down event.  keyCode indicates the key pressed while extKeyFlags indicates the extended key status at the time of the key down event (see http://msdn.microsoft.com/en-gb/library/windows/desktop/ms646280%28v=vs.85%29.aspx).
	void handleKeyDown(const WPARAM keyCode, const LPARAM extKeyFlags);
	
	// Process key up event.  keyCode indicates the key released while extKeyFlags indicates the extended key status at the time of the key up event (see http://msdn.microsoft.com/en-us/library/windows/desktop/ms646281%28v=vs.85%29.aspx).
	void handleKeyUp(const WPARAM keyCode, const LPARAM extKeyFlags);

	//
	// Methods to handle initialisation, update and rendering of the scene
	//
	HRESULT rebuildViewport(Camera *camera);
	HRESULT initDefaultPipeline();
	HRESULT bindDefaultPipeline();
	HRESULT LoadShader(ID3D11Device *device, const char *filename, char **PSBytecode, ID3D11PixelShader **pixelShader);
	uint32_t LoadShader(ID3D11Device *device, const char *filename, char **VSBytecode, ID3D11VertexShader **vertexShader);
	HRESULT initialiseSceneResources();
	HRESULT updateScene(ID3D11DeviceContext *context);
	HRESULT renderScene();
	HRESULT renderSceneElements(ID3D11DeviceContext *context);
	void BuildShadowTransform();

	void DrawScene(ID3D11DeviceContext *context);

};


struct BoundingSphere
{
	BoundingSphere() : Center(0.0f, 0.0f, 0.0f), Radius(0.0f) {}
	XMFLOAT3 Center;
	float Radius;
};