" Vim syntax file
" Language: OFlux

if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

so $VIMRUNTIME/syntax/cpp.vim

" Flux extentions
syn keyword fluxStatement as where terminate handle begin end if precedence
syn keyword fluxType guard readwrite sequence pool condition node source error atomic instance module exclusive initial plugin free
syn keyword fluxModifier detached abstract read write external unordered gc
syn keyword fluxInclude include

" Default highlighting
if version >= 508 || !exists("did_flux_syntax_inits")
  if version < 508
    let did_flux_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif
  HiLink fluxStatement Statement
  HiLink fluxType Type
  HiLink fluxModifier StorageClass
  HiLink fluxInclude Include
  " OFlux uses C label syntax for regular statements
  HiLink cUserLabel NONE
  delcommand HiLink
endif


let b:current_syntax = "flux"

