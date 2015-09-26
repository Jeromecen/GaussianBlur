#include "GaussianBlur.h"
#include <math.h>
#include <float.h>
static const int N_FilterLength3 = 3;
static const int N_FilterLength4 = 4;

bool CGaussianBlur::FEqual(float fValue1, float fValue2 )
{
	if( fabs(fValue1 - fValue2) < FLT_EPSILON )
	{
		return true;
	}
	else
	{
		return false;
	}
}

float CGaussianBlur::GetGaussianDistribution( float x, float y, float rho )
{
	float g = 1.0f / sqrt( 2.0f * 3.141592654f * rho * rho );
	return g * exp( -(x * x + y * y) / (2 * rho * rho) );
}

void CGaussianBlur::CalculateGaussianFilter(int nLength, float rho)
{
	int nSideLength = (int)floor(nLength * 0.5f);
	int nIndex = 0;
	float fTotalWeight = 0;
	for( int j = -nSideLength; j <= nSideLength ; j++ )
	{
		for( int i = -nSideLength; i <= nSideLength; i++ )
		{
			if( nLength == N_FilterLength3 )
			{
				GaussianFilter3[nIndex] = GetGaussianDistribution(i, j, rho);
				fTotalWeight += GaussianFilter3[nIndex];
			}
			else if( nLength == N_FilterLength4 )
			{
				GaussianFilter4[nIndex] = GetGaussianDistribution(i, j, rho);
				fTotalWeight += GaussianFilter4[nIndex];
			}
			else
			{
				// ERROR Log
				return;
			}

			nIndex++;
		}
	}

	// 避免除0
	if( FEqual( fTotalWeight, 0.0f) )
	{
		// ERROR Log
		return;
	}

	nIndex = 0;
	// 归一化一下
	for( int i = 0; i < nLength; i++ )
	{
		for( int j = 0; j < nLength; j++ )
		{
			if( nLength == N_FilterLength3 )
			{
				GaussianFilter3[nIndex] = GaussianFilter3[nIndex] / fTotalWeight;
			}
			else if( nLength == N_FilterLength4 )
			{
				GaussianFilter4[nIndex] = GaussianFilter4[nIndex] / fTotalWeight;
			}
			nIndex++;
		}
	}
}

float* CGaussianBlur::GetFilter3()
{
	return GaussianFilter3;
}

float* CGaussianBlur::GetFilter4()
{
	return GaussianFilter4;
}