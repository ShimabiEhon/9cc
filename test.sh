#!/bin/bash
assert() {
  expected="$1"
  input="$2"

  ./9cc "$input" > tmp.s
  cc -o tmp tmp.s
  ./tmp
  actual="$?" ## 終了コード＝$expectedになるはず…

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$expected expected, but got $actual"
    exit 1
  fi
}

assert 0 0
assert 42 '42'

assert 21 '25+2-6'

assert 15 ' 19 + 6   -10  '

assert 43 '1+6*7'
assert 15 '5*(9-6)'
assert 4 '(3+5)/2'

assert 10 '-10+20'
assert 10 '-(10-20)'
assert 15 '-3*-5'

echo OK
