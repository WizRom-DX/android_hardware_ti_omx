LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES:= \
	src/Resource_Activity_Monitor.c
	
LOCAL_C_INCLUDES += \
	$(TI_OMX_INCLUDES) \
	$(TI_BRIDGE_TOP)/api/inc \
	$(TI_OMX_SYSTEM)/resource_manager/resource_activity_monitor/inc	

LOCAL_SHARED_LIBRARIES := \
	libdl 	\
	libcutils \
	libbridge \
	libOMX_Core


LOCAL_CFLAGS := $(TI_OMX_CFLAGS)

ifeq ($(OMX_DEBUG), 1)
	LOCAL_CFLAGS += -DOMX_DEBUG=1
endif

LOCAL_MODULE:= libRAM

include $(BUILD_SHARED_LIBRARY)
