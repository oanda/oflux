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
(** simple xml code for generation *)

type xml = Element of (string * (string * string) list * xml list)
        (* PCData section is not needed *)

val to_string_fmt : xml -> string

val get_tag : xml -> string

val get_attributes : xml -> (string * string) list

val get_contents : xml -> xml list

val diff : string list -> (** list of tags whose contents is ordered *)
        (xml -> xml -> xml) -> (** different handler *)
        (xml -> xml) -> (** same handler *)
        xml -> (** XML left input *)
        xml -> (** XML right input *)
        xml  
  (** find the earliest differences in two xml structures and handle them *)

val filter : (xml -> bool) -> xml -> xml option

val filter_list : (xml -> bool) -> xml list -> xml list
