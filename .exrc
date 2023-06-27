map <F7> <cmd>cexpr system('tl build')<CR>
set makeprg=caustic
map <F9> :make make -c -j<CR>
map <C-F9> :make make -j<CR>
nnoremap <silent> <F3> :make make -j<CR>
"nnoremap <<silent> <C-F9> :cexpr system('tl build')<CR>
"nnoremap <silent> <C-F9> :echo TTTT
