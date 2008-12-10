$(warning Reading concur1 contents.mk $(COMPONENT_DIR))

APPS += concur1
OFLUX_INCLUDES += $(COMPONENT_DIR)

OFLUX_FLOW := concur.flux
OBJS := $(GENERATED_OFLUX_OBJS) 
 
concur1: $(OBJS) 

#dependencies
-include $(OBJS:.o=.d)
