if g:os == "Windows"
    nnoremap <leader>b :!build\build.bat -DPROFILER=1<CR>
    nnoremap <leader>r :!build\build.bat<CR>
else
    nnoremap <leader>b :!build/build.sh -DPROFILER=1<CR>
    nnoremap <leader>r :!build/build.sh<CR>
endif
