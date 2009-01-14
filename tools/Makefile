#
# make file for the OFlux tool
#

CODE= \
	vers.ml \
	debug.ml \
	xml.ml \
	cmdLine.ml \
	parserTypes.ml \
	parser.ml \
	lexer.ml \
	symbolTable.ml \
	unify.ml \
	cycleFind.ml \
	unionFind.ml \
	typeCheck.ml \
	flatten.ml \
	flow.ml \
	generateXML.ml \
	dot.ml \
	codePrettyPrinter.ml \
	generateCPP1.ml \
	topLevel.ml 

HEADERS= \
	xml.mli \
	cycleFind.mli \
	unionFind.mli \
	codePrettyPrinter.mli \
	cmdLine.mli \
	parserTypes.mli \
	flatten.mli \
	parser.mli \
	symbolTable.mli \
	unify.mli \
	typeCheck.mli \
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

ifeq ($(shell test -e vers.ml && grep "^\"v" vers.ml | sed s/\"//g), $(shell git describe --tags))
  VERSDEPEND=
else
  VERSDEPEND=FORCE
endif

BINDIR= bin

COMPILED_CODE = $(addprefix $(BINDIR)/,$(addsuffix .$(OBJECTEXT),$(basename $(CODE))))
COMPILED_HEADERS = $(addprefix $(BINDIR)/,$(addsuffix .cmi,$(basename $(HEADERS))))

COMPILED_MAIN = $(BINDIR)/main.cmi $(BINDIR)/main.$(OBJECTEXT)

#XMLLIGHTLIB=xml-light/bin/xml-light.$(LIBRARYEXT)

INCLUDEOPTS= -I $(BINDIR) #-I xml-light/bin

OFLUXLIB=$(BINDIR)/oflux.$(LIBRARYEXT)
OFLUXTOPLEVEL=$(BINDIR)/ofluxcli
OFLUX=$(BINDIR)/oflux

all: $(XMLLIGHTLIB) $(BINDIR)/.depend.bin $(OFLUXLIB) $(if $(WILLMAKETOPLEVEL),$(OFLUXTOPLEVEL),) $(OFLUX)

$(OFLUXTOPLEVEL): $(OFLUXLIB) 
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	ocamlmktop $(INCLUDEOPTS) unix.$(LIBRARYEXT) $(OFLUXLIB) -o $(OFLUXTOPLEVEL)

$(OFLUX): $(OFLUXLIB) $(COMPILED_MAIN)
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	$(OCAMLCOMPILER) $(INCLUDEOPTS) unix.$(LIBRARYEXT) $(OFLUXLIB) $(BINDIR)/main.$(OBJECTEXT) -o $(OFLUX)


vers.ml: $(VERSDEPEND)
	(echo "(* auto-generated - do not modify*)"; echo ""; echo "let vers= "; echo "\""`git describe --tags`"\""; echo "") > vers.ml

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
#	make -wC xml-light depend

$(BINDIR)/.depend.bin: $(BINDIR)/.depend
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	sed -e 's/[_a-zA-Z0-9]*\.cm[oaix]/bin\/&/g' < $(BINDIR)/.depend > $(BINDIR)/.depend.bin

$(BINDIR)/.depend:
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
#	@[ -d $(XMLLIGHTBIN) ] || mkdir -p $(XMLLIGHTBIN)
	ocamldep $(INCLUDEOPTS) *.mli *.ml > $(BINDIR)/.depend

clean:
	rm -f $(BINDIR)/* lexer.ml parser.ml parser.mli oflux ofluxcli parser.output $(BINDIR)/.depend* *.cmi *.cmo *.cmx ../doc/oflux-syntax.html ../doc/compiler/* vers.ml;
#	make -wC xml-light clean

doc:
	(cat parser.output | awk -f grammardoc.awk | sed -f grammardoc.sed) > ../doc/oflux-syntax.html
	mkdir -p ../doc/compiler
	ocamldoc -html -I bin -d ../doc/compiler $(HEADERS)
#	ocamldoc -html -I bin -I xml-light/bin -d ../doc/compiler $(HEADERS)

FORCE:

ifneq ($(MAKECMDGOALS),depend)
 -include $(BINDIR)/.depend.bin
endif

