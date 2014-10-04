#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <ctime>

int main(int argc, char *argv[])
{
  if(argc!=9){
    std::cout<<"usage: <ori_feat_file> <ori_label_file> <dst_feat_file> <dst_label_file> <dst_meta_file> <num_data> <dim_feat> <percent>"<<std::endl;
  }
  FILE * fp=fopen(argv[1],"rb");
  FILE * fp_w=fopen(argv[3],"wb");
  
  std::ifstream infile;
  std::ofstream outfile;
  infile.open(argv[2]);
  outfile.open(argv[4]);
  
  int num_data=atoi(argv[6]);
  int dim_feat=atoi(argv[7]);
  float pct=atof(argv[8]);
  float * tmp=new float[dim_feat];
  int cnt=0;
  for(int i=0;i<num_data;i++){
    fread(tmp, sizeof(float),dim_feat, fp);
    int label;
    infile>>label;
    if(rand()%10000/10000.0<pct){
       fwrite(tmp, sizeof(float), dim_feat, fp_w);
       outfile<<label<<std::endl;
	cnt++;
    }
  }
  delete [] tmp;
  infile.close();
  outfile.close();
  fclose(fp);
  fclose(fp_w);

  outfile.open(argv[5]);
  outfile<<cnt<<"\t"<<argv[3]<<"\t"<<argv[4]<<std::endl;
  outfile.close();
}










