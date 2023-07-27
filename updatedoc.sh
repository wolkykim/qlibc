#!/bin/sh
git checkout gh-pages
git pull
(cd ../qlibc/src; make cleandoc; make doc)
rm -rf doc
cp -rp ../qlibc/doc .
cp ../qlibc/README.md index.md
git add --all doc
git add -u
git status
echo "Press ENTER to PUSH"
read x
git commit -a -m "Update doc"
git push origin gh-pages
