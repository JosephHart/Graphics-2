
// Basic colour pixel shader


// input fragment - this is the per-fragment packet interpolated by the rasteriser stage
struct fragmentInputPacket {

	float4				colour		: COLOR;
	float2				texCoord	: TEXCOORD;


};


struct fragmentOutputPacket {

	float4				fragmentColour : SV_TARGET;
};

//Texture2DMS<float> inputTex : register(t0);
Texture2D inputTex : register(t0);
Texture2D RTTex: register(t1);
SamplerState linearSampler : register(s0);

fragmentOutputPacket main(fragmentInputPacket inputFragment) {

	fragmentOutputPacket outputFragment;
	outputFragment.fragmentColour = RTTex.Load(int4(inputFragment.texCoord.x * 600, inputFragment.texCoord.y * 600, 0, 0), 0);
	/*float d = RTTex.Load(int4(inputFragment.texCoord.x * 256, inputFragment.texCoord.y * 256, 0, 0), 0).r;
	outputFragment.fragmentColour = float4(d, d, d, 1);*/

	return outputFragment;
}
