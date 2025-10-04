" @TODO: source the file with correct paths
if g:os == "Windows"
    nnoremap <leader>b :!pushd HaversineMathTests && build\build.bat /DPROFILER=1 && popd<CR>
    nnoremap <leader>r :!pushd HaversineMathTests && build\build.bat /O2 && popd<CR>
else
    nnoremap <leader>b :!pushd HaversineMathTests && build/build.sh -DPROFILER=1 && popd<CR>
    nnoremap <leader>r :!pushd HaversineMathTests && build/build.sh -O3 && popd<CR>
endif
