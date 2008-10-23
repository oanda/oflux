
let determine_default_uribase () =
        let cwd = Sys.getcwd ()
        in  "file://"^cwd^"/"
        
let uribase_path = ref (determine_default_uribase())

let include_path = ref []

let module_name = ref None

let root_filename = ref None

let noios = ref false

let get_timing_on () = !Debug.timing_on

let weak_unify_on = ref true

let set_weak_unify tf = (weak_unify_on := tf)

let get_weak_unify () = !weak_unify_on

let help_text =
	 "usage: oflux [-d] [-t] [-duribase path] [-dnoios] [-a modulename] [-I incpath] file.flux\n"
	^" -a  for compiling module code\n"
	^" -I  for adding an include path (useful for locating modules elsewhere\n"
	^" -d  throws out debugging information (verbose)\n"
        ^" -duribase lets you specify the URI base path for generated DOT docs\n"
        ^" -dnoios avoids dot output with input/output data labels\n"
        ^" -t turn on timing of compilation steps"
        ^" -us cause only strong (old style) type unification to be allowed"

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
			    | ("-duribase",[dub]) -> (uribase_path := dub; [])
			    | ("-d",[]) -> (Debug.debug := true; [])
			    | ("-t",[]) -> (Debug.timing_on := true; [])
			    | ("-dnoios",[]) -> (noios := true; [])
			    | ("-us",[]) -> (set_weak_unify false; [])
			    | _ -> arg::stk)
	    in  pa stk (i-1)
    in  pa [] (sz-1)


let get_include_path () = !include_path

let get_module_name () = !module_name

let get_root_filename () = !root_filename

let get_uribase_path () = !uribase_path

let get_avoid_dot_ios () = !noios
