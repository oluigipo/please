IF NOT EXIST TracyClient.obj (
	clang++ ../../ext/tracy/TracyClient.cpp -c -o TracyClient.obj -O2 -w -DTRACY_ENABLE
)

make TUS=unity_build "CC=clang -w -DTRACY_ENABLE" "LD=clang TracyClient.obj -fuse-ld=lld-link"
