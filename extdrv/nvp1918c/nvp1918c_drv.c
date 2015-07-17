#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/system.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/string.h>
#include <linux/list.h>
#include <asm/delay.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>
#include <asm/bitops.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <linux/moduleparam.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>

#include "../gpio_i2c/gpio_i2c.h"
#include "nvp1918c.h"
#include "video.h"
#include "coax_protocol.h"
#include "motion.h"

//extern void sil9034_1080i60_init(void);
//extern void sil9034_1080i50_init(void);
//extern void sil9034_1080p30_init(void);

unsigned int vdec_mode = VDEC_PAL_960;
#ifdef NVC1700A_NVP1918C
unsigned int vdec_cnt = 2;
unsigned int vdec_slave_addr[2] = {0x64, 0x66};
#else
unsigned int vdec_cnt = 0;
unsigned int vdec_slave_addr[4] = {0x60, 0x62, 0x64, 0x66};
#endif 

module_param(vdec_mode, uint, S_IRUGO);

int check_id(unsigned int dec)
{
	int ret;
	gpio_i2c_write(dec, 0xFF, 0x01);
	ret = (gpio_i2c_read(dec, 0xf4)<<8) | (gpio_i2c_read(dec, 0xf5));
	return ret;
}

int vdec_open(struct inode * inode, struct file * file)
{
	return 0;
} 

int vdec_close(struct inode * inode, struct file * file)
{
	return 0;
}

