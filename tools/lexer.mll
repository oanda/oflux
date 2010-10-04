{ (* code here *)

open Parser 
open ParserTypes

exception Eof

let initial_position fno = { file= fno; lineno = 1; characterno = 0 }

let thePosition = ref (initial_position None)

let theLastPosition = ref (initial_position None)

let theFileStack = ref []

let reset () =
	begin
	thePosition := initial_position None;
	theLastPosition := initial_position None
	end

let updateNewLine () =
	begin
	theLastPosition := !thePosition;
	thePosition :=
		{ file = (!thePosition).file
		; lineno = (!thePosition).lineno + 1
		; characterno = 0 }
	end

let updateIntoFile fn =
	begin
	theFileStack := ((fn,!thePosition)::(!theFileStack));
	theLastPosition := !thePosition;
	thePosition := 
		{ file = Some fn
		; lineno = 0
		; characterno = 0 };
	end

let updateOutOfFile () =
	let _,pos = List.hd (!theFileStack)
	in  begin
	    theFileStack := List.tl (!theFileStack);
	    theLastPosition := pos;
	    thePosition := pos;
	    end

let moveovertoken lexbuf =
        let length = (Lexing.lexeme_end lexbuf)
                        -(Lexing.lexeme_start lexbuf)+1 in
        let _ = theLastPosition := !thePosition in
        let _ = thePosition :=
                        { file = (!thePosition).file
			; lineno = (!thePosition).lineno
			; characterno = (!thePosition).characterno+length
			(*;
                          currline = ((!thePosition).currline)^" "^(Lexing.lexeme lexbuf) *) 
			}
        in  ()

let updatePos lexbuf tok =
        let _ = moveovertoken lexbuf
        in tok

let getPos() = !thePosition
let getLastPos() = !theLastPosition
let getFile () = (!thePosition).file
let getLineNo () = (!thePosition).lineno
let getCharNo () = (!thePosition).characterno
let getLastFile () = (!theLastPosition).file
let getLastLineNo () = (!theLastPosition).lineno
let getLastCharNo () = (!theLastPosition).characterno
(*let getCurrLine () = (!thePosition).currline*)
(*let getCurrLine () = (!thePosition).currline*)

let updatePosInTok lexbuf tokfun =
        let _ = moveovertoken lexbuf in
        let startpos =
                { file = getLastFile ()
		; lineno = getLastLineNo ()
                ; characterno = getLastCharNo ()
                } in
        let endpos =
                { file = getFile ()
		; lineno = getLineNo ()
                ; characterno = getCharNo ()
                }
        in  tokfun (startpos,endpos)

(* some documentation tags for within comments 
   Comments need be /**  */ enclosed
   @node description of the node
   @input input-name description of the input 
   @output output-name description of the output
   @guard guard-name description of the guard (why it is needed)
*)

type tag_type = ANothing |ANode | AOutput | AInput | AGuard

type doctag_context =
        { mutable ndl : DocTag.node_doc_list
        ; mutable n : DocTag.node_doc option
        ; mutable i : string DocTag.doc option
        ; mutable o : string DocTag.doc option
        ; mutable g : string DocTag.doc option
        ; mutable tt : tag_type
        }


let theDocContext =
        { ndl = DocTag.empty
        ; n = None
        ; i = None
        ; o = None
        ; g = None
        ; tt = ANothing
        }

let getNodeDocList () = theDocContext.ndl

let dt_close () = 
        try let n = match theDocContext.n with
                        None -> raise Not_found
                        |(Some n) -> n  in
            let res = 
                match theDocContext.tt with
                         ANothing -> (Debug.dprint_string "nothing to close\n")
                        |ANode ->
                                begin
                                theDocContext.ndl <- (n::(theDocContext.ndl));
                                (*theDocContext.n <- None*)
                                end
                        |AInput ->
                                (match theDocContext.i with
                                        None -> (Debug.dprint_string "no input\n")
                                        | (Some id) -> 
                                                begin
                                                DocTag.add_input n id;
                                                theDocContext.i <- None
                                                end)
                        |AOutput ->
                                (match theDocContext.o with
                                        None -> (Debug.dprint_string "no output\n")
                                        | (Some od) -> 
                                                begin
                                                DocTag.add_output n od;
                                                theDocContext.o <- None
                                                end)
                        |AGuard ->
                                (match theDocContext.g with
                                        None -> (Debug.dprint_string "no guard\n")
                                        | (Some gd) -> 
                                                begin
                                                DocTag.add_guard n gd;
                                                theDocContext.o <- None
                                                end)
                in  (theDocContext.tt <- ANothing); res
        with Not_found -> Debug.dprint_string "no current node"


let dt_open_node s =
        begin
        dt_close(); 
        theDocContext.tt <- ANode;
        theDocContext.n <- Some (DocTag.new_node s)
        end
let dt_open_input s =
        begin
        dt_close(); 
        theDocContext.tt <- AInput;
        theDocContext.i <- Some (DocTag.new_item s)
        end
let dt_open_output s =
        begin
        dt_close(); 
        theDocContext.tt <- AOutput;
        theDocContext.o <- Some (DocTag.new_item s)
        end
let dt_open_guard s =
        begin
        dt_close(); 
        theDocContext.tt <- AGuard;
        theDocContext.g <- Some (DocTag.new_item s)
        end

let dt_add_content s =
        match theDocContext.tt with
                ANothing -> Debug.dprint_string "added content to nothing\n"
                | ANode -> 
                        (match theDocContext.n with
                                None -> Debug.dprint_string "cannot add content to undef node\n"
                                | (Some nd) -> DocTag.add_description nd s)
                | AInput ->
                        (match theDocContext.i with
                                None -> Debug.dprint_string "cannot add content to undef input\n"
                                | (Some ad) -> DocTag.add_content_string ad s)
                | AOutput ->
                        (match theDocContext.o with
                                None -> Debug.dprint_string "cannot add content to undef output\n"
                                | (Some ad) -> DocTag.add_content_string ad s)
                | AGuard ->
                        (match theDocContext.g with
                                None -> Debug.dprint_string "cannot add content to undef guard\n"
                                | (Some ad) -> DocTag.add_content_string ad s)

}

