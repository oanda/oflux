(** semantic analysis -- generates a flow *)

type flow (** opaque type *)

val flow_apply : ( string -> string -> flow -> flow -> 'a )
		* ( string -> (string option list * flow) list -> 'a )
		* ( string ->  flow list -> 'a )
		* ( string -> string -> flow -> flow -> 'a )
		* ( unit -> 'a ) -> 
		flow -> 'a

val null_flow : flow

val get_name : flow -> string

type flowmap

val flowmap_fold : (string -> flow -> 'b -> 'b) -> flowmap -> 'b -> 'b

val flowmap_find : string -> flowmap -> flow

val pp_flow_map : bool -> flowmap -> unit

val is_concrete : SymbolTable.symbol_table -> flowmap -> string -> bool

val is_null_node : flow -> bool

exception Failure of string * ParserTypes.position

val empty_flow_map : flowmap

type unified_list = ((string * bool) * (string * bool)) list

type built_flow = 
		{ sources : string list
		; fmap : flowmap
		; ulist : ((string * bool) * (string * bool)) list
		; symtable : SymbolTable.symbol_table
		; errhandlers : string list
		; modules : string list
                ; terminates : string list
                ; runoncesources : string list
		}

val build_flow_map : 
	SymbolTable.symbol_table ->
	ParserTypes.node_decl list ->
	ParserTypes.mainfun list ->
	ParserTypes.expr list ->
	ParserTypes.err list ->
        string list ->
	ParserTypes.mod_def list -> (** unflattened *)
		built_flow

