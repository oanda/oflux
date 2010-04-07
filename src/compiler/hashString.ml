

let hash obj =
        let mstr = Marshal.to_string obj [] in
        let len = String.length mstr in
        let l1 = len/2 in
        let m1 = String.sub mstr 0 l1 in
        let m2 = String.sub mstr l1 (len-l1) in
        let digest x = Digest.to_hex (Digest.string x)
        in  (digest m1)^(digest m2)
