#ifndef UTIL_H_
#define UTIL_H_


#include <math.h>
#include <stdlib.h>
#include <fstream>
#include <time.h>
#include "aux_data.h"

int myrandom (int i);
int myrandom2 (int i);
//get time
float get_time();
float randfloat();
//z=x-y
void vec_sub(float * x, float * y, float * z, int dim);
//y=mat*x
void mat_vec_mul(float ** mat, float * x, float * y, int num_rows_mat, int num_cols_mat);

//z=x-y
void vec_sub2(float * x, float * y, float * z, int dim);
//y=mat*x
void mat_vec_mul2(float ** mat, float * x, float * y, int num_rows_mat, int num_cols_mat);


//return ||vec||^{2}
float vec_sqr(float * vec,int dim);
//void data sparse format
void load_sparse_data(float ** data, int num_data, char * file);
//load dense data
void load_pairs(pair * pairs, int num_pairs,char * file);

void load_dense_data(float ** data, int num_data, int feat_dim, char * file);
void load_pairs_bin(pair * pairs, int num_pairs, char * file);
#endif
