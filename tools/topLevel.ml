open ParserTypes

exception SemanticFailure of string * ParserTypes.position

(* done in 3 passes *)

let open_file_from_include_path fn =
	let rec offip ip =
		match ip with
			(hp::tp) ->
				(try let fullfile = hp^"/"^fn
                                     in  (open_in fullfile), fullfile
				with (Sys_error _) -> offip tp)
			| [] -> raise (SemanticFailure ("file "^fn^" not found",
				ParserTypes.noposition))
	in  offip ("."::(CmdLine.get_include_path()))

(*PASS 1*)

(* The Lexstack type. *)
type 'a t =
{   mutable stack : (string * in_channel * Lexing.lexbuf) list ;
    mutable filename : string ;
    mutable filenamelist : string list ;
    mutable chan : in_channel ;
    mutable lexbuf : Lexing.lexbuf ;
    lexfunc : Lexing.lexbuf -> 'a ;
    }

(*
** Create a lexstack with an initial top level filename and the
** lexer function.
*)
let ls_create top_filename lexer_function =
    let chan,_ = open_file_from_include_path top_filename in
        { stack = [] 
	; filename = top_filename 
	; filenamelist = [top_filename]
	; chan = chan 
	; lexbuf = Lexing.from_channel chan 
	; lexfunc = lexer_function
        }

(*
** The the next token. Need to accept an unused dummy lexbuf so that
** a closure consisting of the function and a lexstack can be passed
** to the ocamlyacc generated parser.
*)
let rec get_token ls dummy_lexbuf =
    match ls.lexfunc ls.lexbuf with
    |    Parser.INCLUDE fname ->
	    (*let _ = if List.mem fname ls.filenamelist 
		    then print_string ("double include of "^fname^" detected\n")
		    else ()
	    in*)
	    if List.mem fname ls.filenamelist then
		()
	    else 
                (let cha,full_fn = open_file_from_include_path fname in
                let _ = Debug.dprint_string (" including "^full_fn^"\n")
                in
		begin
		Lexer.updateIntoFile full_fn;
                ls.stack <- (ls.filename, ls.chan, ls.lexbuf) :: ls.stack ;
                ls.filename <- full_fn ;
                ls.filenamelist <- full_fn::(ls.filenamelist) ;
                ls.chan <- cha;
                ls.lexbuf <- Lexing.from_channel ls.chan ;
		end);
            get_token ls dummy_lexbuf
    |    Parser.ENDOFFILE ->
            (   match ls.stack with
                |    [] -> Parser.ENDOFFILE
                |    (fn, ch, lb) :: tail ->
			Lexer.updateOutOfFile ();
                        ls.filename <- fn ;
			close_in ls.chan ;
                        ls.chan <- ch ;
                        ls.stack <- tail ;
			ls.lexbuf <- lb ;
                        get_token ls dummy_lexbuf
                )
    |    anything -> anything


(* Get the current lexeme. *)
let lexeme ls =
    Lexing.lexeme ls.lexbuf

(* Get filename, line number and column number of current lexeme. *)
let current_pos ls =
    let pos = Lexing.lexeme_end_p ls.lexbuf in
    let linepos = pos.Lexing.pos_cnum - pos.Lexing.pos_bol -
        String.length (Lexing.lexeme ls.lexbuf)
    in  { file=Some ls.filename; lineno=pos.Lexing.pos_lnum; characterno=linepos }



let parsefile fl =
	let _ = Lexer.reset() in
	let lexstack = ls_create fl Lexer.token in
	let dummy_lexbuf = Lexing.from_string "" in
	let res =
	    try Some (Parser.top_level_program (get_token lexstack) dummy_lexbuf)
	    with (Parsing.Parse_error| (Failure _)) ->
		let pos = (*current_pos lexstack*) Lexer.getPos()
		in  (print_string (fl^" Syntax Error "^(position_to_string pos)^"\n"); None)
	in  res


(*PASS 2*)
let semantic_analysis p mod_defs =
	let m_fns = p.mainfun_list in
	let node_decls = p.node_decl_list in
	let exprs = p.expr_list in
	let errs = p.err_list in
        let terms = List.map strip_position p.terminate_list in
	let symboltable = SymbolTable.empty_symbol_table in
	let symboltable = SymbolTable.add_program symboltable p in
	let builtflow = Flow.build_flow_map symboltable
		node_decls m_fns exprs errs terms mod_defs
	in  builtflow
	
(*PASS 3*)
let generate fn br =
	let debug = !Debug.debug in
	let _ = Debug.dprint_string "FINAL FMAP\n"; Flow.pp_flow_map debug br.Flow.fmap in
	let h_code,cpp_code,conseq_res = GenerateCPP1.emit_cpp (CmdLine.get_module_name()) br in
	let xml = match CmdLine.get_module_name() with
                None -> GenerateXML.emit_xml fn br conseq_res
                | _ -> raise (SemanticFailure ("no xml for modules",noposition))
	in  xml,cpp_code,h_code



