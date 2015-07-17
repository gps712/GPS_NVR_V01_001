# Hisilicon Hi3516 sample Makefile
#include ../Makefile_My.param

########
# Hisilicon Hi3520D sample Makefile.param

ifeq ($(PARAM_FILE), )
     PARAM_FILE:=../../Makefile.param
     include $(PARAM_FILE)
endif
#增加的部分START1
include mk.param
#增加的部分END1

#demo or socket, just useful for hi3520D
HI3520D_BOARD=DEMO
#HI3520D_BOARD=SOCKET


COMMON_DIR := $(PWD)/../common_hi

INC_FLAGS := -I$(REL_INC) 
INC_FLAGS += -I$(COMMON_DIR) 
#增加的部分START2
INC_FLAGS += $(MY_APP_FLAGS)
#增加的部分END2
#INC_FLAGS += -I$(SDK_PATH)/mpp/$(EXTDRV)/tw2865 
#INC_FLAGS += -I$(SDK_PATH)/mpp/$(EXTDRV)/nvp1918c 
#INC_FLAGS += -I$(SDK_PATH)/mpp/$(EXTDRV)/tw2960 
#INC_FLAGS += -I$(SDK_PATH)/mpp/$(EXTDRV)/tlv320aic31 
#INC_FLAGS += -I$(SDK_PATH)/mpp/$(EXTDRV)/cx26828
INC_FLAGS += -I$(PWD)/../../extdrv/tw2865 
INC_FLAGS += -I$(PWD)/../../extdrv/nvp1918c 
INC_FLAGS += -I$(PWD)/../../extdrv/tw2960 
INC_FLAGS += -I$(PWD)/../../extdrv/tlv320aic31 
INC_FLAGS += -I$(PWD)/../../extdrv/cx26828

CFLAGS := -Wall -g $(INC_FLAGS) -D$(HIARCH) -DHICHIP=$(HICHIP) -D$(HIDBG) -D$(HI_FPGA) -D$(HI3520D_BOARD) -lpthread -lm

COMM_SRC := $(wildcard $(COMMON_DIR)/*.c)
COMM_OBJ := $(COMM_SRC:%.c=%.o)
#增加的部分START3
COMM_OBJ += $(MY_APP_OBJ)
COMM_OBJ += $(MY_CPP_OBJ)
#增加的部分END3

########

#ifeq ($(SAMPLE_PARAM_FILE), )
#     SAMPLE_PARAM_FILE:=../Makefile.param
#     include $(SAMPLE_PARAM_FILE)
#endif

# target source

SRC  := $(wildcard *.c)
OBJ  := $(SRC:%.c=%.o)

TARGET := $(OBJ:%.o=%)
.PHONY : clean all

CPP = arm-hisiv100nptl-linux-g++
all: $(TARGET)

MPI_LIBS := $(REL_LIB)/libmpi.a
MPI_LIBS += $(REL_LIB)/libhdmi.a
$(TARGET):%:%.o $(COMM_OBJ)
	$(CPP) $(CFLAGS) $(PARAM_FLAGS) -lpthread -lm -o $@ $^ $(MPI_LIBS) $(AUDIO_LIBA) $(JPEGD_LIBA) $(MY_APP_LIBA)

#$(MY_CPP_OBJ):%:%.o $(MY_CPP_SRC)
$(MY_CPP_OBJ):$(MY_CPP_SRC)
	arm-hisiv100nptl-linux-g++ $(CFLAGS) $(PARAM_FLAGS) -lpthread -lm -o $@ -c $^ $(MPI_LIBS) $(AUDIO_LIBA) $(JPEGD_LIBA) $(MY_APP_LIBA)

clean:
	@rm -f $(TARGET)
	@rm -f $(OBJ)
	@rm -f $(COMM_OBJ)

cleanstream:
	@rm -f *.h264
	@rm -f *.jpg
	@rm -f *.mjp
	@rm -f *.mp4
