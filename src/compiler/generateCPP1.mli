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
(** code generator for the C++ runtime engine c++.1 *)

val emit_cpp :  string option -> (** name of the module if generating for one*)
		Flow.built_flow -> (** flattened flow *)
                (string * string) list -> (** uses model *)
			CodePrettyPrinter.code (** h code *)
			* CodePrettyPrinter.code (** cpp code *)

val emit_plugin_cpp : string -> (** plugin name *)
                Flow.built_flow -> (** flattened flow pre-plugin *)
                Flow.built_flow -> (** flattened flow post-plugin *)
                (string * string) list -> (** uses model *)
                string list -> (** plugin dependencies list *)
			CodePrettyPrinter.code (** h code *)
			* CodePrettyPrinter.code (** cpp code *)

val get_module_file_suffix : string option -> string

exception CppGenFailure of string (** when things go wrong *)
