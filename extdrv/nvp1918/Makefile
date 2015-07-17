#ifeq ($(KERNELRELEASE),)
#export MMZ_SRC_BASE=$(PWD)
#endif

ifeq ($(PARAM_FILE), )
		PARAM_FILE:=../../Makefile.param
		include $(PARAM_FILE)
endif
obj-m := nvp1918.o
nvp1918-objs := nvp1918_drv.o coax_protocol.o video.o motion.o audio.o

#release this to make a nvp1118b driver
#EXTRA_CFLAGS += -DNVP1118B

EXTRA_CFLAGS += -I$(PWD)/../gpio_i2c_8b

default:
	@make -C $(LINUX_ROOT) M=$(PWD) modules 
	#cp *.ko /nfs/hwarang/hi3531/ko/extdrv
clean: 
	@make -C $(LINUX_ROOT) M=$(PWD) clean 


