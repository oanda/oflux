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

