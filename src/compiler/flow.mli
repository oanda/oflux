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

val flowmap_diff : 
        flowmap -> (** flowmap A *)
        flowmap -> (** flowmap B *)
        (flow option * flow option) list (**stuff in A-B sorta*)
        

val pp_flow_map : bool -> flowmap -> unit

val is_concrete : SymbolTable.symbol_table -> flowmap -> string -> bool

val is_null_node : flow -> bool

exception Failure of string * ParserTypes.position

val empty_flow_map : flowmap

type built_flow = 
		{ sources : string list
		; fmap : flowmap
		; ulist : TypeCheck.unification_result
		; symtable : SymbolTable.symbol_table
		; errhandlers : string list
		; modules : string list
                ; terminates : string list
                ; runoncesources : string list
		; doorsources : string list
                ; consequences : TypeCheck.consequence_result
                ; guard_order_pairs : (string * string) list
		}

val build_flow_map : 
	SymbolTable.symbol_table ->
	ParserTypes.node_decl list ->
	ParserTypes.mainfun list ->
	ParserTypes.expr list ->
	ParserTypes.err list ->
        string list ->
	ParserTypes.mod_def list -> (** unflattened *)
	ParserTypes.order_decl list -> 
		built_flow

val make_compatible : 
        built_flow -> (* const baseline one *)
        built_flow -> (* one to change *)
        built_flow
