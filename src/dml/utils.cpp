#include <cmath>
#include "utils.h"
#include <iostream>
#include <ctime>
#include <time.h>
#include <omp.h>
#include <stdio.h>

void load_dense_data(float ** data, int num_data, int feat_dim, char * file)
{
  FILE * fp=fopen(file,"rb");
  for(int i=0;i<num_data;i++)
    fread(data[i], sizeof(float), feat_dim, fp);
  fclose(fp);
}

int myrandom (int i) 
{ 
  std::srand(777);
  return std::rand()%i;
}

int myrandom2 (int i) 
{ 
  std::srand(time(NULL));
  return std::rand()%i;
}


//get time
float get_time() {
  struct timespec start;
  clock_gettime(CLOCK_MONOTONIC, &start);
  return (start.tv_sec + start.tv_nsec/1000000000.0);
}
float randfloat()
{
	int prec=10000;
	int rint=rand()%prec;
	float rv=(rint*2-prec)*1.0/prec/100;
	return rv;
}

//z=x-y
void vec_sub2(float * x, float * y, float * z, int dim)
{

	for(int i=0;i<dim;i++)
		z[i]=x[i]-y[i];
}
//y=mat*x
void mat_vec_mul2(float ** mat, float * x, float * y, int num_rows_mat, int num_cols_mat)
{

	for(int i=0;i<num_rows_mat;i++)
	{
		float sum=0;
		for(int j=0;j<num_cols_mat;j++)
		{
			sum+=mat[i][j]*x[j];
		}
		y[i]=sum;
	}
}


//z=x-y
void vec_sub(float * x, float * y, float * z, int dim)
{
	#pragma omp parallel for
	for(int i=0;i<dim;i++)
		z[i]=x[i]-y[i];
}
//y=mat*x
void mat_vec_mul(float ** mat, float * x, float * y, int num_rows_mat, int num_cols_mat)
{
       #pragma omp parallel for
	for(int i=0;i<num_rows_mat;i++)
	{
		float sum=0;
		for(int j=0;j<num_cols_mat;j++)
		{
			sum+=mat[i][j]*x[j];
		}
		y[i]=sum;
	}
}
//return ||vec||^{2}
float vec_sqr(float * vec,int dim)
{
	float sqr=0;
	for(int i=0;i<dim;i++)
		sqr+=vec[i]*vec[i];
	return sqr;
}
//void data sparse format
void load_sparse_data(float ** data, int num_data, char * file)
{

  FILE * fp=fopen(file,"r");
  int num_tokens, idx;
  float value;
  for(int i=0;i<num_data;i++)
  {
    fscanf(fp,"%d", &num_tokens);
	fscanf(fp,"%d", &num_tokens);

    for(int j=0;j<num_tokens;j++){
      fscanf(fp,"%d:%f",&idx, &value);
      data[i][idx]=value;
    }
  }
  fclose(fp);
}
//load dense data
void load_pairs(pair * pairs, int num_pairs,char * file)
{
	std::ifstream infile;
	infile.open(file);
	for(int i=0;i<num_pairs;i++)
	{
		infile>>pairs[i].x>>pairs[i].y;
	}
	infile.close();
}

void load_pairs_bin(pair * pairs, int num_pairs, char * file)
{
	FILE * fp=fopen(file,"rb");
	int * tmp=new int[num_pairs*2];
	fread(tmp, sizeof(int),num_pairs*2, fp);
	for(int i=0;i<num_pairs;i++){
		pairs[i].x=tmp[i*2];
		pairs[i].y=tmp[i*2+1];
	}
       delete[]tmp;
	fclose(fp);
}
















