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

let dump ndl =
        let dt = "doctag>" in
        let en = "<\n" in
        let dp = Debug.dprint_string in
        let _ = dp "\n" in
        let di label sd = dp (dt^label^sd.name^" : "^sd.content^en) in
        let d_nd nd = 
                begin
                dp (dt^nd.name^en);
                dp (dt^"desc>"^nd.content.description^en);
                List.iter (di " input>") nd.content.inputs;
                List.iter (di "output>") nd.content.outputs;
                List.iter (di " guard>") nd.content.guards
                end
        in  List.iter d_nd ndl
                

let empty_node_content () = 
        { description = ""
        ; inputs = []
        ; outputs = []
        ; guards = []
        }

type node_doc = node_content doc

let new_node s =
        { name = s
        ; content = empty_node_content ()
        }

let new_item s =
        { name = s
        ; content = ""
        }

type node_doc_list = node_doc list

let empty = []

let add_input' nc sd =
        nc.inputs <- (sd::(nc.inputs))

let add_output' nc sd =
        nc.outputs <- (sd::(nc.outputs))

let add_guard' nc sd =
        nc.guards <- (sd::(nc.guards))

let add_input (nd:node_doc) (sd:string doc) = 
        let (res:unit) = add_input' nd.content sd
        in  res
let add_output nd sd = add_output' nd.content sd
let add_guard nd sd = add_guard' nd.content sd 

let add_content_string ad s =
        ad.content <- (ad.content)^" "^s

let add_description nd s =
        nd.content.description <- nd.content.description^s

let add_node ndl nd = nd::ndl

let find ndl n = List.find (fun dt -> n = dt.name) ndl


(*
let add_input ntag iname idesc =
	ntag.inputs <- {name=iname;content=idesc}::ntag.inputs

let add_output ntag oname odesc =
	ntag.outputs <- {name=oname;content=odesc}::ntag.outputs

let add_guard ntag gname gdesc =
	ntag.guards <- {name=gname;content=gdesc}::ntag.guards
*)