rule token = parse
	[ ' ' '\t' ] { (token lexbuf) }
	| ['\n'] { updateNewLine(); token lexbuf }
	| ( '0' | ( ['1' - '9'] (['0' - '9'])* ) )
			{ updatePosInTok lexbuf
				   (fun (x1,x2) -> NUMBER
					 (int_of_string (Lexing.lexeme lexbuf),x1,x2)) }
	| "=>" { updatePosInTok lexbuf (fun x -> ARROW x) }
	| "<=" { updatePosInTok lexbuf (fun x -> BACKARROW x) }
	| "!=" { updatePosInTok lexbuf (fun x -> NOTEQUALS x) }
	| "==" { updatePosInTok lexbuf (fun x -> ISEQUALS x) }
	| "&&" { updatePosInTok lexbuf (fun x -> DOUBLEAMPERSAND x) }
	| "||" { updatePosInTok lexbuf (fun x -> DOUBLEBAR x) }
	| '(' { updatePosInTok lexbuf (fun x -> LEFT_PAREN x) }
	| ')' { updatePosInTok lexbuf (fun x -> RIGHT_PAREN x) }
	| '[' { updatePosInTok lexbuf (fun x -> LEFT_SQ_BRACE x) }
	| ']' { updatePosInTok lexbuf (fun x -> RIGHT_SQ_BRACE x) }
	| '{' { updatePosInTok lexbuf (fun x -> LEFT_CR_BRACE x) }
	| '}' { updatePosInTok lexbuf (fun x -> RIGHT_CR_BRACE x) }
	| "->" { updatePosInTok lexbuf (fun x -> PIPE x) }
	| "-" { updatePosInTok lexbuf (fun x -> MINUS x) }
	| "::" { updatePosInTok lexbuf (fun x -> DOUBLECOLON x) }
	| ':' { updatePosInTok lexbuf (fun x -> COLON x) }
	| ';' { updatePosInTok lexbuf (fun x -> SEMI x) }
	| ',' { updatePosInTok lexbuf (fun x -> COMMA x) }
	| '=' { updatePosInTok lexbuf (fun x -> EQUALS x) }
	| '*' { updatePosInTok lexbuf (fun x -> STAR x) }
	| '+' { updatePosInTok lexbuf (fun x -> PLUS x) }
	| '?' { updatePosInTok lexbuf (fun x -> QUESTION x) }
	| '!' { updatePosInTok lexbuf (fun x -> EXCLAMATION x) }
	| '>' { updatePosInTok lexbuf (fun x -> GREATERTHAN x) }
	| '<' { updatePosInTok lexbuf (fun x -> LESSTHAN x) }
	| "&=" { updatePosInTok lexbuf (fun x -> AMPERSANDEQUALS x) }
	| '&' { updatePosInTok lexbuf (fun x -> AMPERSAND x) }
	| "..." { updatePosInTok lexbuf (fun x -> ELLIPSIS x) }
	| "as" { updatePosInTok lexbuf (fun x -> AS x) }
	| "where" { updatePosInTok lexbuf (fun x -> WHERE x) }
	| "bool" { updatePosInTok lexbuf (fun x -> BOOL x) }
	| "const" { updatePosInTok lexbuf (fun x -> CONST x) }
	| "guard" { updatePosInTok lexbuf (fun x -> GUARD x) }
        | "unordered" { updatePosInTok lexbuf (fun x -> UNORDERED x) }
        | "gc" { updatePosInTok lexbuf (fun x -> GC x) }
	| "exclusive" { updatePosInTok lexbuf (fun x -> EXCLUSIVE x) }
	| "readwrite" { updatePosInTok lexbuf (fun x -> READWRITE x) }
	| "free" { updatePosInTok lexbuf (fun x -> FREE x) }
	| "depends" { updatePosInTok lexbuf (fun x -> DEPENDS x) }
	| "sequence" { updatePosInTok lexbuf (fun x -> SEQUENCE x) }
	| "pool" { updatePosInTok lexbuf (fun x -> POOL x) }
	| "condition" { updatePosInTok lexbuf (fun x -> CONDITION x) }
	| "node" { updatePosInTok lexbuf (fun x -> NODE x) }
	| "detached" { updatePosInTok lexbuf (fun x -> DETACHED x) }
	| "typedef" { updatePosInTok lexbuf (fun x -> TYPEDEF x) }
	| "source" { updatePosInTok lexbuf (fun x -> SOURCE x) }
	| "terminate" { updatePosInTok lexbuf (fun x -> TERMINATE x) }
	| "initial" { updatePosInTok lexbuf (fun x -> INITIAL x) }
	| "handle" { updatePosInTok lexbuf (fun x -> HANDLE x) }
	| "error" { updatePosInTok lexbuf (fun x -> ERROR x) }
	| "atomic" { updatePosInTok lexbuf (fun x -> ATOMIC x) }
	| "abstract" { updatePosInTok lexbuf (fun x -> ABSTRACT x) }
	| "mutable" { updatePosInTok lexbuf (fun x -> MUTABLE x) }
	| "static" { updatePosInTok lexbuf (fun x -> STATIC x) }
	| "instance" { updatePosInTok lexbuf (fun x -> INSTANCE x) }
	| "module" { updatePosInTok lexbuf (fun x -> MODULE x) }
	| "plugin" { updatePosInTok lexbuf (fun x -> PLUGIN x) }
	| "external" { updatePosInTok lexbuf (fun x -> EXTERNAL x) }
	| "begin" { updatePosInTok lexbuf (fun x -> BEGIN x) }
	| "end" { updatePosInTok lexbuf (fun x -> END x) }
	| "read" { updatePosInTok lexbuf (fun x -> READ x) }
	| "upgradeable" { updatePosInTok lexbuf (fun x -> UPGRADEABLE x) }
	| "nullok" { updatePosInTok lexbuf (fun x -> NULLOK x) }
	| "if" { updatePosInTok lexbuf (fun x -> IF x) }
	| "precedence" { updatePosInTok lexbuf (fun x -> PRECEDENCE x) }
	| "write" { updatePosInTok lexbuf (fun x -> WRITE x) }
	| "include" { let fstr = incl lexbuf
			in INCLUDE fstr }
	| ( ( ['a' - 'z'] | ['A' - 'Z'] | '_' )( '.' | ['a' - 'z'] | ['A' - 'Z'] | '_' | ['0' - '9'] )* )
			{ updatePosInTok lexbuf
				(fun (x1,x2) -> IDENTIFIER (Lexing.lexeme lexbuf,x1,x2)) }
	| eof                   { updatePos lexbuf ENDOFFILE (*; raise Eof*) }
	| "/**" { doccomment lexbuf; token lexbuf }
	| "/*" { comment lexbuf; token lexbuf }
	| '/' { updatePosInTok lexbuf (fun x -> SLASH x) }
