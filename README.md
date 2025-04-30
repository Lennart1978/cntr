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
### An (senseless) example:
```bash
> cal | cntr
                                   April 2025
                              Su Mo Tu We Th Fr Sa
                                     1  2  3  4  5
                               6  7  8  9 10 11 12
                              13 14 15 16 17 18 19
                              20 21 22 23 24 25 26
                              27 28 29 30
```

**File "cntr" is a compiled Linux x86_64 binary**
