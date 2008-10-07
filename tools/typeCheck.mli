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

type consequence_result = 
        { equiv_classes : (string * bool) list list
        ; union_map : ((string * bool) * int) list
        }

val consequences : unification_result ->
        SymbolTable.symbol_table ->
        consequence_result

val get_union_from_equiv : consequence_result ->
        (string * bool) list ->
        int

val get_union_from_strio : consequence_result ->
        (string * bool) ->
        int

val consequences_equiv_fold : ( 'a -> (string * bool) list -> 'a ) ->
        'a ->
        consequence_result ->
        'a

val consequences_umap_fold : ( 'a -> ((string * bool) * int) -> 'a ) ->
        'a ->
        consequence_result ->
        'a


