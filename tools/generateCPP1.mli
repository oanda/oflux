(** code generator for the C++ runtime engine c++.1 *)

val emit_cpp :  string option -> (** name of the module if generating for one*)
		Flow.built_flow -> (** flattened flow *)
                (string * string) list -> (** uses model *)
			CodePrettyPrinter.code (** h code *)
			* CodePrettyPrinter.code (** cpp code *)

val emit_plugin_cpp : string -> (** plugin name *)
                Flow.built_flow -> (** flattened flow pre-plugin *)
                Flow.built_flow -> (** flattened flow post-plugin *)
                (string * string) list -> (** uses model *)
			CodePrettyPrinter.code (** h code *)
			* CodePrettyPrinter.code (** cpp code *)

val get_module_file_suffix : string option -> string

exception CppGenFailure of string (** when things go wrong *)
