if exists("b:current_syntax")
  finish
endif

syntax keyword slKeyword if else end then continue break while def var and or elif return false true
syntax keyword slDataType int string bool float

syntax match slVariable "\$[a-zA-Z_][a-zA-Z0-9_]*"

syntax match slFunction "\<[a-zA-Z_][a-zA-Z0-9_]*\>\ze\."
syntax match slSub "\.\zs[a-zA-Z_][a-zA-Z0-9_]*"

syntax region slMultiString start='"""' end='"""' keepend
syntax region slString start='"' end='"' skip='\\"' keepend
syntax region slChar start='\'' end='\'' skip='\\"' keepend

syntax match slComment "#.*$"

syntax match slFloat "\<\d\+\.\d\+\>"
syntax match slNumber "\<\d\+\>"

syntax keyword slCompare equ neq eqg eql
syntax match slCompare "[<>]"

syntax match slOperator "[+\-*/%]"
syntax match slBitwise "[&|^~]"
syntax match slBitwise "<<\|>>"
syntax match slAssignment "="

highlight slKeyword guifg=#cba6f7
highlight slDataType guifg=#89dceb
highlight slVariable guifg=#f9e2af
highlight slFunction guifg=#89b4fa
highlight slSub guifg=#a6e3a1
highlight slMultiString guifg=#ff9999
highlight slString guifg=#ff9999
highlight slChar guifg=#ff9999
highlight slComment guifg=#6c7086
highlight slNumber guifg=#fab387
highlight slFloat guifg=#fab387
highlight slCompare guifg=#f38ba8
highlight slOperator guifg=#f38ba8
highlight slBitwise guifg=#f38ba8
highlight slAssignment guifg=#f38ba8

let b:current_syntax = "sl"
