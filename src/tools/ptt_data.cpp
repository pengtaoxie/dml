#include <fstream>
#include <stdio.h>
#include <vector>
#include <iostream>
int main(int argc, char * argv[])
{
  if(argc!=6){
    std::cout<<"usage: <original_data_record> <num_ptts> <dim_feat> <save_dir> <save_prefix> "<<std::endl;
    return 0;
  }
  //load meta data
  int dim_feat=atoi(argv[3]);
  
  char ori_feat_file[512];
  char ori_label_file[512];
  int num_data;
  std::ifstream infile;
  infile.open(argv[1]);
  infile>>num_data>>ori_feat_file>>ori_label_file;
  
  infile.close();

  //break data
  int num_ptts=atoi(argv[2]);
  
  int num_data_major=num_data/num_ptts;
  int num_data_last=num_data-num_data_major*(num_ptts-1);
  
  char ptt_files_meta[512];
  sprintf(ptt_files_meta,"%s/%s_ptt_meta.txt",argv[4], argv[5]);
  std::ofstream out_meta;
  out_meta.open(ptt_files_meta);
  
  std::vector<FILE *> feat_writers;
  std::vector<FILE *> label_writers;
  for(int i=0;i<num_ptts;i++){
    char ptt_feat_file[512];
    sprintf(ptt_feat_file,"%s/%s_feat_%d.dat",argv[4], argv[5],i);
    
    FILE * fpt_feat=fopen(ptt_feat_file,"wb");
    feat_writers.push_back(fpt_feat);

    char ptt_label_file[512];
    sprintf(ptt_label_file,"%s/%s_label_%d.txt",argv[4],argv[5],i);
    
    FILE * fpt_lb=fopen(ptt_label_file,"w");
    label_writers.push_back(fpt_lb);
    if(i<num_ptts-1)
      out_meta<<num_data_major<<"\t";
    else
      out_meta<<num_data_last<<"\t";
    out_meta<<ptt_feat_file<<"\t"<<ptt_label_file<<std::endl;
  }
  out_meta.close();

  FILE * fp=fopen(ori_feat_file,"rb");
  infile.open(ori_label_file);
  float * tmp=new float[dim_feat];
  for(int i=0;i<num_data;i++)
  {
    fread(tmp,sizeof(float),dim_feat,fp);
    int label;
    infile>>label;
    
    int ptt=i/num_data_major;
    if(ptt==num_ptts)
      ptt=num_ptts-1;
    //std::cout<<label<<"\t"<<ptt<<std::endl;
    fwrite(tmp,sizeof(float),dim_feat,feat_writers[ptt]);
    fprintf(label_writers[ptt],"%d\n",label);
  }
  delete[]tmp;
  fclose(fp);
  infile.close();

  
  for(int i=0;i<num_ptts;i++)
    fclose(feat_writers[i]);
  for(int i=0;i<num_ptts;i++)
    fclose(label_writers[i]);

  return 0;
}



















