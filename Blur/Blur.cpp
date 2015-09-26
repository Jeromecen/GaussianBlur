// 项目目标：
// 1.提高了一个完整的fx框架编写Shader,解决掉了编写过程中遇到的所有问题。验证了在
// FX框架可以不用GetParameterByName获取句柄的，直接用字符串作为句柄即可。
// 2.对高斯模糊图片提供了一个思路，就是高斯过滤矩阵通过程序设置给Shader,相比在Shader中做高斯运算性能更高
// 或者相比在Shader中写死高斯过滤矩阵，但是这样会不够灵活。

// 项目：Blur.vcxproj实现高斯模糊的项目
// BlurWithoutShader.vcxproj绘制非高斯模糊项目(用于对比）
// Lighting and Texturing教程项目，来自于：Author: Frank Luna (C) All Rights Reserved

// 编程总结：
//1.获取效果文件，hlsl注意fx要是asscii格式，不能是utf8格式
//2.结构体不能VS_OUTPUT outData = VS_OUTPUT(0);而是直接VS_OUTPUT outData就好了。
//3.顶点着色器输入输出都要float4类型，且像素着色器不能输入顶点，因为已经是在光栅化阶段了。
//4.像素着色器需要注意颜色通道的组成，ARGB还是RGBA,或者RGB。
// 顶点着色器的输入
//struct VS_INPUT
//{
//	float4 POS : POSITION;
//	float2 UV : TEXCOORD0;
//};
//// 顶点着色器的输出
//struct VS_OUTPUT
//{
//	float4 POS : POSITION;
//	float2 UV : TEXCOORD0;
//};
// 4.纹理需要用Sampler正确，不能用texture来当sampler用。
// 5.alpha融合需要注意，否则会导致渲染层次有问题，融合结果。

// 6.渲染框架要正确，无论是固定管线还是着色器，每帧的Clear,BeginSecne,渲染设置顶点格式/顶点数据/索引数据/Draw函数，EndScene
// 都是要首先正确的。
// 7.对于顶点部分:
// 顶点位置要正确，索引下标要正确，还有世界视图投影变换，特别是索引如果是拷贝的那么很容易什么都不提交PIX也看不到原因
// 8. 对于像素部分：颜色没有出来，可能是光照没有关闭又没有设置材质，也可能是纹理缺失或者设置错误，alpha融合和各种融合情况
// 9.注意视口外裁剪不可见，背面剔除，深度剔除，模板过滤，导致物体绘制不出来。
// 10.要使用上述思维进行代码分析，errorBuffer/Control panel/#define D3D_DEBUG_INFO/PIX来调试分析,反复对比注释提高理论查找问题
// 如果有N卡用NVIDIA Nsight，GPA分析问题和性能更好。
// 11.图像异常：UV坐标没对，PIX传入都没对，那么是UV相关设置出了问题，最后对比发现是顶点声明出了问题：
//D3DVERTEXELEMENT9 Decl_Element [] = 
//{ 
//	{eBlurDT_Pos, 0, D3DDECLTYPE_FLOAT4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION, 0},
//	{eBlurDT_UV, 0, D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD, 0},
//	D3DDECL_END()
//};
//而不是：
	//D3DVERTEXELEMENT9 Decl_Element [] = 
	//{ 
	//	{0, 0, D3DDECLTYPE_FLOAT4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION, 0},
	//	{0, 16, D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD, 0},
	//	D3DDECL_END()
	//};

#include "d3dUtility.h"
#include "GaussianBlur.h"

LPDIRECT3DDEVICE9 Device = NULL;
LPD3DXEFFECT BlurTexEffect = NULL;
LPDIRECT3DVERTEXDECLARATION9 VT_DECL = NULL;
LPDIRECT3DVERTEXBUFFER9 VB_Pos = NULL;
LPDIRECT3DVERTEXBUFFER9 VB_UV = NULL;
LPDIRECT3DINDEXBUFFER9 IB = NULL;

D3DXHANDLE WorldViewProjectHandle  = NULL;
D3DXHANDLE BlurTechHandle = NULL;
CGaussianBlur g_BlurFilter;

LPDIRECT3DTEXTURE9 TexObj = NULL;

const int Width  = 640;
const int Height = 480;
// 模糊的顶点数和索引缓存数
static const int N_Blur_Vertex_Count = 4;
static const int N_Blur_Index_Count = 6;
static bool g_bChangeFilter = false;

// 顶点声明的数据流索引
enum BlurVertexDeclType
{
	eBlurDT_Pos,
	eBlurDT_UV
};

//struct tagTextureVertex
//{
//	float x, y, z;
//	float u, v;
//
//	tagTextureVertex(float ax, float ay, float az, float au, float av )
//	{
//		x = ax; y = ay; z = az;
//		u = au; v = av;
//	}
//	static const DWORD FVF;
//};
//
//const DWORD tagTextureVertex::FVF = D3DFVF_XYZ | D3DFVF_TEX1;

