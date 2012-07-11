(*
 *    OFlux: a domain specific language with event-based runtime for C++ programs
 *    Copyright (C) 2008-2012  Mark Pichora <mark@oanda.com> OANDA Corp.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Affero General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *)
open ParserTypes

exception SemanticFailure of string * ParserTypes.position

(* done in 3 passes *)

let open_file_from_include_path fn =
        let def_incl_path = if (String.get fn 0) = '/' then "" else "." in
	let rec offip ip =
		match ip with
			(hp::tp) ->
				(try let fullfile = hp^"/"^fn
                                     in  (open_in fullfile), fullfile
				with (Sys_error _) -> offip tp)
			| [] -> raise (SemanticFailure ("file "^fn^" not found",
				ParserTypes.noposition))
	in  offip (def_incl_path::(CmdLine.get_include_path()))

(*PASS 1*)

(* The Lexstack type. *)
type 'a t =
{   mutable stack : (string * in_channel * Lexing.lexbuf) list ;
    mutable filename : string ;
    mutable filenamelist : string list ;
    mutable chan : in_channel ;
    mutable lexbuf : Lexing.lexbuf ;
    mutable incluses : (string * string) list ;
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
        ; incluses = []
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
                ls.incluses <- (ls.filename,full_fn) :: ls.incluses ;
                ls.filename <- full_fn ;
                ls.filenamelist <- fname::(ls.filenamelist) ;
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

let basename fn =
        try let ri = String.rindex fn '.' in
            let rs = try min ri ((String.rindex fn '/') +1)
                     with Not_found -> 0
            in  String.sub fn rs (ri-rs)
        with Not_found -> fn


let parsefile fl =
	let _ = Lexer.reset() in
	let lexstack = ls_create fl Lexer.token in
	let dummy_lexbuf = Lexing.from_string "" in
	let res =
	    try Some (Parser.top_level_program (get_token lexstack) dummy_lexbuf)
	    with (Parsing.Parse_error| (Failure _)) ->
		let pos = (*current_pos lexstack*) Lexer.getPos()
		in  (print_string (fl^" Syntax Error "^(position_to_string pos)^"\n"); None)
                in
        let _ = DocTag.dump (Lexer.getNodeDocList())
	in  res, lexstack.incluses


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
		node_decls m_fns exprs errs terms mod_defs p.order_decl_list
	in  builtflow
	
(*PASS 3*)
let generate fn deplist xmldeplist br br_aft_opt um =
	let debug = !Debug.debug in
	let _ = Debug.dprint_string "FINAL FMAP\n"; Flow.pp_flow_map debug br.Flow.fmap in
	let h_code,cpp_code = 
                match br_aft_opt with
                        None ->
                                GenerateCPP1.emit_cpp (CmdLine.get_module_name()) br um
                        | (Some br_aft) ->
                                let pname = 
                                        match CmdLine.get_plugin_name() with
                                                None -> "APlugin"
                                                |(Some pn) -> pn
                                in  GenerateCPP1.emit_plugin_cpp pname br br_aft um deplist in 
	let xmlopt = match CmdLine.get_module_name(),br_aft_opt with
                (None,None) -> Some (GenerateXML.emit_program_xml fn br)
                | (None,(Some br_aft)) -> 
                        Some (GenerateXML.emit_plugin_xml fn (deplist@xmldeplist) br br_aft um)
                | _ -> None
	in  xmlopt,cpp_code,h_code



(*MAIN*)
let smain fn = 
	match parsefile fn with
		(None,_) -> (print_string "no parse result\n"; None)
		| ((Some pres),_) -> Some (semantic_analysis pres pres.mod_def_list)

let print_result _ (xml,h_code,cpp_code) =
	print_string (Xml.to_string_fmt xml); 
	print_string (CodePrettyPrinter.to_string h_code);
	print_string (CodePrettyPrinter.to_string cpp_code)

type oflux_result =
	{ xmlopt : Xml.xml option
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
	let is_plugin = 
		match CmdLine.get_plugin_name () with
			None -> false
			| _ -> true
		in
	let base_file = basename fn in
	let xml_file = base_file^".xml" in
	let dot_file = base_file^".dot" in
	let dot_flat_file = base_file^"-flat.dot" in  
        let write_timer = Debug.timer "writing" in
        let result =
	    begin
	    (let h_timer = Debug.timer "write .h" in
	     let _ = CodePrettyPrinter.output 
			of_result.h_filename of_result.h_code 
 	     in  h_timer ());
            (let cpp_timer = Debug.timer "write .cpp" in 
	     let _ = CodePrettyPrinter.output
			of_result.cpp_filename of_result.cpp_code
	     in  cpp_timer());
            (let dot_timer = Debug.timer "write .dot" in
             let dot_outchan = open_out dot_file 
		 in output_string dot_outchan (Dot.to_string of_result.dot_output); 
                    close_out dot_outchan;
                    dot_timer ());
	    if is_module then ()
	    else
		(let xml_timer = Debug.timer "write .xml" in
                 let xml = match of_result.xmlopt with
                                None -> raise (SemanticFailure ("no xml result",ParserTypes.noposition))
                                | (Some xml) -> xml in
                 let xml_outchan = open_out xml_file
		 in output_string xml_outchan (Xml.to_string_fmt xml);
                    close_out xml_outchan;
                    xml_timer ());
	    if is_module || is_plugin then ()
	    else
                (let dot_flat_timer = Debug.timer "write -flat.dot" in
                 let dot_flat_outchan = open_out dot_flat_file in
                 let df_out = match  of_result.dot_flat_output with
                                (Some op) -> op
                                | None -> raise Not_found
                 in output_string dot_flat_outchan (Dot.to_string df_out); 
                    close_out dot_flat_outchan;
                    dot_flat_timer ())
	    end in
        let _ = write_timer () 
        in  flush_all () ; result

let get_uses_model incluses program =
        let fnopt_to_fn fnopt =
                match fnopt with 
                        None -> (match CmdLine.get_root_filename () with 
                                None -> "-" 
                                | (Some s) -> s)
                        | (Some s) -> s in
        let get_plugin_module_uses pdef =
                let res = List.map (fun y -> strip_position y.modsourcename) pdef.pluginprogramdef.mod_inst_list
                in  List.map (fun h -> (strip_position pdef.pluginname,h)) res in
        let get_plugin_plugin_uses pdef =
                let ff flist (fn1,fn2) =
                        if (List.mem fn1 flist) && not (List.mem fn2 flist) then
                                fn2::flist
                        else flist in
                let pname,p1,_ = pdef.pluginname in
                let fp fn = List.fold_left ff [fn] incluses in
                let incluses_subset = fp (fnopt_to_fn p1.file) in
                let plist = List.filter (fun pdef' ->
                        let _,p2,_ = pdef'.pluginname 
                        in  List.mem (fnopt_to_fn p2.file) incluses_subset)
                        program.plugin_list in
                let res = List.map (fun x -> (pname, strip_position x.pluginname)) plist
                in  res in
        let stable = SymbolTable.add_program SymbolTable.empty_symbol_table program in  
        let module_uses_model = SymbolTable.get_module_uses_model stable in
        let um1 = (List.map (fun x-> strip_position x.pluginname,"") program.plugin_list) in
        let um2 = (List.map (fun x-> "",strip_position x.modsourcename) program.mod_inst_list) in
        let um3 = (List.concat (List.map get_plugin_module_uses program.plugin_list)) in
        let um4 = (List.concat (List.map get_plugin_plugin_uses program.plugin_list)) in
        let uses_model = um1 @ um2 @ um3 @ um4 @ module_uses_model in
        let _ = let pp_use (a,b) = Debug.dprint_string (a^" uses "^b^"\n")
                in  if (List.length uses_model) = 0 then
                        Debug.dprint_string "no modules/plugins found\n"
                    else List.iter pp_use uses_model 
        in  uses_model

let xmain do_result fn = 
        let parse_timer = Debug.timer "parser" in
	match parsefile fn with
		(None,_) -> (print_string "no parse result\n"; parse_timer(); Some 1)
		| (Some pres, incluses) ->
                        let _ = parse_timer () in
                        let flatten_timer = Debug.timer "flatten" in
			let pres_flat, pres_flat_after_opt, deplist, xmldeplist = 
                                (match (CmdLine.get_module_name()), (CmdLine.get_plugin_name()) with
					(None,None) ->  
                                                (Flatten.flatten pres
                                                ,None
                                                ,[]
						,[])
					| (Some mn,_) -> 
                                                (Flatten.flatten_module mn pres
                                                ,None
                                                ,[]
						,[])
					| (_,Some pn) -> 
                                                let bef,aft,xmldeplist = Flatten.flatten_plugin pn pres in
                                                let deplist = List.filter (fun p -> not (pn = p)) (List.map (fun pl -> strip_position pl.pluginname) pres.plugin_list)
                                                in bef, Some aft, deplist, xmldeplist)
				in
                        let _ = flatten_timer () in
                        let sem_an_timer = Debug.timer "semantic analysis" in
			let br,br_after_opt = 
                                match pres_flat_after_opt with
                                        None ->
                                                let br = semantic_analysis pres_flat pres.mod_def_list 
                                                in  br,None
                                        | (Some pres_flat_after) ->  
                                                let br_bef = semantic_analysis pres_flat pres.mod_def_list in
                                                let br_aft = semantic_analysis pres_flat_after pres.mod_def_list
                                                in ( br_bef , Some (Flow.make_compatible br_bef br_aft)) in
                        let _ = sem_an_timer () in
                        let uses_model_timer = Debug.timer "uses model" in
                        let uses_model = get_uses_model incluses pres in
                        let _ = uses_model_timer () in
			let debug = ! Debug.debug in
			let _ = Flow.pp_flow_map debug br.Flow.fmap in
                        let generate_timer = Debug.timer "generate" in
			let xmlopt,cpp_code,h_code = generate fn deplist xmldeplist br br_after_opt uses_model in
                        let _ = generate_timer () in
                        let dot_timer = Debug.timer "dot" in
			let dot_flat_output = 
                                        match CmdLine.get_module_name (), CmdLine.get_plugin_name () with
                                                (_,Some _) -> None 
                                                | (Some _,_) -> None 
                                                | (None,None) -> 
                                                        Some (Dot.generate_flow br.Flow.fmap)
                                in
                        let dot_output = 
                                match CmdLine.get_module_name() with
                                        None -> Dot.generate_program (basename fn) pres
                                        | (Some mn) ->
                                                Dot.generate_program mn 
                                                        (Flatten.context_for_module pres mn) in
                        let _ = dot_timer () in
			let suff_string = GenerateCPP1.get_module_file_suffix
				(CmdLine.get_module_name()) in
                        let suff_string = if suff_string = "" then
                                GenerateCPP1.get_module_file_suffix
                                        (CmdLine.get_plugin_name())
                                else suff_string in
                        let code_prefix = CmdLine.get_code_prefix () in
			let h_filename = code_prefix^suff_string^".h" in
			let cpp_filename = code_prefix^suff_string^".cpp" 
			in  do_result fn 
				{ xmlopt = xmlopt
				; h_code = h_code
				; cpp_code = cpp_code
				; dot_output = dot_output
				; dot_flat_output = dot_flat_output
				; h_filename = h_filename
				; cpp_filename = cpp_filename
				};
				None


