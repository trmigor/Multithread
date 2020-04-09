#!/usr/bin/env bash
echo "Environment setting."
DIRNAME=$(dirname "$0")
CPPFLAGS="-pthread -Wall -Wextra -pedantic -std=c++17 -O3 -Wshadow -Wformat=2 -Wfloat-equal -Wconversion -Wcast-qual -Wcast-align -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -fsanitize=address,undefined -fno-sanitize-recover=all -fstack-protector"
rm -rf ${DIRNAME}/res/
mkdir ${DIRNAME}/res
mkdir ${DIRNAME}/res/num_i
mkdir ${DIRNAME}/res/size_i

echo "Compiling."
g++ ${CPPFLAGS} -o ${DIRNAME}/a.out ${DIRNAME}/main.cpp

echo "Started."

echo "num_i_size_100_many_1"
for ((i = 1; i < 50; ++i)); do
    ${DIRNAME}/a.out $i 100 1 >>${DIRNAME}/res/num_i/num_i_size_100_many_1.txt
done

echo "num_i_size_1000_many_1"
for ((i = 1; i < 50; ++i)); do
    ${DIRNAME}/a.out $i 1000 1 >>${DIRNAME}/res/num_i/num_i_size_1000_many_1.txt
done

echo "num_1_size_i_many_1"
for ((i = 1; i < 500; ++i)); do
    ${DIRNAME}/a.out 1 $i 1 >>${DIRNAME}/res/size_i/num_1_size_i_many_1.txt
done

echo "num_2_size_i_many_1"
for ((i = 1; i < 500; ++i)); do
    ${DIRNAME}/a.out 2 $i 1 >>${DIRNAME}/res/size_i/num_2_size_i_many_1.txt
done

echo "num_4_size_i_many_1"
for ((i = 1; i < 500; ++i)); do
    ${DIRNAME}/a.out 4 $i 1 >>${DIRNAME}/res/size_i/num_4_size_i_many_1.txt
done

echo "num_8_size_i_many_1"
for ((i = 1; i < 500; ++i)); do
    ${DIRNAME}/a.out 8 $i 1 >>${DIRNAME}/res/size_i/num_8_size_i_many_1.txt
done

echo "num_16_size_i_many_1"
for ((i = 1; i < 500; ++i)); do
    ${DIRNAME}/a.out 16 $i 1 >>${DIRNAME}/res/size_i/num_16_size_i_many_1.txt
done

echo "Ended."
