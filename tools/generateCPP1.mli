(** code generator for the C++ runtime engine c++.1 *)

val emit_cpp : 
		string option -> (** name of the module if generating for one*)
		Flow.built_flow -> (** flattened flow *)
                (string * string) list -> (** uses model *)
			CodePrettyPrinter.code 
			* CodePrettyPrinter.code 
			* TypeCheck.consequence_result

val get_module_file_suffix : string option -> string

exception CppGenFailure of string (** when things go wrong *)
