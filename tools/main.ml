
open TopLevel
open ParserTypes

let banner = "OFlux v0.388"

let help_text = banner^"\n"^CmdLine.help_text

let main () = 
	let pos_to_string p = ParserTypes.position_to_string p in
	let _ = CmdLine.parse_argv () in
	let res =
		try match CmdLine.get_root_filename () with
			(Some fl) ->
				let _ = print_string (banner^" on "^fl^"\n") in
                                let main_timer = Debug.timer "total" in
				let r = xmain write_result fl in
                                let _ = main_timer ()
                                in r
			| None -> 
				let _ = print_string help_text
				in  None
		with (Flow.Failure (msg,pos)) ->
			(print_string (" Error : "^(pos_to_string pos)^" "^msg^"\n"); Some 2)
		    | (TopLevel.SemanticFailure (s,p)) ->
			(print_string (" Error : "^s
			^" "^(pos_to_string p)^"\n"); Some 3)
		    | (GenerateXML.XMLConversion (s,p)) ->
			(print_string (" Error : XML conversion - "^s
			^" "^(pos_to_string p)^"\n"); Some 4)
		    | (GenerateCPP1.CppGenFailure s) ->
			(print_string (" Error : C++ code gen - "^s); Some 5)
		    | (SymbolTable.DeclarationLookup s) ->
			(print_string (" Error : Symbol table lookup - "^s); Some 5)
		    | (TypeCheck.Failure (s,p)) ->
			(print_string (" Error : "^(pos_to_string p)^"Type checking - "^s); Some 5)
		    | (Flatten.FlattenFailure (s,p)) ->
			(print_string (" Error : module flattening - "^s^" "
				^(pos_to_string p)^"\n");Some 6)
	in  match res with
		(Some errcode) -> exit errcode
		| _ -> () (*err code 0 *)
		

let evalmain = Printexc.catch main ()
