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
(* documentation tags for OFlux *)

type 'a doc = 
	{ name : string
	; mutable content : 'a
	}

type node_content =
        { mutable description : string
        ; mutable inputs : string doc list
        ; mutable outputs : string doc list
        ; mutable guards : string doc list
        }

type node_doc = node_content doc

type node_doc_list = node_doc list

val dump : node_doc_list -> unit

val empty_node_content : unit-> node_content

val empty : node_doc_list

val add_input : node_doc -> string doc -> unit
val add_output : node_doc -> string doc -> unit
val add_guard : node_doc -> string doc -> unit

val add_content_string : string doc -> string -> unit

val add_description : node_doc -> string -> unit

val add_node : node_doc_list -> node_doc -> node_doc_list

val find : 'a doc list -> string -> 'a doc

val new_node : string -> node_doc

val new_item : string -> string doc


