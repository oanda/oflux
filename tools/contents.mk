$(info Reading contents.mk $(COMPONENT_DIR))

TOOLS += oflux 

OFLUX_COMPONENT_DIR:=$(COMPONENT_DIR)


HEADERS:= \
	xml.cmi \
	hashString.cmi \
	docTag.cmi \
	cycleFind.cmi \
	unionFind.cmi \
	codePrettyPrinter.cmi \
	cmdLine.cmi \
	flatten.cmi \
	parserTypes.cmi \
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
	hashString.$(OBJECTEXT) \
	docTag.$(OBJECTEXT) \
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

OFLUXTOOLSRC:= $(CODE:%.$(OBJECTEXT)=%.ml) $(HEADERS:%.cmi=%.mli)
OFLUXOCAMLDEPENDS:= $(shell (test -r oflux.ocaml.depend || ((cd $(OFLUX_COMPONENT_DIR); $(OCAMLDEP) $(INCLUDEOPTS) $(OFLUXTOOLSRC); cd $(CURDIR)) > oflux.ocaml.depend)); echo oflux.ocaml.depend)

OBJS := $(HEADERS)
OBJS += $(CODE)

COMPILED_CODE = $(addprefix $(CURDIR)/,$(addsuffix .$(OBJECTEXT),$(basename $(CODE))))

oflux: vers.ml $(OBJS) oflux.$(LIBRARYEXT) 
	$(OCAMLCOMPILER) $(INCLUDEOPTS) oflux.$(LIBRARYEXT) main.$(OBJECTEXT) -o oflux 
#	$(OCAMLCOMPILER) $(INCLUDEOPTS) unix.$(LIBRARYEXT) oflux.$(LIBRARYEXT) main.$(OBJECTEXT) -o oflux 


oflux.$(LIBRARYEXT): $(CODE)
	$(OCAMLCOMPILER) -a $(COMPILED_CODE) -o $@

OFLUX_VERS_READ:=$(shell test -r $(CURDIR)/vers.ml && grep "^\"" $(CURDIR)/vers.ml | sed s/\"//g)
OFLUX_VERS_EXISTING:=$(shell cd $(OFLUX_COMPONENT_DIR); git describe --tags; cd $(CURDIR))

ifeq ($(OFLUX_VERS_READ),$(OFLUX_VERS_EXISTING))
  VERSDEPEND=
else
  VERSDEPEND=FORCE
endif

FORCE:

vers.ml: $(VERSDEPEND) 
	(echo "(* auto-generated - do not modify*)"; echo ""; echo "let vers= "; cd $(OFLUX_COMPONENT_DIR); echo "\""`git describe --tags`"\""; echo ""; cd $(CURDIR)) > vers.ml

# Hack to work around ocamlyacc's limitation of putting results in the
# same dir as source.
$(CURDIR)/% : $(OFLUX_COMPONENT_DIR)/%
	ln -sf "$<" "$@"

parser.mli parser.ml: $(CURDIR)/parser.mly
	ocamlyacc -v $<

lexer.ml: $(CURDIR)/lexer.mll
	ocamllex $<

doc/oflux-syntax.html: parser.ml grammardoc.sed grammardoc.sed
	mkdir -p $(dir $@); \
	(cat parser.output \
		| awk -f $(OFLUX_COMPONENT_DIR)/grammardoc.awk \
		| sed -f $(OFLUX_COMPONENT_DIR)/grammardoc.sed) > $@

doc/compiler: $(HEADERS)
	mkdir -p $@; \
	ocamldoc -html -I . -d doc/compiler \
		$(foreach f,$(filter-out main.cmi parser.cmi,$^), \
			$(OFLUX_COMPONENT_DIR)/$(f:.cmi=.mli))

OFLUX_DOCUMENTATION += doc/compiler doc/oflux-syntax.html

include $(OFLUXOCAMLDEPENDS)

# rules not discovered by ocamldep until ocamlyacc ocamllex run
parser.cmi: parserTypes.cmi
parser.cmo: parserTypes.cmi parser.cmi 
parser.cmx: parserTypes.cmx parser.cmi 
lexer.cmi: parserTypes.cmi
lexer.cmo: parserTypes.cmi parser.cmi 
lexer.cmx: parserTypes.cmx parser.cmx 
topLevel.cmx: parser.cmx lexer.cmx
topLevel.cmo: parser.cmi lexer.cmi

OFLUXGITTOP=`find ../.. | grep ofluxgittop | xargs dirname`
ofluxforbjam : oflux
	cp oflux ${OFLUXGITTOP}/ofluxforbjam

