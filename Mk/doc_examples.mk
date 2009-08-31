$(info Reading doc_examples.mk)

OFLUX_DOCUMENTATION+= doc/examples/index.html


.SECONDEXPANSION:
doc/examples/index.html: doc/examples
	(cat $(SRCDIR)/doc/examples_header.html; \
	$(foreach P,$(OFLUX_PROJECTS),$(SRCDIR)/doc/example-doc.sh $(P) $($(P)_OFLUX_MAIN_TARGET) $($(P)_OFLUX_PATH);) \
	$(foreach P,$(OFLUX_PLUGIN_PROJECTS),$(SRCDIR)/doc/example-doc.sh $(P) $($(P)_OFLUX_MAIN:.flux=) $($(P)_OFLUX_PATH);) \
	cat $(SRCDIR)/doc/examples_footer.html) > $@
