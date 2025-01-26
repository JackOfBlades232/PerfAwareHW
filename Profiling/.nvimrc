if g:os == "Windows"
    nnoremap <leader>b :!build\build.bat<CR>
else
    nnoremap <leader>b :!build/build.sh<CR>
endif
