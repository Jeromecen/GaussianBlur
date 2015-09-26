# GaussianBlur
Gaussian Blur with HLSL Effect file 高斯图像模糊算法的实现\n
Author:JeromeCen 2015.6.12 编译出问题请联系：Jeromecen@hotmail.com qq:1021900404
安装环境：VS2010 及其以后版本; 项目需要配置下DX SDK库head file和lib directory指定.

启动项目：\Lighting and Texturing\Lighting and Texturing.sln
项目：Blur.vcxproj实现高斯模糊的项目
BlurWithoutShader.vcxproj绘制非高斯模糊项目(用于对比）
Lighting and Texturing教程项目，来自于：Author: Frank Luna (C) All Rights Reserved

 项目目标：
 1.提高了一个完整的fx框架编写Shader,解决掉了编写过程中遇到的所有问题。验证了在
 FX框架可以不用GetParameterByName获取句柄的，直接用字符串作为句柄即可。
 2.对高斯模糊图片提供了一个思路，就是高斯过滤矩阵通过程序设置给Shader,相比在Shader中做高斯运算性能更高
 或者相比在Shader中写死高斯过滤矩阵，但是这样会不够灵活。

 编程总结：
1.获取效果文件，hlsl注意fx要是asscii格式，不能是utf8格式
2.结构体不能VS_OUTPUT outData = VS_OUTPUT(0);而是直接VS_OUTPUT outData就好了。
3.顶点着色器输入输出都要float4类型，且像素着色器不能输入顶点，因为已经是在光栅化阶段了。
4.像素着色器需要注意颜色通道的组成，ARGB还是RGBA,或者RGB。
 顶点着色器的输入
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
 4.纹理需要用Sampler正确，不能用texture来当sampler用。
 5.alpha融合需要注意，否则会导致渲染层次有问题，融合结果。

 6.渲染框架要正确，无论是固定管线还是着色器，每帧的Clear,BeginSecne,渲染设置顶点格式/顶点数据/索引数据/Draw函数，EndScene
 都是要首先正确的。
 7.对于顶点部分:
 顶点位置要正确，索引下标要正确，还有世界视图投影变换，特别是索引如果是拷贝的那么很容易什么都不提交PIX也看不到原因
 8. 对于像素部分：颜色没有出来，可能是光照没有关闭又没有设置材质，也可能是纹理缺失或者设置错误，alpha融合和各种融合情况
 9.注意视口外裁剪不可见，背面剔除，深度剔除，模板过滤，导致物体绘制不出来。
 10.要使用上述思维进行代码分析，errorBuffer/Control panel/#define D3D_DEBUG_INFO/PIX来调试分析,反复对比注释提高理论查找问题
 如果有N卡用NVIDIA Nsight，GPA分析问题和性能更好。
 11.图像异常：UV坐标没对，PIX传入都没对，那么是UV相关设置出了问题，最后对比发现是顶点声明出了问题：
D3DVERTEXELEMENT9 Decl_Element [] = 
{ 
	{eBlurDT_Pos, 0, D3DDECLTYPE_FLOAT4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION, 0},
	{eBlurDT_UV, 0, D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD, 0},
	D3DDECL_END()
};
而不是：
D3DVERTEXELEMENT9 Decl_Element [] = 
{ 
	{0, 0, D3DDECLTYPE_FLOAT4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION, 0},
	{0, 16, D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD, 0},
	D3DDECL_END()
};
