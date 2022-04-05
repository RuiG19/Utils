#!/bin/bash

# Check the branchs and last commit hash on all git repositories in sub directories
for d in */; do
 cd $d
 if [ -d ".git" ]; then
   branch=$(git status | head -n 1)
   hash=$(git rev-parse --short HEAD)
   echo $d '->' $branch '(' $hash ')'
 fi
 cd ..
done