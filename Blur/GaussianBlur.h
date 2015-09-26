#ifndef _GAUSSIANBLUR_H_
#define _GAUSSIANBLUR_H_
class CGaussianBlur
{
public:
	CGaussianBlur(){};
	~CGaussianBlur(){};
	// 计算高斯权值
	void CalculateGaussianFilter(int nLength, float rho);
	float* GetFilter3();
	float* GetFilter4();
	float GaussianFilter3[9];
private:
	// 参数高斯权值
	float GetGaussianDistribution( float x, float y, float rho );
	// 是否接近0
	inline bool FEqual(float fValue1, float fValue2 );

	// 高斯模糊矩阵，提供3x3的和4x4的
	float GaussianFilter4[16];
};

#endif