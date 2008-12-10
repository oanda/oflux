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
        vers.cmx \
	debug.cmx \
	xml.cmx \
	cmdLine.cmx \
	parserTypes.cmx \
        parser.cmx \
	lexer.cmx \
	symbolTable.cmx \
	unify.cmx \
	cycleFind.cmx \
	unionFind.cmx \
	typeCheck.cmx \
	flatten.cmx \
	flow.cmx \
	generateXML.cmx \
	dot.cmx \
	codePrettyPrinter.cmx \
	generateCPP1.cmx \
	topLevel.cmx \
        main.cmx 

OBJS := $(HEADERS)
OBJS += $(CODE)

COMPILED_CODE = $(addprefix $(CURDIR)/,$(addsuffix .$(OBJECTEXT),$(basename $(CODE))))

oflux: vers.ml $(OBJS) oflux.cmxa 
	$(OCAMLCOMPILER) $(INCLUDEOPTS) unix.$(LIBRARYEXT) oflux.cmxa main.$(OBJECTEXT) -o oflux 


oflux.cmxa: 
	$(OCAMLCOMPILER) -a $(COMPILED_CODE) -o $@

.PHONY : vers.ml
vers.ml: 
	(echo "(* auto-generated - do not modify*)"; echo ""; echo "let vers= "; echo "\""`git describe --tags`"\""; echo "") > vers.ml

# Hack to work around ocamlyacc's limitation of putting results in the
# same dir as source.
$(CURDIR)/% : $(COMPONENT_DIR)/%
	cp "$<" "$@"

parser.mli: $(CURDIR)/parser.mly
	ocamlyacc -v $<

parser.ml: $(CURDIR)/parser.mli
	echo "parser.ml built from yacc."

lexer.ml: $(CURDIR)/lexer.mll
	ocamllex $<

