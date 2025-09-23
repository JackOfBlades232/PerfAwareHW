" @TODO: source the file with correct paths
if g:os == "Windows"
    nnoremap <leader>b :!pushd Testing && build\build.bat /DPROFILER=1 && popd<CR>
    nnoremap <leader>r :!pushd Testing && build\build.bat && popd<CR>
else
    nnoremap <leader>b :!pushd Testing && build/build.sh -DPROFILER=1 && popd<CR>
    nnoremap <leader>r :!pushd Testing && build/build.sh && popd<CR>
endif
