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
(** for retrieving command line arguments *)

val parse_argv : unit -> unit

val get_include_path : unit -> string list

val get_root_filename : unit -> string option

val get_module_name : unit -> string option

val get_plugin_name : unit -> string option

val get_uribase_path : unit -> string

val help_text : string

val get_runtime_engine : unit -> string

val get_avoid_dot_ios : unit -> bool

val get_timing_on : unit -> bool

val get_exclusive_tests : unit -> bool

val get_weak_unify : unit -> bool

val get_abstract_termination : unit -> bool

val get_code_prefix : unit -> string
