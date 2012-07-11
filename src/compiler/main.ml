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

open TopLevel
open ParserTypes

let banner = "OFlux " ^ Vers.vers

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
		    | (CodePrettyPrinter.CodeOutput s) ->
			(print_string (" Error : C++ code file I/O - "^s); Some 9)
		    | (SymbolTable.DeclarationLookup s) ->
			(print_string (" Error : Symbol table lookup - "^s); Some 5)
		    | (TypeCheck.Failure (s,p)) ->
			(print_string (" Error : "^(pos_to_string p)^" Type checking - "^s^"\n"); Some 5)
		    | (Flatten.FlattenFailure (s,p)) ->
			(print_string (" Error : module flattening - "^s^" "
				^(pos_to_string p)^"\n");Some 6)
	in  match res with
		(Some errcode) -> exit errcode
		| _ -> () (*err code 0 *)
		

let evalmain = Printexc.catch main ()
