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

(** Code pretty printing in a C++ style (auto-indents based on incoming strings)
*)

exception CodeOutput of string

type code (** opaque type *)

val empty_code : code (** you begin with an empty code value *)

val add_code : code -> string -> code 
(** you add a string (line) to an existing code value to append it to the end
*)
 
val add_cr : code -> code
(** add a new line to the end of the code *)

val to_string : code -> string
(** convert the code value to a big string *)

val output : string -> (* filename *)
	code -> unit
(** write the code to the given filename *)
