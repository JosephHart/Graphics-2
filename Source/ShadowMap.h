#include "D3D11.h"

#pragma once

class ShadowMap
{
public:
	ShadowMap(ID3D11Device* device, UINT width, UINT height);
	~ShadowMap();
	ID3D11ShaderResourceView* DepthMapSRV();
	ID3D11DepthStencilView* GetDepthMapDSV();
	D3D11_VIEWPORT GetViewport();
private:
	ShadowMap(const ShadowMap& rhs);
	ShadowMap& operator=(const ShadowMap& rhs);
private:
	UINT mWidth;
	UINT mHeight;
	ID3D11ShaderResourceView* mDepthMapSRV;
	ID3D11DepthStencilView* mDepthMapDSV;
	D3D11_VIEWPORT mViewport;
};