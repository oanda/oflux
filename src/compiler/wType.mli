(** code to determin the wtype int for particular guard acquisitions *)
(* nice to have this in one place *)

val wtype_of : string (*gtype*) -> 
		(ParserTypes.guardmod list) -> 
		int

val wtype_for : SymbolTable.symbol_table -> 
		string (*guard name*) ->
		(ParserTypes.guardmod list) -> 
		int


