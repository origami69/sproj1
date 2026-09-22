# PA1 Written Analysis

**Name(s):** Arturo Becerra
**Date:**
**Built and tested on:** WSL2:
Distributor ID: Ubuntu
Description:    Ubuntu 24.04.4 LTS
Release:        24.04
Codename:       noble
gcc: (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0


Answer all five questions, **including every lettered part** — a question with
(a), (b) and (c) is not answered until all three are. Two to four sentences per
part is the right size; the whole file should come to two or three pages, not
one and not eight. Paste real output from your own runs where a question asks
for it; invented output is easy to spot and is handled as an academic-honesty
issue, not as a wrong answer.

Budget about two hours for this, and write it while the code is fresh in your
head. It is 20 points, which is more than any two tests are worth.

Write your own explanation. "It gets buffered" is not an explanation. Say
*what* is buffered, *where* that buffer lives, and *what happens to it*.

---

## Q1. The buffering trap (4 points)

Build and run the provided demonstration two different ways:

```
make
./workload/buffer_trap
./workload/buffer_trap | cat
```

Run the first command in a real terminal (an ssh session or a WSL shell), not from an editor's Run button. If both outputs look the same, your stdout is not a terminal.

**(a)** Paste both outputs exactly as you got them.
**(b)** They are different, and the program did not change. Explain why, in
terms of where `printf` actually writes and what `fork()` copies.
**(c)** Name the one library call that fixes it, say exactly where it goes in
`ptime.c`, and explain why placing it *after* the `fork()` would not work.

_Your answer:_
A.
./workload/buffer_trap:
A: printed before fork()
B: child
C: parent
./workload/buffer_trap | cat:
A: printed before fork()
B: child
A: printed before fork()
C: parent

B.
In the first example printf will wrtite to buffer and flush with \n before fork then after the fork which we got a printf which also writes and flush after \n then returns. This is what happens while the main process was waiting for the child process to finish and then printf too.
The reason we dont see duplicates here is because we use \n which flushes buffer when we are writing for console which uses line buffering
In the second example we must first understand the meaning of these two commands |, and cat
https://www.networkworld.com/article/2516127/linux-operators-using-and-many-more.html
According to this article the | command called pipe allows us to send output from command on leftside to the one the onto the right 
but this uses full buffering and not line buffering for stdout!
https://www.man7.org/linux/man-pages/man1/cat.1.html 
the cat command when no file is inputed will read from standard input and concat to standard ouput

now when we run the second command we call printf which will get buffered but this doesnt get flushed since it is being piped using full buffering and now when fork is called it will copy the previous buffer then we will also printf and place into buffer and when we return 0 this will also call exit and flush which will show A and B on screen then we return to parent we will printf and put C into buffer and return  0 will also flush the buffer and show [A, C] on screen
Parent[A] -> Child[A] -> Child[A, B] -> Flush -> Parent[A, C] -> Flush
C.
flush before calling fork will fix this, if we dont flush before but after then the child process will copy the buffer with the initial printf and then flush is called on both print and child it will show the initial printf from before fork twice, and this occurs once from parent and once on child. 
---
## Q2. Wall time is not CPU time (4 points)

Run both of these with your finished `ptime` and paste both reports:

```
./ptime ./workload/sleeper 1000
./ptime ./workload/spin 1000
```

**(a)** For each one, say which is larger — wall, or user+sys — and why.

**(b)** A program can finish with `user + sys` *greater* than `wall`. Describe
a program that would do that. (None of the provided workload programs on its own will do it; say what
kind of program would, and what hardware makes it possible. You can check your answer with
`./ptime sh -c "./workload/spin 500 & ./workload/spin 500 & wait"`.)

_Your answer:_
A.
./ptime ./workload/sleeper 1000:
sleeper: slept 1000 ms
=== ptime ===
command : ./workload/sleeper 1000
pid     : 8444
status  : exited 0
wall    : 1.009 s
user    : 0.001 s
sys     : 0.000 s
./ptime ./workload/spin 1000:
spin: burned 1000 ms of CPU (checksum 518080000)
=== ptime ===
command : ./workload/spin 1000
pid     : 8653
status  : exited 0
wall    : 1.008 s
user    : 0.987 s
sys     : 0.000 s

B.
If we use multi threading across different cores then it is possible for the sum of user and system times to exceed wall time since by multithreading we can reduce the time we spend running the section of code we are timing and the amount of work across those cores will be the same plus some overhead thats accosciaated with running multithreading so all in all we get a small increase to both user and system times and a decrease to walltime.
---

## Q3. Which clock, and why (4 points)

You used `CLOCK_MONOTONIC`. `CLOCK_REALTIME` also exists, and it reports the
wall-clock time of day.

**(a)** Describe a concrete situation in which measuring an interval with
`CLuOCK_REALTIME` would give a wrong answer — including one where the measured
duration comes out *negative*.

**(b)** Given that, why does `CLOCK_REALTIME` exist at all? Name one job it is
right for and `CLOCK_MONOTONIC` is wrong for.

_Your answer:_
https://www.man7.org/linux/man-pages/man3/clock_gettime.3.html
A)
According to this article clock realtime like the name says gets real time like a real wall clock but is affected by
discontinuous jumps in system time so an example would be when we get the duration like this 
time1 - time0
let time0 be 23:59 and time1 be 0:00 then we get a negative number since we have had a discontious jump in time from a new day passing!
according to the article this would be caused by the NTP (Network Time Protocal)
another way we can get a negative is the administrator changes the wallclock time themseleves too
B)
We should use Clock__Realtime when we want to do logging since the we want the time not to be stable and want to get timestamps
idk what some seconds from program execution means when i do logs for example:
RealTime log: 22:45 - "User Bean has logged to server"
Monotonic log: 432 sec - "User Bean has logged to server"
---

