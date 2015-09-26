#ifndef _GAUSSIANBLUR_H_
#define _GAUSSIANBLUR_H_
class CGaussianBlur
{
public:
	CGaussianBlur(){};
	~CGaussianBlur(){};
	// �����˹Ȩֵ
	void CalculateGaussianFilter(int nLength, float rho);
	float* GetFilter3();
	float* GetFilter4();
	float GaussianFilter3[9];
private:
	// ������˹Ȩֵ
	float GetGaussianDistribution( float x, float y, float rho );
	// �Ƿ�ӽ�0
	inline bool FEqual(float fValue1, float fValue2 );

	// ��˹ģ�������ṩ3x3�ĺ�4x4��
	float GaussianFilter4[16];
};

#endif