bool Setup()
{
	// 顶点声明
	 D3DVERTEXELEMENT9 Decl_Element [] = 
	 { 
		 {eBlurDT_Pos, 0, D3DDECLTYPE_FLOAT4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION, 0},
		 {eBlurDT_UV, 0, D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD, 0},
		 D3DDECL_END()
	 };

	if( FAILED(Device->CreateVertexDeclaration(Decl_Element,&VT_DECL) ) )
	{
		return false;
	}
	/* Device->CreateVertexBuffer( 4 * sizeof( tagTextureVertex ),
								 D3DUSAGE_WRITEONLY, 
								tagTextureVertex::FVF, 
								D3DPOOL_MANAGED,
								&VB_Pos,
								0
								);*/
	// 顶点位置
	if( FAILED( Device->CreateVertexBuffer(4 * sizeof(D3DXVECTOR4), D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, &VB_Pos, 0) ) )
	{
		return false;
	}
	D3DXVECTOR4 *pPos = NULL;
	if( FAILED (VB_Pos->Lock(0, 0, (void**)&pPos, 0) ) )
	{
		return false;
	}

	//tagTextureVertex *pPos = (tagTextureVertex*)VertexData;
	float fHalfWidth = Width * 0.5f;
	float fHalfHeight = Height * 0.5f;
	pPos[0] = D3DXVECTOR4(-1.0f, -1.0f, 1.0f,  1.0f);
	pPos[1] = D3DXVECTOR4(-1.0f, 1.0f, 1.0f, 1.0f);
	pPos[2] = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.0f);
	pPos[3] = D3DXVECTOR4(1.0f, -1.0f, 1.0f, 1.0f);
	/*pPos[0] = tagTextureVertex(-1.0f,  -1.0f,1.0f,  0.0f, 1.0f);
	pPos[1] = tagTextureVertex(-1.0f,  1.0f,  1.0f, 0.0f, 0.0f);
	pPos[2] = tagTextureVertex( 1.0f,   1.0f,  1.0f,1.0f, 0.0f);
	pPos[3] = tagTextureVertex( 1.0f,  -1.0f, 1.0f, 1.0f, 1.0f);*/
	VB_Pos->Unlock();

	//// 顶点UV
	if( FAILED( Device->CreateVertexBuffer(4 * sizeof(D3DXVECTOR2), D3DUSAGE_WRITEONLY, 0, D3DPOOL_MANAGED, &VB_UV, 0) ) )
	{
		return false;
	}
	D3DXVECTOR2 *UV_Data = NULL;
	if( FAILED( VB_UV->Lock(0,0,(void**)&UV_Data, 0)) )
	{
		return false;
	}
	UV_Data[0] = D3DXVECTOR2(0.0f, 1.0f);
	UV_Data[1] = D3DXVECTOR2(0.0f, 0.0f);
	UV_Data[2] = D3DXVECTOR2(1.0f, 0.0f);
	UV_Data[3] = D3DXVECTOR2(1.0f, 1.0f);
	VB_UV->Unlock();

	// 索引缓存
	if( FAILED( Device->CreateIndexBuffer( 6 * sizeof( WORD ), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &IB, 0) ) )
	{
		return false;
	}
	WORD *IB_Buffer = NULL;
	if( FAILED( IB->Lock(0,0, (void**)&IB_Buffer, 0) ) )
	{
		return false;
	}
	IB_Buffer[0] = 0;IB_Buffer[1] = 1;IB_Buffer[2] = 2;
	IB_Buffer[3] = 0;IB_Buffer[4] = 2;IB_Buffer[5] = 3;
	IB->Unlock();

	// 纹理对象设置进去就好了
	
	D3DXIMAGE_INFO imageInfo;
	D3DXCreateTextureFromFileEx(
		Device,
		"crate.png",
		D3DX_DEFAULT,
		D3DX_DEFAULT,
		D3DX_DEFAULT,
		0,
		D3DFMT_UNKNOWN,
		D3DPOOL_MANAGED,
		D3DX_DEFAULT,
		D3DX_DEFAULT,
		0,
		&imageInfo,
		NULL,
		&TexObj);

	/*Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	Device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);*/

	ID3DXBuffer* errorBuffer = 0;
	if( FAILED( D3DXCreateEffectFromFile(
		Device,
		"blur.fx",
		0,                // no preprocessor definitions
		0,                // no ID3DXInclude interface
		D3DXSHADER_DEBUG, // compile flags
		0,                // don't share parameters
		&BlurTexEffect,
		&errorBuffer) ) )
	{
		if( errorBuffer )
		{
			::MessageBox(0, (char*)errorBuffer->GetBufferPointer(), 0, 0);
			d3d::Release<ID3DXBuffer*>(errorBuffer);
		}
		return false;
	}

	// output any error messages
	//D3DXHANDLE TextureHandle =  BlurTexEffect->GetParameterByName(0, "BlurTex");
	BlurTexEffect->SetTexture(/*TextureHandle*/"BlurTex", TexObj);
	if ( TexObj != NULL )
	{
		TexObj->Release();
		TexObj = NULL;
	}
	FLOAT texSize[2] = {imageInfo.Width * 1.0f, imageInfo.Height * 1.0f};
	D3DXHANDLE TextureSizeHandle =  BlurTexEffect->GetParameterByName(0, "TexSize");
	BlurTexEffect->SetFloatArray(/*TextureSizeHandle*/"TexSize", texSize, 2);

	D3DXMATRIX W, P;
	D3DXMatrixIdentity(&W);
	//Device->SetTransform(D3DTS_WORLD, &W);
	
	D3DXVECTOR3 position( 0.0f, 0.0f, -2.0f );
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
	D3DXMATRIX V;
	D3DXMatrixLookAtLH(&V, &position, &target, &up);

	//Device->SetTransform(D3DTS_VIEW, &V);
	/*D3DXMatrixLookAtLH(&V, &position, &target, &up);
	Device->SetTransform(D3DTS_PROJECTION, &V);*/
	Device->SetRenderState( D3DRS_LIGHTING, FALSE);

	D3DXMatrixPerspectiveFovLH(
		&P,	D3DX_PI * 0.25f, // 45 - degree
		(float)Width / (float)Height,
		1.0f, 1000.0f);
	//D3DXMatrixOrthoLH(&P, float(Width), float(Height), 1.0f, 1000.0f);
	//Device->SetTransform(D3DTS_PROJECTION, &P);

	D3DXMATRIX worldViewProj = W * V * P;
	D3DXHANDLE MatrixHandle =  BlurTexEffect->GetParameterByName(0, "WorldViewProj");
	BlurTexEffect->SetMatrix(/*MatrixHandle*/"WorldViewProj", &worldViewProj);

	if( !g_bChangeFilter )
	{
		g_BlurFilter.CalculateGaussianFilter(3, 8.5f);
	}
	D3DXHANDLE filterHandle =  BlurTexEffect->GetParameterByName(0, "ArrayGaussianFilter");
	BlurTexEffect->SetFloatArray(/*filterHandle*/"ArrayGaussianFilter", g_BlurFilter.GetFilter3(), 9);

	BlurTechHandle = BlurTexEffect->GetTechniqueByName("BurTechnique");

	return true;
}

