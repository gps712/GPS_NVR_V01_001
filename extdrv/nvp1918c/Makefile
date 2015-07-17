#ifeq ($(KERNELRELEASE),)
#export MMZ_SRC_BASE=$(PWD)
#endif

ifeq ($(PARAM_FILE), )
		PARAM_FILE:=../../Makefile.param
		include $(PARAM_FILE)
endif
obj-m := nvp1918c_ex.o
nvp1918c_ex-objs := nvp1918c_drv.o coax_protocol.o video.o motion.o audio.o

#release this to use 108M 4D1 output
#EXTRA_CFLAGS += -DONLY_720H 

EXTRA_CFLAGS += -I$(PWD)/../gpio_i2c_8b

default:
	@make -C $(LINUX_ROOT) M=$(PWD) modules 
	#cp *.ko /nfs/hwarang/hi3531/ko/extdrv
clean: 
	@make -C $(LINUX_ROOT) M=$(PWD) clean 


