# cntr
cntr: A CLI command that prints a file or data from a pipe centered to stdout. Written in C
## example:
```bash
./cntr text.txt
cat text.txt | ./cntr
```
## Compile:
```bash
gcc -s -O3 -lm cntr.c -o cntr
```
### Install:
```bash
sudo cp cntr /usr/bin
```

