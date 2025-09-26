" @TODO: source the file with correct paths
if g:os == "Windows"
    nnoremap <leader>b :!pushd Haversine && build\build.bat /DPROFILER=1 && popd<CR>
    nnoremap <leader>r :!pushd Haversine && build\build.bat && popd<CR>
else
    nnoremap <leader>b :!pushd Haversine && build/build.sh -DPROFILER=1 && popd<CR>
    nnoremap <leader>r :!pushd Haversine && build/build.sh && popd<CR>
endif
