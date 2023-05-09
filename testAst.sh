#!/bin/bash
code=0
for ast in test*.ast; do
  echo
  echo "Testing $ast ..."

  input=$(awk '
    /^### EXPECT$/ { section="" }
    section=="input" { print }
    /^### INPUT$/ { section="input" }
  ' "$ast")

  expect=$(awk '
    section=="expect" { print }
    /^### EXPECT$/ { section="expect" }
  ' "$ast")

  actual=$(echo "$input" | ./rpn 2>&1)

  if ! mismatch=$(diff <(echo "$actual") <(echo "$expect")); then
    printf "FAIL: %s\ndiff actual expect: %s\n" \
      "$input" "$mismatch"
    code=1
  else
    printf "PASS: %s\n" "$input"
  fi
done

exit "$code"
