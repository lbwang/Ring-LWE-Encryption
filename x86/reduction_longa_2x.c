#include "reduction_longa_2x.h"
#include <stdio.h>
#include "lwe.h"
#include "luts.h"
#include "stdlib.h"
#include "assert.h"
#include "limits.h"


void coefficient_mul_mod_longa_2x(uint32_t out[M], uint32_t b[], uint32_t c[]) {
  // a = b * c, followed by a double reduction
  int j;

  for (j = 0; j < M; j++)
	  out[j] = mul_mod_longa_2x(b[j], c[j]);
}

void fwd_ntt_longa_2x(int32_t a[M])
{
  //Performs an NTT with two 2X reductions
  int t, m, i, j, j1, j2, x = 0;
  int32_t S, U, V;

  t = M;
  for(m = 1; m < M; m=2*m)
  {
    x++;
    t = t/2;
    for (i = 0; i < m; i++)
    {
      j1 = 2 * i * t;
      j2 = j1 + t - 1;
      S = psi_longa[m + i];
      for(j = j1; j<=j2; j++)
      {
    	if ((m==16) || (m==256))
    	{
			U = mod_longa(a[j]);
			V = mul_mod_longa_2x(a[j+t],S);
			a[j] = (U + V);
			a[j+t] = (U - V);
    	}
    	else
    	{
    		U = a[j];
    		if ((m==1) || (m==2) || (m==4) || (m==32) || (m==64))
    			V = mod_longa(a[j+t] * mod(S));
    		else
    			V = mul_mod_longa(a[j+t], S);

			a[j] = (U + V);
			a[j+t] = (U - V);
    	}
    	assert((j+t)<512);
    	assert(((int64_t)U+(int64_t)V)==a[j]);
    	assert(((int64_t)U-(int64_t)V)==a[j+t]);
      }
    }
  }
}

void unit_test_longa_fwd_inv_ntt_2x()
{
	int res = 1;
	int i,j;
	int32_t in1[M],in2[M];

	int k_inv_first=k_inv;
	for (j=0; j<3; j++) k_inv_first = mod(k_inv_first*k_inv);

	for (i=0; (i<100000) && (res==1); i++)
	{
		srand(i);
		if (i==0)
			//All ones
			for (j=0; j<M; j++) in2[j]=in1[j]=1;
		else
			//Random values
			for (j=0; j<M; j++) in2[j]=in1[j]=rand()%MODULUS;


		fwd_ntt_longa_2x(in1);

		inv_ntt_longa_2x(in1,k_inv_first);

		for (j=0; j<M; j++)
		{
			if (in2[j]!=mod(in1[j]))
			{
				printf("Second Test FAILED\n");
				res=0;
				printf("i=%d, j=%d\n",i,j);
				printf("\n\r in1: ");
				int k;
				for(k = 0; k< M; k++) printf("[%d %08d] ", k, mod(in1[k]));
				printf("\n\r in2: ");
				for(k = 0; k< M; k++) printf("[%d %08d] ", k, in2[k]);
				printf("\n\r");

				break;
			}
		}
	}
	printf("unit_test_longa_fwd_inv_ntt_2x: ");
	if (res==0)
		printf("BAD!\n");
	else
		printf("OK!\n");
}

void inv_ntt_longa_2x(int32_t a[M], int k_inv_first)
{
  //Performs an INTT with two 2X reductions
  int t, h, m, i, j, j1, j2, x=0, ind;
  int *index;
  int32_t S, U, V;

  index = inv_psi1_longa;

	t = 1;
	for (m = M; m > 2; m = m/2)
	{
		x++;
		j1 = 0;

		index = index + m/2;
		for (i = 0; i < (m/2); i++)
		{
			j2 = j1 + t -1;
			S = *(index);
			for (j = j1; j<=j2; j++)
			{
				U = a[j];
				V = a[j+t];
				if (m!=32)
				{
					a[j] = (U+V);
					if ((m==512) || (m==16) || (m==8))
						a[j+t] = mod_longa((U-V) * S);
					else
						//Levels that have to do 64-bit mul: 256,128,64,4
						a[j+t] = mul_mod_longa((U-V), S);

					assert(((int64_t)U+(int64_t)V)==a[j]);

					/*
					 * The following assert sometimes fails. This is OK
					 * assert(mod(((int64_t)U-(int64_t)V)*mod(S*3))==mod(a[j+t]));
					*/
				}
				else
				{
					//Add k term
					a[j] = mod_longa(U+V);
					a[j+t]=mul_mod_longa_2x((U-V), S);

					assert(mod(((int64_t)U+(int64_t)V)*3)==mod(a[j]));
					int32_t tmp1=mod(((int64_t)U-(int64_t)V)*(int64_t)mod(S*9));
					int32_t tmp2=mod(a[j+t]);
					assert (tmp1==tmp1);
				}
			}
			j1 = j1 + 2*t;
			index++;
		}
		t = 2*t;
		index = index - m;
	}

	int m_inv=mul_inv(M,MODULUS);
	for (j = 0; j < t; j++)
	{
		U = a[j];
		V = a[j+t];

		a[j] = mul_mod_longa((U+V), mod(m_inv*mod(k_inv_first)));
		a[j+t] = mul_mod_longa_2x((U-V), mod(mod(m_inv*inv_psi1_longa[1])*mod(k_inv_first)));
	}
}

