
exception FlattenFailure of string * ParserTypes.position

val break_namespaced_name : string -> string list

val flatten : ParserTypes.program -> ParserTypes.program
	(** expand the modules *)

val flatten_module : string -> ParserTypes.program -> ParserTypes.program

val context_for_module : ParserTypes.program -> string -> ParserTypes.program