//int vdec_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
int vdec_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned int on;
	unsigned long ptz_ch;
	unsigned int value;
	unsigned int sens[16];
	vdec_video_mode vmode;
	vdec_video_adjust v_adj;
	

	switch (cmd)
	{
        case IOC_VDEC_SET_VIDEO_MODE:
            if (copy_from_user(&vmode, arg, sizeof(vdec_video_mode)))
				return -1;

			vdec_mode = vmode.mode;

			if(vmode.mode == VDEC_NTSC)
			{  
			    #ifdef NVP1914C
				nvp1914c_h720_mode (NTSC);
				#else
				nvp1918c_720h_nt();
				#ifdef NVC1700A_NVP1918C
				nvc1700_init(NVC1700_VM_NTSC, NVC1700_OUTMODE_4D1_16CIF);
				#endif 
				#endif

			}
			else if(vmode.mode == VDEC_PAL)
			{	
				 #ifdef NVP1914C
				nvp1914c_h720_mode (PAL);
				#else
				nvp1918c_720h_pal();
				#ifdef NVC1700A_NVP1918C
			    nvc1700_init(NVC1700_VM_PAL, NVC1700_OUTMODE_4D1_16CIF);
				#endif 
				#endif
			}
			else if(vmode.mode == VDEC_NTSC_960)
			{	
				#ifdef NVP1914C
				nvp1914c_h960_mode (NTSC);
				#else
				nvp1918c_960h_nt();
				#endif
			}
			else if(vmode.mode == VDEC_PAL_960)
			{	
				#ifdef NVP1914C
				nvp1914c_h960_mode (PAL);
				#else
				nvp1918c_960h_pal();
				#endif 
			}
			break;
#if 1
		case IOC_VDEC_SET_BRIGHTNESS:
            if(copy_from_user(&v_adj, arg, sizeof(vdec_video_adjust))) return -1;
			vdec_video_set_brightness(v_adj.ch, v_adj.value, vdec_mode&1);
			break;
		case IOC_VDEC_SET_CONTRAST:
			if(copy_from_user(&v_adj, arg, sizeof(vdec_video_adjust))) return -1;
			vdec_video_set_contrast(v_adj.ch, v_adj.value, vdec_mode&1);
			break;
		case IOC_VDEC_SET_HUE:
			if(copy_from_user(&v_adj, arg, sizeof(vdec_video_adjust))) return -1;
			vdec_video_set_hue(v_adj.ch, v_adj.value, vdec_mode&1);
			break;
		case IOC_VDEC_SET_SATURATION:
			if(copy_from_user(&v_adj, arg, sizeof(vdec_video_adjust))) return -1;
			vdec_video_set_saturation(v_adj.ch, v_adj.value, vdec_mode&1);
			break;
		case IOC_VDEC_SET_SHARPNESS:
			break;
#endif 
		case IOC_VDEC_PTZ_PELCO_INIT:
			vdec_coaxial_init();
			pelco_coax_mode();
			break;
		case IOC_VDEC_PTZ_PELCO_RESET:
			pelco_reset();
			break;
		case IOC_VDEC_PTZ_PELCO_SET:
			pelco_set();
			break;
		case IOC_VDEC_PTZ_CHANNEL_SEL:
            if (copy_from_user(&ptz_ch, arg, sizeof(ptz_ch)))
			{
				return -1;
			}
			vdec_coaxial_select_ch(ptz_ch);
			break;
		case IOC_VDEC_PTZ_PELCO_UP:
		 	pelco_up();
		 	break;
		case IOC_VDEC_PTZ_PELCO_DOWN:
		 	pelco_down();
		 	break;
		case IOC_VDEC_PTZ_PELCO_LEFT:
		 	pelco_left();
		 	break;
		case IOC_VDEC_PTZ_PELCO_RIGHT:
			pelco_right();
			break;
		case IOC_VDEC_PTZ_PELCO_FOCUS_NEAR:
			//FIXME
			pelco_osd();
			//pelco_focus_near();
		 	break;
		case IOC_VDEC_PTZ_PELCO_FOCUS_FAR:
			//FIXME
			pelco_set();
			//pelco_focus_far();
		 	break;
		case IOC_VDEC_PTZ_PELCO_ZOOM_WIDE:
			//pelco_zoom_wide();
			pelco_iris_open();
		 	break;
		case IOC_VDEC_PTZ_PELCO_ZOOM_TELE:
			//pelco_zoom_tele();
			pelco_iris_close();
			break;

		case IOC_VDEC_INIT_MOTION:
			vdec_motion_init();
			hi3520_init_blank_data(0);
			break;
		case IOC_VDEC_ENABLE_MOTION:
			break;
		case IOC_VDEC_DISABLE_MOTION:
			break;
		case IOC_VDEC_SET_MOTION_AREA:
			break;
		case IOC_VDEC_GET_MOTION_INFO:
			vdec_get_motion_info(0);
			break;
		case IOC_VDEC_SET_MOTION_DISPLAY:
            if(copy_from_user(&on, arg, sizeof(unsigned int))) return -1;
			vdec_motion_display(0,on);
			break;
		case IOC_VDEC_SET_MOTION_SENS:
            if(copy_from_user(&sens, arg, sizeof(unsigned int)*16)) return -1;
			vdec_motion_sensitivity(sens);
			break;
		case IOC_VDEC_ENABLE_LOW_RES:
			vdec_low_resoultion_enable(0xff);
			break;
		case IOC_VDEC_DISABLE_LOW_RES:
			vdec_low_resoultion_disable(0xff);
			break;
		case IOC_VDEC_ENABLE_BW:
			vdec_bw_detection_enable(0xff);
			break;
		case IOC_VDEC_DISABLE_BW:
			vdec_bw_detection_disable(0xff);
			break;
		case IOC_VDEC_READ_BALCK_COUNT:
			value = vdec_bw_black_count_read(0);
			copy_to_user(arg,&value, sizeof(int));
			break;
		case IOC_VDEC_READ_WHITE_COUNT:
			break;
		case IOC_VDEC_4CH_VIDEO_SEQUENCE:
			break;


#if 0
        case IOC_VIDEO_GET_VIDEO_MODE:
		case IOC_VIDEO_SET_MOTION:
        case IOC_VIDEO_GET_MOTION:
		case IOC_VIDEO_SET_MOTION_EN:
		case IOC_VIDEO_SET_MOTION_SENS:
		case IOC_VIDEO_SET_MOTION_TRACE:
        case IOC_VIDEO_GET_VIDEO_LOSS:
		case IOC_VIDEO_GET_IMAGE_ADJUST:
        case IOC_AUDIO_SET_SAMPLE_RATE:
        case IOC_AUDIO_SET_AUDIO_PLAYBACK:
        case IOC_AUDIO_SET_AUDIO_DA_VOLUME:
		case IOC_AUDIO_SET_BRIGHTNESS:
		case IOC_AUDIO_SET_CONTRAST:
		case IOC_AUDIO_SET_HUE:
		case IOC_AUDIO_SET_SATURATION:
		case IOC_AUDIO_SET_SHARPNESS:
		case IOC_AUDIO_SET_AUDIO_MUTE:
		case IOC_AUDIO_SET_LIVE_CH:
		case IOC_AUDIO_SET_PB_CH:
#endif
		default:
            //printk("drv:invalid nc decoder ioctl cmd[%x]\n", cmd);
			break;
	}
	return 0;
}