## Q4. `_exit` versus `exit` in the child (4 points)

Your child process calls `_exit()` after a failed `execvp()` — never `exit()`,
and never `return`.

**(a)** What does `exit()` do that `_exit()` does not?

**(b)** Connect this to Q1. Suppose `ptime` also printed a line to stdout before `fork()` (a debug line, say). Describe the specific wrong output a student would see if they used `exit()` in the child and had also skipped the fix from Q1. Pasting a real run is the best answer.

**(c)** In a program larger than this one, why is `return` from `main()` in the
child worse still?

_Your answer:_
I read from that doc quite a bit so dont expect to know and I read articles about many of the functions u gave us to 
understand what input and what output to get and what it does
https://www.man7.org/linux/man-pages/man2/exit.2.html
https://www.man7.org/linux/man-pages/man3/_exit.3.html
https://www.geeksforgeeks.org/c/c-exit-abort-and-assert-functions/

A)
exit() terminates the calling thread whereas _exit() terminates the calling process.
So when we run fork() we make a copy of the main process and exit() will flush buffered data
whereas _exit() will not flush and these buffers which are copies from parent will be eliminated.
B)
if we used a print to stdout then fork will copy the buffer containing whatever was printed into the parent buffer and when 
we exit we will flush the child buffer and cause a duplicate print here is an example:
printf("wow") parent buffer: ["wow"] not flushed yet
fork..... child copies parent buffer: ["wow"]
if(...) exit(1); child buffer: [] we flush and shows on screen
else return; parent buffer: [] we flush and shows on screen
output:
wowwow
i removed most the code since its just making a child and trap but the output would be wow twice in a row
C) 
return also calls exit which does the same flushing mechanism and remember testing this and would get a duplicate wow 
if i am not careful with flushing and its not always safe to assume line buffering will always save us.
---

## Q5. Whose CPU time did you measure? (4 points)

You called `getrusage(RUSAGE_CHILDREN, &ru)` once, after `waitpid` returned.

**(a)** Suppose `ptime` were changed to run the command three times in a row,
calling `getrusage(RUSAGE_CHILDREN, ...)` after each one. What would the third
call report — that run's CPU time, or something else? Say precisely what.

**(b)** Give a correct way to get *per-run* CPU time out of `RUSAGE_CHILDREN`
anyway.

**(c)** What would `getrusage(RUSAGE_SELF, ...)` have reported instead, and
roughly what number would you have seen in your report?

_Your answer:_
A)
If we call getrusage(RUSAGE_CHILDREN, ...) after 3 times then this will accumulate so it will add up from the previous times
here is what the total would look like after some time
0 + t_0 -> t_0 + t_1 -> (t_0 + t_1) + t_2
B)
the correct way would be to get per run cpu time is to save the previous state in another rusage 
and subtract the current run time by that previous rusage runtime, here is some pseudo assembly since im lazy:
ru1, ru2 = {0};
main_loop:
run some code then after waitpid finished
call, getrusage, ru1
print(tv_to_sec(&ru1) - tv_to_sec(&ru2))
ru2, ru1, 0
jmp main_loop;
C)
Like i said it would be the sums of previous times
for a commad like:
./ptime ./workload/spin 100 -n 3
I have seen user time go from 0.1 -> 0.2 -> 0.3 exceeding the wall clock times.
---

