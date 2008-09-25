(** for retrieving command line arguments *)

val parse_argv : unit -> unit

val get_include_path : unit -> string list

val get_root_filename : unit -> string option

val get_module_name : unit -> string option

val get_uribase_path : unit -> string

val help_text : string

val get_avoid_dot_ios : unit -> bool

val get_timing_on : unit -> bool
