LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
	AacDecTest.c \

LOCAL_C_INCLUDES := \
	$(TI_OMX_SYSTEM)/common/inc \
	$(TI_OMX_COMP_C_INCLUDES) \
	$(TI_OMX_AUDIO)/aac_dec/inc

LOCAL_SHARED_LIBRARIES := $(TI_OMX_COMP_SHARED_LIBRARIES)


LOCAL_CFLAGS := $(TI_OMX_CFLAGS) -DOMX_DEBUG

LOCAL_MODULE:= AacDecTest

include $(BUILD_EXECUTABLE)
