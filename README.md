SOAPdenovo-Trans
================

A fork of SOAPdenovo-Trans from http://sourceforge.net/projects/soapdenovotrans/

Fixes a number of 'seqmentation fault' errors

COMPILATION
===========

Type

./make.sh

RUNNING
=======

First you will need a config file. Here is an example:

```
max_rd_len=20000
[LIB]
avg_ins=250
reverse_seq=0
asm_flags=3
q1=left.fastq
q2=right.fastq
q=single.fastq
```

To run SOAPdenovo-Trans

./SOAPdenovo-Trans-127mer all -s config -o assembly -p 8

for running on 8 cpus.

