#ifndef _VDEC_VIDEO_HI_
#define _VDEC_VIDEO_HI_

void vdec_ntsc_common_init(void);
void vdec_pal_common_init(void);
void nvp1918c_720h_nt( void );
void nvp1918c_720h_pal( void );
void nvp1918c_960h_nt( void );
void nvp1918c_960h_pal( void );
void nvp1918c_1280h_nt( void );
void nvp1918c_1280h_pal( void );
void nvp1918c_1440h_nt( void );
void nvp1918c_1440h_pal( void );


void vdec_ntsc_common_init_h720(void);
void vdec_pal_common_init_h720(void);
void vdec_ntsc_common_init_h960(void);
void vdec_pal_common_init_h960(void);
void nvp1914c_h720_mode ( unsigned char vformat );
void nvp1914c_h960_mode ( unsigned char vformat );


void vdec_video_set_contrast(unsigned int ch, unsigned int value, unsigned int v_format);
void vdec_video_set_brightness(unsigned int ch, unsigned int value, unsigned int v_format);
void vdec_video_set_saturation(unsigned int ch, unsigned int value, unsigned int v_format);
void vdec_video_set_hue(unsigned int ch, unsigned int value, unsigned int v_format);


void vdec_low_resoultion_enable(unsigned char ch);
void vdec_low_resoultion_disable(unsigned char ch);
void vdec_bw_detection_enable(unsigned int ch);
void vdec_bw_detection_disable(unsigned int ch);
unsigned int vdec_bw_black_count_read(unsigned int ch);
unsigned int nvc1700_init(unsigned char vformat, unsigned char outmode);  
#endif
