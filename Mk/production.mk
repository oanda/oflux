$(warning Reading production.mk)

# Setting for production builds

ifeq ($(_PROC),i386)
	OPTIMIZATION_FLAGS := -O0
endif