and doccomment = parse
	"*/"                    { dt_close(); () }
	| "*"                   { doccomment lexbuf }
	| [ '\n' ]              { updateNewLine(); doccomment lexbuf } (* should count these *)
        | "@node"               { let astr = docelementname lexbuf in dt_open_node astr; doccomment lexbuf }
        | "@input"              { let astr = docelementname lexbuf in dt_open_input astr; doccomment lexbuf  }
        | "@output"             { let astr = docelementname lexbuf in dt_open_output astr; doccomment lexbuf  }
        | "@guard"              { let astr = docelementname lexbuf in dt_open_guard astr; doccomment lexbuf  }
	| ( ['a' - 'z'] | ['A' - 'Z'] | ['0' - '9'] | '_' | '.' )+
				{ dt_add_content (Lexing.lexeme lexbuf); doccomment lexbuf }
	| _                     { dt_add_content (Lexing.lexeme lexbuf); doccomment lexbuf }
and docelementname = parse
	[ ' ' '\t' ]            { incl lexbuf }
	(*| ( ['a' - 'z'] | ['A' - 'Z'] | ['0' - '9'] | '_' | '.' )+
				{ Lexing.lexeme lexbuf }*)
	| _                     { Lexing.lexeme lexbuf }
and comment = parse
	"*/"                    { () }
	| [ '\n' ]              { updateNewLine(); comment lexbuf } (* should count these *)
	| _                     { comment lexbuf }
and incl = parse
	[ ' ' '\t' ]            { incl lexbuf }
	| ( ['a' - 'z'] | ['A' - 'Z'] | ['0' - '9'] | '_' | '.' )+
				{ Lexing.lexeme lexbuf }
	| _                     { raise Eof }


