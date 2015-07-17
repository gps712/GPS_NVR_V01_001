#ifndef _H_FILE_H
#define _H_FILE_H

#ifdef __cplusplus
extern "C"
{
#endif


extern gps_contrl data_contrl;


int gps_save(char *path ,const void* buffer, size_t size, size_t count);
int gps_read(char *path,void *buffer,size_t size, size_t count,int*num);
int gps_update(char *path ,int* offset);
int gps_data_move(char *bpath,char*path,int offset);
int gps_maxid(char *path);





#ifdef __cplusplus
}; //end of extern "C" {
#endif

#endif
