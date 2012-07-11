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

let wtype_of guarddeclstr gmlist =
	let rec determine_wtype gtype_str modifiers =
		match gtype_str, modifiers with
			("readwrite",(ParserTypes.Read::_)) -> 1
			| ("readwrite",(ParserTypes.Write::_)) -> 2
			| ("readwrite",(ParserTypes.Upgradeable::_)) -> 4
			| ("pool",[]) -> 5 
			| ("exclusive",[]) -> 3
			| ("free",[]) -> 6
			| (_,(ParserTypes.NullOk::t)) -> determine_wtype gtype_str t
			| _ -> raise Not_found 
	in  determine_wtype guarddeclstr gmlist


let wtype_for stable gname gmlist =
	let gd = SymbolTable.lookup_guard_symbol stable gname 
	in wtype_of gd.SymbolTable.gtype gmlist