## Optional: stretch features

If you implemented any STRETCH items, list which ones and give one example
command line plus its output for each. Extra credit is not awarded for stretch
work that is not documented here.

_Your answer:_
Our command line is broken into three parts 
./ptime | command to run | extra features
here is my bash script i run for some test which allows me run a random amount of memory

also you can run multiple of these commands at the same time since I parse through argv from left to right
so the right most value will have priority for example:

./ptime ./myBash.sh -n 2 -n 3 -n 4 -m -t 3

the program will run 4 times in a row since it is the right most -n and then we also track maxrss usage across all the children and have a timer for each child of 3 sec
as long as the integer entered is greater than 0 then the commands are pretty flexible

here is my bash script i run for some test which allows me run a random amount of memory to test how maxrss works and i understand now
---------------
#!/bin/bash
size=$(((RANDOM % 997 + 1)))
head -c ${size}m /dev/zero | tail > /dev/null
--------------
1. -n count, use -n integer. And do this after writing your command since anything afterwards will not be run as a command 
./ptime ./myBash.sh -n 2
=== ptime ===
command : ./myBash.sh
pid     : 37068
status  : exited 0
wall    : 0.882 s
user    : 0.183 s
sys     : 1.053 s
=== ptime ===
command : ./myBash.sh
pid     : 37071
status  : exited 0
wall    : 0.693 s
user    : 0.130 s
sys     : 0.868 s
=== ptime avg ===
wall avg : 0.788 s
wall max : 0.882 s
wall min : 0.693 s

we print each individual run then at the end we return some avg, max, and min stats for the wall time
we can also do this for user and wall but the assignment did not ask so i will not.

2. -m we can run -m alone on command line with no extra numbers
./ptime ./myBash.sh -m
=== ptime ===
command : ./myBash.sh
pid     : 40820
status  : exited 0
wall    : 1.170 s
user    : 0.274 s
sys     : 1.376 s
maxrss  : 1002008 KB

3. -t number, this will set a timer for how long the command can run and prints if it passed or failed
Pass example:
/ptime ./myBash.sh -t 1
=== ptime ===
command : ./myBash.sh
pid     : 41501
status  : exited 0
wall    : 0.772 s
user    : 0.139 s
sys     : 0.960 s
Timer   : Pass

fail example:
=== ptime ===
command : ./workload/spin 1000
pid     : 41800
status  : signaled 9 (SIGKILL)
wall    : 1.001 s
user    : 1.001 s
sys     : 0.000 s
Timer   : Failed

4. For the pipe one i get the errno and print it out and the standard output with no error is 0 or it can be an unknown due to 
a signal KILL which will prevent it from getting the errno but you will be able to see by the statu. I just added a new field named
errno which will report child state 

status good child failed
ptime: cannot run 'cd': No such file or directory
=== ptime ===
command : cd tests
pid     : 11937
status  : exited 127
wall    : 0.035 s
user    : 0.002 s
sys     : 0.000 s
errno   : exited 2

both good
Makefile  PA1_instructions.md  README.md  analysis.md  myBash.sh  ptime  ptime.c  status  tests  workload
=== ptime ===
command : ls
pid     : 12241
status  : exited 0
wall    : 0.002 s
user    : 0.001 s
sys     : 0.000 s
errno   : exited 0

Timer example with sigkill
=== ptime ===
command : ./workload/spin 1200
pid     : 13360
status  : signaled 9 (SIGKILL)
wall    : 1.001 s
user    : 0.994 s
sys     : 0.000 s
errno   : exited 0
Timer   : Failed

many test at once
spin: burned 1200 ms of CPU (checksum 638680000)
=== ptime ===
command : ./workload/spin 1200
pid     : 13532
status  : exited 0
wall    : 1.204 s
user    : 1.202 s
sys     : 0.004 s
errno   : exited 0
maxrss  : 1636 KB
Timer   : Pass
spin: burned 1200 ms of CPU (checksum 638880000)
=== ptime ===
command : ./workload/spin 1200
pid     : 13547
status  : exited 0
wall    : 1.202 s
user    : 1.206 s
sys     : 0.000 s
errno   : exited 0
maxrss  : 1636 KB
Timer   : Pass
=== ptime avg ===
wall avg : 1.203 s
wall max : 1.204 s
wall min : 1.202 s