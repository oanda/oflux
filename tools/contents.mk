$(warning Reading contents.mk $(COMPONENT_DIR))

TOOLS += oflux 

HEADERS:= \
	xml.cmi \
	cycleFind.cmi \
	unionFind.cmi \
	codePrettyPrinter.cmi \
	cmdLine.cmi \
	parserTypes.cmi \
	flatten.cmi \
	parser.cmi \
	symbolTable.cmi \
	unify.cmi \
	typeCheck.cmi \
	flow.cmi \
	generateCPP1.cmi \
	generateXML.cmi \
	dot.cmi \
	main.cmi

CODE:= \
	vers.$(OBJECTEXT) \
	debug.$(OBJECTEXT) \
	xml.$(OBJECTEXT) \
	cmdLine.$(OBJECTEXT) \
	parserTypes.$(OBJECTEXT) \
	parser.$(OBJECTEXT) \
	lexer.$(OBJECTEXT) \
	symbolTable.$(OBJECTEXT) \
	unify.$(OBJECTEXT) \
	cycleFind.$(OBJECTEXT) \
	unionFind.$(OBJECTEXT) \
	typeCheck.$(OBJECTEXT) \
	flatten.$(OBJECTEXT) \
	flow.$(OBJECTEXT) \
	generateXML.$(OBJECTEXT) \
	dot.$(OBJECTEXT) \
	codePrettyPrinter.$(OBJECTEXT) \
	generateCPP1.$(OBJECTEXT) \
	topLevel.$(OBJECTEXT) \
	main.$(OBJECTEXT)

OBJS := $(HEADERS)
OBJS += $(CODE)

COMPILED_CODE = $(addprefix $(CURDIR)/,$(addsuffix .$(OBJECTEXT),$(basename $(CODE))))

oflux: vers.ml $(OBJS) oflux.$(LIBRARYEXT) 
	$(OCAMLCOMPILER) $(INCLUDEOPTS) unix.$(LIBRARYEXT) oflux.$(LIBRARYEXT) main.$(OBJECTEXT) -o oflux 


oflux.$(LIBRARYEXT): $(CODE)
	$(OCAMLCOMPILER) -a $(COMPILED_CODE) -o $@

ifeq ($(shell grep "^\"v" $(CURDIR)/vers.ml | sed s/\"//g), $(shell git describe --tags))
  VERSDEPEND=
else
  VERSDEPEND=FORCE
endif

FORCE:

vers.ml: $(VERSDEPEND) 
	(echo "(* auto-generated - do not modify*)"; echo ""; echo "let vers= "; echo "\""`git describe --tags`"\""; echo "") > vers.ml

# Hack to work around ocamlyacc's limitation of putting results in the
# same dir as source.
$(CURDIR)/% : $(COMPONENT_DIR)/%
	cp "$<" "$@"

parser.mli parser.ml: $(CURDIR)/parser.mly
	ocamlyacc -v $<

lexer.ml: $(CURDIR)/lexer.mll
	ocamllex $<
