To run huffman smoke test you need POSIX compatible shell.

1. Build your solution. Locate executable file, e.g. /home/user/ha2/huffman
2. Change directory to directory containing this README.txt:

$ cd /home/user/huffman_smoke_test/
$ ls
00000000001.2.in
0000000000.2.in
...
README.txt
...
smoke_test.sh
...

3. Set executable flag on smoke_test.sh:

$ chmod +x smoke_test.sh

4. Run smoke_test with path to your solution executable:

./smoke_test.sh /home/user/ha2/huffman

It's important to run smoke test script from directory that contains tests!

5. Check output for unexpected errors.

If you have valgrind you can enable it by uncommenting line in the
smoke_test.sh that marked accordingly.
