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

let determine_default_uribase () =
        let cwd = Sys.getcwd ()
        in  "file://"^cwd^"/"
        
let uribase_path = ref (determine_default_uribase())

let include_path = ref []

let module_name = ref None

let plugin_name = ref None

let root_filename = ref None

let runtime_engine = ref "classic"

let get_runtime_engine () = !runtime_engine

let noios = ref false

let exclusive_tests = ref false

let get_exclusive_tests() = !exclusive_tests

let code_prefix = ref "OFluxGenerate"

let get_code_prefix () = !code_prefix

let abstract_termination = ref false

let get_timing_on () = !Debug.timing_on

let weak_unify_on = ref true

let set_weak_unify tf = (weak_unify_on := tf)

let get_weak_unify () = !weak_unify_on

let help_text =
	 "usage: oflux [-d] [-t] [-duribase path] [-dnoios] [-a modulename | -p pluginname] [-absterm] [-oprefix pref] [-I incpath] [-rclassic | -rmelding] file.flux\n"
	^" -a  for compiling module code\n"
	^" -p  for compiling plugin code\n"
        ^" -absterm for terminating hanging abstract nodes\n"
	^" -I  for adding an include path (useful for locating modules elsewhere\n"
	^" -d  throws out debugging information (verbose)\n"
        ^" -duribase lets you specify the URI base path for generated DOT docs\n"
        ^" -dnoios avoids dot output with input/output data labels\n"
        ^" -t turn on timing of compilation steps\n"
        ^" -us cause only strong (old style) type unification to be allowed\n"
        ^" -oprefix causes the 'OFluxGenerate' code prefix to change\n"
        ^" -rclassic specifies the classic runtime engine (C++) [default]\n"
        ^" -rmelding specifies the melding runtime engine (C++)\n"
	^" -x  specifies exclusive conditions (no overlap in tests) -- helps efficiency\n"

let parse_argv () =
    let sz = Array.length Sys.argv in
    let rec pa stk i =
	if i <= 0 then ()
	else 
	    let arg = Array.get Sys.argv i in
	    let stk = 
		if i = sz-1 then (root_filename := Some arg; stk)
	        else (match arg,stk with
			    ("-I",[ipath]) -> 
				    (include_path := ipath::(!include_path); [])
			    | ("-a",[modname]) -> (module_name := (Some modname); [])
			    | ("-p",[plgname]) -> (plugin_name := (Some plgname); [])
			    | ("-duribase",[dub]) -> (uribase_path := dub; [])
			    | ("-oprefix",[pref]) -> (code_prefix := pref; [])
			    | ("-d",[]) -> (Debug.debug := true; [])
			    | ("-x",[]) -> (exclusive_tests := true; [])
			    | ("-t",[]) -> (Debug.timing_on := true; [])
			    | ("-absterm",[]) -> (abstract_termination := true; [])
			    | ("-dnoios",[]) -> (noios := true; [])
			    | ("-us",[]) -> (set_weak_unify false; [])
			    | ("-rclassic",[]) -> (runtime_engine := "classic"; [])
			    | ("-rmelding",[]) -> (runtime_engine := "melding"; [])
			    | _ -> arg::stk)
	    in  pa stk (i-1)
    in  pa [] (sz-1)


let get_include_path () = !include_path

let get_module_name () = !module_name

let get_plugin_name () = !plugin_name

let get_root_filename () = !root_filename

let get_uribase_path () = !uribase_path

let get_avoid_dot_ios () = !noios

let get_abstract_termination () = !abstract_termination
