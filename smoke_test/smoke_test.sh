#!/bin/sh -eu

if [ "$#" -lt 1 ]; then
    echo "Usage:"
    echo "    $0 path_to_huffman"
    exit 0
fi

EXECUTABLE="$1"
REAL_EXEC="$EXECUTABLE"

VALGRIND_OPTS="--leak-check=yes -q --leak-resolution=high --main-stacksize=64000000"
# To use Valgrind uncomment next line:
#REAL_EXEC="valgrind $VALGRIND_OPTS $EXECUTABLE"

COMPRESSED_FILE=compressed
DECOMPRESSED_FILE=decompressed

run()
{
    echo "***** Running: $EXECUTABLE $@"
    $REAL_EXEC "$@"
    return $?
}

for source_file in *.in; do
    run -c $source_file $COMPRESSED_FILE
    run -d $COMPRESSED_FILE $DECOMPRESSED_FILE
    diff -q $source_file $DECOMPRESSED_FILE
done

echo "Smoke test passed!"
