$(info Reading production.mk)

# Setting for production builds
# This decision is meant to be _only_ local to oflux

OPTIMIZATION_FLAGS?=-O0 # can override on make invocation
_OPTIMIZATION_FLAGS=$(OPTIMIZATION_FLAGS)

