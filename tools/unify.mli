
(** unification utilities *)

val unify_single : string * string * string ->
		string * string * string -> string option

type success_type = Weak | Strong

type unify_result =
	Success of success_type
        | Fail of int * string (** ith and reason *)

val unify_type_in_out : ParserTypes.decl_formal list ->
		ParserTypes.decl_formal list -> unify_result

val unify' : bool * bool -> SymbolTable.symbol_table -> string -> string -> unify_result

val unify : SymbolTable.symbol_table -> string -> string -> unify_result

