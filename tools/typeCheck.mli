(** Basic type checking for maintaining consequences of 
    the type unification that are done when semantically analyzing 
    an OFlux program *)

(* type is opaque *)

type unification_result

(* basic operations *)

val empty : unification_result (* initial value for an empty program *)

val concat : unification_result -> unification_result -> unification_result

(* input operations -- constructing *)

val from_basic_nodes : SymbolTable.symbol_table ->
        string list -> (* sinks *)
        string list -> (* sources *)
        unification_result

val type_check_general : (bool * bool) -> (* s-IO-type, IO-type *)
        unification_result -> (* unification_result so far *)
        SymbolTable.symbol_table ->
        string -> (* s-Name *)
        string -> (* Name *)
        ParserTypes.position -> (* s-position *)
        unification_result

val type_check :  unification_result -> (* unification result so far *)
        SymbolTable.symbol_table ->
        string -> (* s-Name *)
        string -> (* Name *)
        ParserTypes.position -> (* s-position *)
        unification_result

val type_check_concur :  unification_result -> (* unification result so far *)
        SymbolTable.symbol_table -> 
        string -> (* Name1 *)
        string -> (* Name2 *)
        ParserTypes.position -> (* position1 *)
        unification_result

val type_check_inputs_only :  unification_result -> (* unification result so far *)
        SymbolTable.symbol_table -> 
        string -> (* Name1 *)
        string -> (* Name2 *)
        ParserTypes.position -> (* position1 *)
        unification_result

val type_check_outputs_only :  unification_result -> (* unification result so far *)
        SymbolTable.symbol_table -> 
        string -> (* Name1 *)
        string -> (* Name2 *)
        ParserTypes.position -> (* position1 *)
        unification_result

(* getting the consequences *)

val consequences : unification_result ->
        SymbolTable.symbol_table ->
        (string * bool) list list (*list*)
        (** The output is a list of list of equivalence classes:
            [ [ e11; e21; e31; ...] ; [ e12; e22; e32; ...] ; ... ]
            eij : string list ( an equivalence class
            - all the eij have pairwise empty intersections
            - Name1 & Name2 in eij means that they are exactly the same (type)
            - eij;e{i+1}j side by side means that the type list of eij
                is a subset of e{i+1}j
          The concept is that we want the equivalence class chains in the result          to be of maximal size.  
          Each chain is an oportunity to do some inheritance on IO structs
          if we ever get that fancy.
        *)