(*MAIN*)
let smain fn = 
	match parsefile fn with
		None -> (print_string "no parse result\n"; None)
		| (Some pres) -> Some (semantic_analysis pres pres.mod_def_list)

let print_result _ (xml,h_code,cpp_code) =
	print_string (Xml.to_string_fmt xml); 
	print_string (CodePrettyPrinter.to_string h_code);
	print_string (CodePrettyPrinter.to_string cpp_code)

type oflux_result =
	{ xml : Xml.xml
	; h_code : CodePrettyPrinter.code
	; cpp_code : CodePrettyPrinter.code
	; dot_output : Dot.dot
	; dot_flat_output : Dot.dot option
	; h_filename : string
	; cpp_filename : string
	}

let write_result fn of_result =
	let is_module = 
		match CmdLine.get_module_name () with
			None -> false
			| _ -> true
		in
	let base_file = 
		try let ri = String.rindex fn '.'
		    in  String.sub fn 0 ri
		with Not_found -> fn in
	let xml_file = base_file^".xml" in
	let dot_file = base_file^".dot" in
	let dot_flat_file = base_file^"-flat.dot" in  
        let write_timer = Debug.timer "writing" in
        let result =
	    begin
	    (let h_timer = Debug.timer "write .h" in
             let h_outchan = open_out of_result.h_filename in
	     let h_lines =CodePrettyPrinter.to_string of_result.h_code 
		(*in
	     let _ = Debug.dprint_string ("wrote h:\n"^h_lines) *)
	     in  output_string h_outchan h_lines; 
                 close_out h_outchan;
                 h_timer ()); 
            (let cpp_timer = Debug.timer "write .cpp" in 
             let cpp_outchan = open_out of_result.cpp_filename in
	     let cpp_lines = CodePrettyPrinter.to_string of_result.cpp_code
	     in output_string cpp_outchan cpp_lines; 
                close_out cpp_outchan;
                cpp_timer ()); 
            (let dot_timer = Debug.timer "write .dot" in
             let dot_outchan = open_out dot_file 
		 in output_string dot_outchan (Dot.to_string of_result.dot_output); 
                    close_out dot_outchan;
                    dot_timer ());
	    if is_module then ()
	    else
		begin
		(let xml_timer = Debug.timer "write .xml" in
                 let xml_outchan = open_out xml_file
		 in output_string xml_outchan (Xml.to_string_fmt of_result.xml);
                    close_out xml_outchan;
                    xml_timer ());
                (let dot_flat_timer = Debug.timer "write -flat.dot" in
                 let dot_flat_outchan = open_out dot_flat_file in
                 let df_out = match  of_result.dot_flat_output with
                                (Some op) -> op
                                | None -> raise Not_found
                 in output_string dot_flat_outchan (Dot.to_string df_out); 
                    close_out dot_flat_outchan;
                    dot_flat_timer ());
		end
	    end in
        let _ = write_timer () 
        in  result

let xmain do_result fn = 
        let parse_timer = Debug.timer "parser" in
	match parsefile fn with
		None -> (print_string "no parse result\n"; parse_timer(); Some 1)
		| (Some pres) ->
                        let _ = parse_timer () in
                        let flatten_timer = Debug.timer "flatten" in
			let pres_flat = (match CmdLine.get_module_name() with
					None ->  Flatten.flatten pres 
					| (Some mn) -> Flatten.flatten_module mn pres)
				in
                        let _ = flatten_timer () in
                        let sem_an_timer = Debug.timer "semantic analysis" in
			let br = semantic_analysis pres_flat pres.mod_def_list in
                        let _ = sem_an_timer () in
			let debug = ! Debug.debug in
			let _ = Flow.pp_flow_map debug br.Flow.fmap in
                        let generate_timer = Debug.timer "generate" in
			let xml,cpp_code,h_code = generate fn br in
                        let _ = generate_timer () in
                        let dot_timer = Debug.timer "dot" in
			let dot_flat_output = 
                                        match CmdLine.get_module_name () with
                                                None -> 
                                                        Some (Dot.generate_flow br.Flow.fmap)
                                                |(Some mn) -> None in
                        let dot_output = 
                                match CmdLine.get_module_name() with
                                        None -> Dot.generate_program fn pres
                                        | (Some mn) ->
                                                Dot.generate_program mn 
                                                        (Flatten.context_for_module pres mn) in
                        let _ = dot_timer () in
			let suff_string = GenerateCPP1.get_module_file_suffix
				(CmdLine.get_module_name()) in
			let h_filename = "OFluxGenerate"^suff_string^".h" in
			let cpp_filename = "OFluxGenerate"^suff_string^".cpp" 
			in  do_result fn 
				{ xml = xml
				; h_code = h_code
				; cpp_code = cpp_code
				; dot_output = dot_output
				; dot_flat_output = dot_flat_output
				; h_filename = h_filename
				; cpp_filename = cpp_filename
				};
				None