void Cleanup()
{
	d3d::Release<LPD3DXEFFECT>(BlurTexEffect);
	d3d::Release<LPDIRECT3DVERTEXBUFFER9>(VB_Pos);
	d3d::Release<LPDIRECT3DVERTEXBUFFER9>(VB_UV);
	d3d::Release<LPDIRECT3DINDEXBUFFER9>(IB);
	d3d::Release<LPDIRECT3DDEVICE9>(Device);
}

bool Display(float timeDelta)
{
	if( Device == NULL || VB_Pos == NULL || VB_UV == NULL || IB == NULL )
	{
		return false;
	}
	Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xffffffff, 1.0f, 0);
	Device->BeginScene();
	Device->SetVertexDeclaration(VT_DECL);
	Device->SetStreamSource(eBlurDT_Pos, VB_Pos, 0, sizeof(D3DXVECTOR4));
	Device->SetStreamSource(eBlurDT_UV, VB_UV, 0, sizeof(D3DXVECTOR2));
	Device->SetIndices(IB);
	/*Device->SetFVF(tagTextureVertex::FVF);
	Device->SetStreamSource(0, VB_Pos, 0, sizeof( tagTextureVertex ));*/
	//Device->SetTexture(0, TexObj);
	
	BlurTexEffect->SetTechnique("BurTechnique"/*BlurTechHandle*/);
	UINT numPasses = 0;
	BlurTexEffect->Begin(&numPasses, 0);
	for( int i = 0; i <  numPasses; i++ )
	{
		BlurTexEffect->BeginPass(i);
		BlurTexEffect->CommitChanges();
		// 绘制
		Device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 4,0, 2);

		BlurTexEffect->EndPass();
	}
	//Device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 4,0, 2);
	BlurTexEffect->End();

	Device->EndScene();
	Device->Present(0, 0, 0, 0);

	return true;
}

//
// WndProc
//
LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch( msg )
	{
	case WM_DESTROY:
		::PostQuitMessage(0);
		break;

	case WM_KEYDOWN:
		if( wParam == VK_ESCAPE )
			::DestroyWindow(hwnd);

		break;
	}
	return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

//
// WinMain
//
int WINAPI WinMain(HINSTANCE hinstance,
	HINSTANCE prevInstance, 
	PSTR cmdLine,
	int showCmd)
{
	if(!d3d::InitD3D(hinstance,
		Width, Height, true, D3DDEVTYPE_HAL, &Device))
	{
		::MessageBox(0, "InitD3D() - FAILED", 0, 0);
		return 0;
	}

	if(!Setup())
	{
		::MessageBox(0, "Setup() - FAILED", 0, 0);
		return 0;
	}

	d3d::EnterMsgLoop( Display );

	Cleanup();

	Device->Release();

	return 0;
}