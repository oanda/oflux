
(** unification utilities *)

val unify_single : string * string * string ->
		string * string * string -> string option

type unify_result =
	 Success
        | Fail of int * string (** ith and reason *)

val unify_type_in_out : ParserTypes.decl_formal list ->
		ParserTypes.decl_formal list -> unify_result

val unify' : bool * bool -> SymbolTable.symbol_table -> string -> string -> unify_result

val unify : SymbolTable.symbol_table -> string -> string -> unify_result

