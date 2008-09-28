#
# make file for the OFlux tool
#

CODE= \
	debug.ml \
	cmdLine.ml \
	parserTypes.ml \
	parser.ml \
	lexer.ml \
	symbolTable.ml \
	flatten.ml \
	unionFind.ml \
	flow.ml \
	generateXML.ml \
	dot.ml \
	codePrettyPrinter.ml \
	generateCPP1.ml \
	topLevel.ml 

HEADERS= \
	unionFind.mli \
	codePrettyPrinter.mli \
	cmdLine.mli \
	parserTypes.mli \
	flatten.mli \
	parser.mli \
	symbolTable.mli \
	flow.mli \
	generateCPP1.mli \
	generateXML.mli \
	dot.mli

ifneq ($(findstring dev,$(OCAMLCONFIG)),dev)
  OCAMLCOMPILER=ocamlopt $(if $(findstring profile,$(OCAMLCONFIG)),-p,)
  OBJECTEXT=cmx
  LIBRARYEXT=cmxa
else
  OCAMLCOMPILER=$(if $(findstring profile,$(OCAMLCONFIG)),ocamlcp -p a,ocamlc -g)
  OBJECTEXT=cmo
  LIBRARYEXT=cma
  WILLMAKETOPLEVEL=yes
endif

BINDIR= bin

COMPILED_CODE = $(addprefix $(BINDIR)/,$(addsuffix .$(OBJECTEXT),$(basename $(CODE))))
COMPILED_HEADERS = $(addprefix $(BINDIR)/,$(addsuffix .cmi,$(basename $(HEADERS))))

COMPILED_MAIN = $(BINDIR)/main.cmi $(BINDIR)/main.$(OBJECTEXT)

XMLLIGHTLIB=xml-light/bin/xml-light.$(LIBRARYEXT)

INCLUDEOPTS= -I $(BINDIR) -I xml-light/bin

OFLUXLIB=$(BINDIR)/oflux.$(LIBRARYEXT)
OFLUXTOPLEVEL=$(BINDIR)/ofluxcli
OFLUX=$(BINDIR)/oflux

all: test $(XMLLIGHTLIB) $(BINDIR)/.depend.bin $(OFLUXLIB) $(if $(WILLMAKETOPLEVEL),$(OFLUXTOPLEVEL),) $(OFLUX)

test:
	@echo $(COMPILED_HEADERS) $(COMPILED_CODE)

$(OFLUXTOPLEVEL): $(OFLUXLIB) $(XMLLIGHTLIB)
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	ocamlmktop $(INCLUDEOPTS) unix.$(LIBRARYEXT) $(XMLLIGHTLIB) $(OFLUXLIB) -o $(OFLUXTOPLEVEL)

$(OFLUX): $(OFLUXLIB) $(XMLLIGHTLIB) $(COMPILED_MAIN)
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	$(OCAMLCOMPILER) $(INCLUDEOPTS) unix.$(LIBRARYEXT) $(XMLLIGHTLIB) $(OFLUXLIB) $(BINDIR)/main.$(OBJECTEXT) -o $(OFLUX)


$(XMLLIGHTLIB):
	make -wC xml-light bin/xml-light.$(LIBRARYEXT)

$(OFLUXLIB): $(COMPILED_HEADERS) $(COMPILED_CODE)
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	$(OCAMLCOMPILER) -a $(COMPILED_CODE) -o $@

lexer.ml: lexer.mll
	ocamllex lexer.mll

parser.mli: parser.mly
	ocamlyacc -v parser.mly

parser.ml: parser.mli
	echo "parser.ml built from yacc."

#.SUFFIXES: .ml .mli .cmo .cmi

# %.ml: %.mll
# 	if [ `df -TP $< | egrep -v "^Filesystem" | cut -d " " -f 2` = "nfs" ] ; then \
# 		# run ocamllex in /tmp, a non-NFS directory \
# 		( cp $< /tmp ; cd /tmp; ocamllex $< ) ; cp /tmp/$@ . ; \
# 	else \
# 		ocamllex $< ; \
# 	fi

$(BINDIR)/%.$(OBJECTEXT): %.ml
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	$(OCAMLCOMPILER) $(INCLUDEOPTS) -o $@ -c $<

$(BINDIR)/%.cmi: %.mli
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	$(OCAMLCOMPILER) $(INCLUDEOPTS) -o $@ -c $<

depend: $(BINDIR)/.depend.bin
	make -wC xml-light depend

$(BINDIR)/.depend.bin: $(BINDIR)/.depend
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	sed -e 's/[_a-zA-Z0-9]*\.cm[oaix]/bin\/&/g' < $(BINDIR)/.depend > $(BINDIR)/.depend.bin

$(BINDIR)/.depend:
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	ocamldep $(INCLUDEOPTS) *.mli *.ml > $(BINDIR)/.depend

clean:
	rm -f $(BINDIR)/* lexer.ml parser.ml parser.mli oflux ofluxcli parser.output $(BINDIR)/.depend* *.cmi *.cmo *.cmx ../doc/oflux-syntax.html ../doc/compiler/*;
	make -wC xml-light clean

doc:
	(cat parser.output | awk -f grammardoc.awk | sed -f grammardoc.sed) > ../doc/oflux-syntax.html
	mkdir -p ../doc/compiler
	ocamldoc -html -I bin -I xml-light/bin -d ../doc/compiler $(HEADERS)


ifneq ($(MAKECMDGOALS),depend)
 -include $(BINDIR)/.depend.bin
endif

