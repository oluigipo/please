IF NOT EXIST TracyClient.obj (
	clang++ ../tracy/TracyClient.cpp -c -o TracyClient.obj -O2 -w -DTRACY_ENABLE
)

make unity "CC=clang -w -DTRACY_ENABLE" "LD=clang TracyClient.obj -fuse-ld=lld-link"
