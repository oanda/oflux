$(info Reading doc_examples.mk)

OFLUX_DOCUMENTATION+= doc/examples/index.html


.SECONDEXPANSION:
doc/examples/index.html: doc/examples
	(cat $(OFLUX_DOC_COMPONENT_DIR)/examples_header.html; \
	$(foreach P,$(OFLUX_PROJECTS),$(OFLUX_DOC_COMPONENT_DIR)/example-doc.sh $(P) $($(P)_OFLUX_MAIN_TARGET) $($(P)_OFLUX_PATH);) \
	$(foreach P,$(OFLUX_PLUGIN_PROJECTS),$(OFLUX_DOC_COMPONENT_DIR)/example-doc.sh $(P) $($(P)_OFLUX_MAIN:.flux=) $($(P)_OFLUX_PATH);) \
	cat $(OFLUX_DOC_COMPONENT_DIR)/examples_footer.html) > $@
