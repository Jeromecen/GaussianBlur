////////////////////////////////////////////////////////////////////////////
// 
// File: ps_alphablend.txt
// 
// Author: Frank Luna (C) All Rights Reserved
//
// System: AMD Athlon 1800+ XP, 512 DDR, Geforce 3, Windows XP, MSVC++ 7.0 
//
// Desc: Pixel shader that does multi texturing.
//          
////////////////////////////////////////////////////////////////////////////

//
// Globals
// 纹理
texture BlurTex;

// 世界视图投影矩阵
matrix WorldViewProj;

// 纹理大小
float2    TexSize;
// 高斯过滤矩阵，暂时用3x3的
float3x3 ArrayGaussianFilter;

sampler BlurSampler = sampler_state
{
	Texture = <BlurTex>;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
};
//
// Structures
// 顶点着色器的输入
struct VS_INPUT
{
	float4 POS : POSITION;
    float2 UV : TEXCOORD0;
};
// 顶点着色器的输出
struct VS_OUTPUT
{
	float4 POS : POSITION;
    float2 UV : TEXCOORD0;
};

VS_OUTPUT VS_Main( VS_INPUT input ) 
{
	VS_OUTPUT outData;	
	outData.POS = mul( input.POS, WorldViewProj);
	outData.UV = input.UV;
	return outData;
}

// 像素着色器的输入
struct PS_INPUT
{
     float2 UV : TEXCOORD0;
};
// 像素着色器的输出
struct PS_OUTPUT
{
    vector diffuse : COLOR0;
};


//用来做滤波操作的函数
float4 dip_filter3(float3x3 _filter , sampler2D _image, float2 _xy, float2 texSize)

{
     //纹理坐标采样的偏移
	 float2 _filter_pos_delta[3][3] =

     {

        { float2(-1.0 , -1.0) , float2(0,-1.0), float2(1.0 , -1.0) },

        { float2( -1.0 , 0.0) , float2(0.0, 0.0), float2(1.0 ,  0.0) },

        { float2( -1.0 , 1.0) , float2(0, 1.0), float2(1.0 ,  1.0) },

     };

     //最终的输出颜色
     float4 final_color = float4(0.0,0.0,0.0,1.0f);
     //对图像做滤波操作

     for(int j = 0 ; j < 3 ; j++ )

     {
         for(int i = 0 ; i < 3 ; i++ )

         {

              //计算采样点，得到当前像素附近的像素的坐标
              float2 _xy_new = float2(_xy.x + _filter_pos_delta[i][j].x ,

                                  _xy.y + _filter_pos_delta[i][j].y);

              float2 _uv_new = float2(_xy_new.x/texSize.x , _xy_new.y/texSize.y);

              //采样并乘以滤波器权重，然后累加
              final_color += tex2D( _image, _uv_new ) * _filter[i][j];

         }
     }

     return final_color;
}

PS_OUTPUT PS_Main( PS_INPUT input)
{

   PS_OUTPUT output;
   float2  intXY = float2(input.UV.x * TexSize.x , input.UV.y * TexSize.y);
   //用于模糊操作的滤波器
   output.diffuse = dip_filter3( ArrayGaussianFilter , BlurSampler , intXY, TexSize);
   return output;
}

// 效果框架
technique BurTechnique
{
    pass P0
    {
		// 状态设置
		Lighting      = false;
		// 编译
		vertexShader = compile vs_2_0 VS_Main();  
		
		AlphaBlendEnable = false;
		StencilEnable = false;
        pixelShader = compile ps_2_0 PS_Main(); 
	}
}
