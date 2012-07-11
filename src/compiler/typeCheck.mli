(*
 *    OFlux: a domain specific language with event-based runtime for C++ programs
 *    Copyright (C) 2008-2012  Mark Pichora <mark@oanda.com> OANDA Corp.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Affero General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Affero General Public License for more details.
 *
 *    You should have received a copy of the GNU Affero General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *)
(** Basic type checking for maintaining consequences of 
    the type unification that are done when semantically analyzing 
    an OFlux program *)

(* type is opaque *)

type unification_result

exception Failure of string * ParserTypes.position

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
        ; full_collapsed : (int * ((string * bool) list)) list
        ; full_collapsed_names : (string * bool) list
        ; aliases : ((string * bool) * (string * bool)) list
        ; subset_order : (int * int) list
                (** (a,b) in this list means that the
                        inverse(union_map) will indicate
                    that a is a subset of b 
                    not reflexive,trans,anti-symmetric closed
                  *)
        }

val consequences : unification_result ->
        SymbolTable.symbol_table ->
        consequence_result

val get_decl_list_from_union : consequence_result ->
        SymbolTable.symbol_table ->
        int -> (* union number *)
        ParserTypes.decl_formal list 

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

val make_compatible : SymbolTable.symbol_table ->
        consequence_result -> (* a pre-existing const consequence result *)
        consequence_result -> (* a new one to be made compatible *)
        consequence_result (* has the same stuff, but is compatible with const version*)
