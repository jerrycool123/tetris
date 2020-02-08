#!/bin/bash
for i in range{1..100};
do
	./main 1234 & >1
	telnet 140.112.30.41 1234 & >1
	pkill -9 main >1
done
