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
(** symbol tables hold the declarations in the program *)


type symbol (** opaque type *)

type symbol_table

val empty_symbol_table : symbol_table

(** conditionals *)

type conditional_data = 
	{ arguments: ParserTypes.decl_formal list 
	; cfunction: string
	}

val add_conditional : symbol_table -> ParserTypes.cond_decl -> symbol_table

val lookup_conditional_symbol : symbol_table ->
		string -> conditional_data (** throws Not_found *)

val fold_conditionals : (string -> conditional_data -> 'b -> 'b) ->
		symbol_table -> 'b -> 'b

(** guards *)

type guard_data = { garguments: ParserTypes.decl_formal list
		; gtype: string
                ; gexternal: bool
		; return: ParserTypes.data_type 
		; magicnumber: int (* used for priority - text order in prog *)
                ; gunordered: bool
                ; ggc: bool
		}

val add_guard : symbol_table -> ParserTypes.atom_decl -> symbol_table

val lookup_guard_symbol : symbol_table -> 
		string -> guard_data (** throws Not_found *)

val fold_guards : (string -> guard_data -> 'b -> 'b) ->
		symbol_table -> 'b -> 'b

(** nodes *)

type node_data = 
		{ functionname : string
		; nodeinputs: ParserTypes.decl_formal list 
		; nodeoutputs: ParserTypes.decl_formal list option
		; nodeguardrefs: ParserTypes.guardref list
		; where: ParserTypes.position 
		; nodedetached: bool 
                ; nodeabstract: bool
                ; nodeexternal: bool
                }

val add_node_symbol : symbol_table -> ParserTypes.node_decl -> symbol_table

val lookup_node_symbol : symbol_table ->
		string -> node_data (** throws Not_found *)

val lookup_node_symbol_from_function : symbol_table ->
		string -> node_data (** throws Not_found *)
		(** finds any match *)

val fold_nodes : (string -> node_data -> 'b -> 'b) ->
		symbol_table -> 'b -> 'b

val is_detached : symbol_table -> string -> bool
val is_abstract : symbol_table -> string -> bool
val is_external : symbol_table -> string -> bool

val strip_position3 : ParserTypes.decl_formal -> string * string * string

exception DeclarationLookup of string

val get_decls : symbol_table -> string * bool -> ParserTypes.decl_formal list

(** modules *)

type module_inst_data =
		{ modulesource : string }

type module_def_data =
		{ modulesymbols : symbol_table
		}

val add_module_instance : symbol_table -> ParserTypes.mod_inst -> symbol_table

val add_module_definition : symbol_table -> ParserTypes.mod_def -> symbol_table

val lookup_module_instance : symbol_table -> string -> module_inst_data

val lookup_module_definition : symbol_table -> string -> module_def_data

val fold_module_definitions : (string -> module_def_data -> 'b -> 'b) ->
	symbol_table -> 'b -> 'b

val fold_module_instances : (string -> module_inst_data -> 'b -> 'b) ->
	symbol_table -> 'b -> 'b
	
(** programs *)

val add_program : symbol_table -> ParserTypes.program -> symbol_table

val get_module_uses_model : symbol_table -> (string * string) list
