#pragma once
#include "DXVertexParticle.h"
#include "Particles.h"



class GPUParticles : public Particles{

	ID3D11Buffer					*vertexBuffer2 = nullptr;
	Effect							*updateEffect= nullptr;

public:

	GPUParticles(ID3D11Device *device, Effect *_effect, Effect *_updateEffect, ID3D11ShaderResourceView *tex_view, Material *_material);
	void render(ID3D11DeviceContext *context);
	void update(ID3D11DeviceContext *context);
};