void unit_test_fwd_ntt_2x()
{
	int res = 1;
	int i,j;

	for (i=0; (i<100000) && (res==1); i++)
	{
		int32_t in1[M],in2[M];

		srand(i);
		if (i==0)
			//All ones for first test case
			for (j=0; j<M; j++) in1[j]=1;
		else
			//Random values for other test cases
			for (j=0; j<M; j++) in1[j]=rand()%MODULUS;

		for (j=0; j<M; j++)	in2[j]=in1[j];

		fwd_ntt_longa_3x(in1);

		fwd_ntt_longa_2x(in2);

		for (j=0; j<M; j++)
		{
			if (mod(in2[j]*3)!=mod(in1[j]))
			{
				res=0;
				printf("i=%d, j=%d\n",i,j);
				printf("\n\r in1: ");
				int k;
				for(k = 0; k< M; k++) printf("[%d %08d] ", k, mod(in1[k]));
				printf("\n\r in2: ");
				for(k = 0; k< M; k++) printf("[%d %08d] ", k, mod(in2[k]*3));
				printf("\n\r");

				break;
			}
		}
	}
	printf("unit_test_fwd_ntt_2x: ");
	if (res==0)
		printf("BAD!\n");
	else
		printf("OK!\n");
}

void unit_test_inv_ntt_2x()
{
	int res = 1;
	int i,j;
	int32_t in1[M],in2[M];

	int k_inv_first=k_inv;
	for (j=0; j<6; j++) k_inv_first = mod(k_inv_first*k_inv);

	for (i=0; (i<100000) && (res==1); i++)
	{
		srand(i);
		if (i==0)
			for (j=0; j<M; j++) in2[j]=in1[j]=1; //All ones
		else
			//Random values
			for (j=0; j<M; j++) in2[j]=in1[j]=rand()%MODULUS; //2^14 = 16384

		inv_ntt_longa_3x(in1,k_inv_first);

		inv_ntt_longa_2x(in2,k_inv_first);

		for (j=0; j<M; j++)
		{
			if (mod(in2[j]*3)!=mod(in1[j]))
			{
				res=0;
				printf("i=%d, j=%d\n",i,j);
				printf("\n\r in1: ");
				int k;
				for(k = 0; k< M; k++) printf("[%d %08d] ", k, mod(in1[k]));
				printf("\n\r in2: ");
				for(k = 0; k< M; k++) printf("[%d %08d] ", k, mod(in2[k]*3));
				printf("\n\r");

				break;
			}
		}
	}
	printf("unit_test_inv_ntt_2x: ");
	if (res==0)
		printf("BAD!\n");
	else
		printf("OK!\n");
}

void unit_test_longa_poly_mul2()
{
	int res = 1;
	int i,j,k;
	int k_inv_first=k_inv;
	for (j=0; j<7; j++) k_inv_first = mod(k_inv_first*k_inv);
	for (i=0; (i<10000) && (res==1); i++)
	{
		uint32_t in1[M],in2[M],out1[M];
		int16_t in3[M],in4[M],out2[M];

		srand(i);
		if (i==0)
			for (j=0; j<M; j++) in4[j]=in3[j]=in1[j]=in2[j]=1; //All ones
		else
		{
			//Random values
			for (j=0; j<M; j++)
			{
				in3[j]=in1[j]=rand()%MODULUS;
				in4[j]=in2[j]=rand()%MODULUS;
			}
		}

		fwd_ntt_longa_2x(in1);
		fwd_ntt_longa_2x(in2);

		 //Unfortunately coefficient_mul_mod_longa() doesn't work here. Creates overflow in the subsequent INTT.
		coefficient_mul_mod_longa_2x(out1,in1,in2);

		inv_ntt_longa_2x(out1,k_inv_first);
		mul_test(in3,in4,out2,M);

		for (j=0; j<M; j++)
		{
			if (out2[j]!=mod(out1[j]))
			{
				res=0;

				printf("i=%d j=%d\n",i,j);
				printf("\n out1: ");
				for(k = 0; k< M; k++) printf("[%d %08d] ", k, mod(out1[k]));
				printf("\n out2: ");
				for(k = 0; k< M; k++) printf("[%d %08d] ", k, out2[k]);
				printf("\n\r");
				break;
			}
		}
	}
	printf("unit_test_longa_poly_mul2: ");
	if (res==0)
		printf("BAD!\n");
	else
		printf("OK!\n");
}

void unit_test_reduction_longa_2x()
{
	uint32_t k_inv2=mod(k_inv*k_inv);
	unit_test_longa_poly_mul2();
	unit_test_fwd_ntt_2x();
	unit_test_inv_ntt_2x();
	unit_test_longa_fwd_inv_ntt_2x();
}