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
(** a generator for the OFlux xml flow descriptions *)
exception XMLConversion of string * ParserTypes.position
  (** when things go wrong *)

val emit_program_xml : string -> (** filename *)
		Flow.built_flow -> 
			Xml.xml

val emit_program_xml' : string -> (** filename *)
		Flow.built_flow -> 
		(string * string) list -> (** full uses model *)
			Xml.xml

val emit_plugin_xml : string -> (** filename *)
                string list -> (** list of dependencies (plugins *)
		Flow.built_flow -> (** before *)
		Flow.built_flow -> (** after *)
		(string * string) list -> (** full used model *)
			Xml.xml

val write_xml : Xml.xml -> string -> unit