static struct file_operations vdec_fops = {
	.owner      = THIS_MODULE,
    //.ioctl      = vdec_ioctl,
    .unlocked_ioctl	= vdec_ioctl,
	.open       = vdec_open,
	.release    = vdec_close
};

static struct miscdevice vdec_dev = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= "nc_vdec",
	.fops  		= &vdec_fops,
};

int chip_id[2];
static int __init vdec_module_init(void)
{
	int ret = 0, i = 0;
    unsigned char i2c_addr;

	unsigned char gpioDir = 0;
	unsigned int clksel = 0;
	
	clksel =  *(volatile unsigned int*)(IO_ADDRESS(0x20030000 +0x002c))  ;
	//printk("clksel is 0x%08x\n",clksel);
	clksel &= 0xfff3ffff;
	clksel |= 0x40000;
	//printk("clksel is 0x%08x\n",clksel);
	*(volatile unsigned int*)(IO_ADDRESS(0x20030000 +0x002c)) = clksel;
	clksel =  *(volatile unsigned int*)(IO_ADDRESS(0x20030000 +0x002c))  ;
	//printk("clksel is 0x%08x\n",clksel);
	clksel =  (*(volatile unsigned int*)(IO_ADDRESS(0x20030000 +0x002c)) >> 18 )& 0x3;
	//printk("clksel is 0x%08x\n",clksel);	
	msleep(100);
	gpioDir = *(volatile unsigned int*)(IO_ADDRESS(0x20160000 + 0x400));
	gpioDir |= (1<<2);
	*(volatile unsigned int*)(IO_ADDRESS(0x20160000 + 0x400)) = gpioDir;
	*(volatile unsigned int*)(IO_ADDRESS(0x20160000 + 0x10)) = (1<<2);
	ret = misc_register(&vdec_dev);

   	if (ret)
	{
		printk("ERROR: could not register NC Decoder devices:%#x \n",i);		
	}
	
	printk("NVP1918C Test Driver 2014.02.14\n");
#if 1
	for(i=0;i<2;i++)
	{
		chip_id[i] = check_id(vdec_slave_addr[i]);
		//if( (chip_id[i] != NVP1918C_R0_ID )||(chip_id[i] != NVP1914C_R0_ID) )
		if(chip_id[i] != 0x8200 )
		{
			printk("Blade1 Device ID Error... %x\n", chip_id[i]);
		}
		else
		{
			printk("Blade1 Device (0x%x) ID OK... %x\n", vdec_slave_addr[i], chip_id[i]);
			vdec_cnt++;
		}
	}
#endif	

#ifdef ONLY_720H
	printk("NVP1118B Count = %x\n", vdec_cnt);
	#ifdef NVC1700A_NVP1918C
	nvc1700_init(NVC1700_VM_PAL, NVC1700_OUTMODE_4D1_16CIF);
	#endif 

	#ifdef NVP1914C
	nvp1914c_h720_mode ( PAL );
	#else
	nvp1918c_720h_pal();
	//nvp1918c_720h_nt();
	#endif 
#else
	printk("NVP1918C Count = %x\n", vdec_cnt);
    #ifdef NVP1914C
	nvp1914c_h960_mode (PAL);
	#else
	//nvp1918c_960h_pal();
	nvp1918c_720h_pal();
	//nvp1918c_960h_nt();
	printk("\n nvp1918c_960h_nt\n");
	#endif	
#endif
	return 0;
}



static void __exit vdec_module_exit(void)
{
	misc_deregister(&vdec_dev);	
}

module_init(vdec_module_init);
module_exit(vdec_module_exit);

MODULE_LICENSE("GPL");

