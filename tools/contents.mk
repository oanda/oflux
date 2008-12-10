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

parser.mli: parser.mly
	ocamlyacc -v $<
	mv $(COMPONENT_DIR)/parser.mli $(CURDIR)

parser.ml: parser.mli
	echo "parser.ml built from yacc."
	cp $(COMPONENT_DIR)/parser.ml $(CURDIR)

lexer.ml: lexer.mll
	ocamllex $(SRCDIR)/tools/lexer.mll
	mv $(COMPONENT_DIR)/lexer.ml $(CURDIR)


