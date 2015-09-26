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
// ����
texture BlurTex;

// ������ͼͶӰ����
matrix WorldViewProj;

// �����С
float2    TexSize;
// ��˹���˾�����ʱ��3x3��
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
// ������ɫ��������
struct VS_INPUT
{
	float4 POS : POSITION;
    float2 UV : TEXCOORD0;
};
// ������ɫ�������
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

// ������ɫ��������
struct PS_INPUT
{
     float2 UV : TEXCOORD0;
};
// ������ɫ�������
struct PS_OUTPUT
{
    vector diffuse : COLOR0;
};


//�������˲������ĺ���
float4 dip_filter3(float3x3 _filter , sampler2D _image, float2 _xy, float2 texSize)

{
     //�������������ƫ��
	 float2 _filter_pos_delta[3][3] =

     {

        { float2(-1.0 , -1.0) , float2(0,-1.0), float2(1.0 , -1.0) },

        { float2( -1.0 , 0.0) , float2(0.0, 0.0), float2(1.0 ,  0.0) },

        { float2( -1.0 , 1.0) , float2(0, 1.0), float2(1.0 ,  1.0) },

     };

     //���յ������ɫ
     float4 final_color = float4(0.0,0.0,0.0,1.0f);
     //��ͼ�����˲�����

     for(int j = 0 ; j < 3 ; j++ )

     {
         for(int i = 0 ; i < 3 ; i++ )

         {

              //��������㣬�õ���ǰ���ظ��������ص�����
              float2 _xy_new = float2(_xy.x + _filter_pos_delta[i][j].x ,

                                  _xy.y + _filter_pos_delta[i][j].y);

              float2 _uv_new = float2(_xy_new.x/texSize.x , _xy_new.y/texSize.y);

              //�����������˲���Ȩ�أ�Ȼ���ۼ�
              final_color += tex2D( _image, _uv_new ) * _filter[i][j];

         }
     }

     return final_color;
}

PS_OUTPUT PS_Main( PS_INPUT input)
{

   PS_OUTPUT output;
   float2  intXY = float2(input.UV.x * TexSize.x , input.UV.y * TexSize.y);
   //����ģ���������˲���
   output.diffuse = dip_filter3( ArrayGaussianFilter , BlurSampler , intXY, TexSize);
   return output;
}

// Ч�����
technique BurTechnique
{
    pass P0
    {
		// ״̬����
		Lighting      = false;
		// ����
		vertexShader = compile vs_2_0 VS_Main();  
		
		AlphaBlendEnable = false;
		StencilEnable = false;
        pixelShader = compile ps_2_0 PS_Main(); 
	}
}
