#APP_STL:= stlport_static
#include $(ANDROID_NDK_ROOT)\sources\cxx-stl\stlport\stlport

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	systemmonitor.cpp 
	#algorithm.cpp \
	#tools.cpp

LOCAL_SHARED_LIBRARIES := \
	libsurfaceflinger \
	libbinder \
	libutils \
	libcutils
	
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../services/surfaceflinger

LOCAL_MODULE:= systemmonitor
#LOCAL_MODULE:= ref_c

include $(BUILD_EXECUTABLE)

