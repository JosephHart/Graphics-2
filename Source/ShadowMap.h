#pragma once

#include <GUObject.h>
#include <Windows.h>

class ShadowMap
{
public:
	ShadowMap(ID3D11Device* device, UINT width, UINT height);
	~ShadowMap();

	void BindDSVAndSetNullRenderTarget(ID3D11DeviceContext* dc);

	ID3D11ShaderResourceView* mDepthMapSRV;
private:
	ShadowMap(const ShadowMap& rhs);
	ShadowMap& operator=(const ShadowMap& rhs);
	UINT mWidth;
	UINT mHeight;
	ID3D11DepthStencilView* mDepthMapDSV;
	D3D11_VIEWPORT mViewport;
};