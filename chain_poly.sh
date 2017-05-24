#!/bin/bash

if [ $# -gt 2 ]; then
    echo "TOO MANY PARAMETERS"
    exit 1
fi

if [ $# -lt 2 ]; then
    echo "TOO FEW PARAMETERS"
    exit 1
fi

if [ ! -x $1 ]; then
    echo "PROGRAM IS NOT AN EXECUTABLE"
    exit 1
fi

if [ ! -d $2 ]; then
    echo "DATA DIRECTORY DOESN'T EXIST"
    exit 1
fi

TEMPDIR=`mktemp -d`

FILE=`grep -xl -m 1 "START" $2/*`

TEMPFILE="$TEMPDIR/temp"
TEMPFILE2="$TEMPDIR/temp2"

# we remove 'START'
tail -n +2 "$FILE" > "$TEMPFILE"

LAST_LINE=`tail -1 $TEMPFILE`

while [ "$LAST_LINE" != "STOP" ]; do
    # last line wasn't stop, so we get the name of the next file from there
    NEXTFILE="$2/${LAST_LINE:5}"
    
    # we remove 'FILE name'
    head -n -1 "$TEMPFILE" > "$TEMPFILE2"
    mv "$TEMPFILE2" "$TEMPFILE"
    
    # now in TEMPFILE we have content ready to be fed to the program
    # './' should't ever cause problems while it can help
    eval "./$1 < $TEMPFILE > $TEMPFILE2"
    rm "$TEMPFILE"
    
    LAST_LINE=`tail -1 "$NEXTFILE"`
    cat "$TEMPFILE2" "$NEXTFILE" > "$TEMPFILE"
    rm "$TEMPFILE2"
done

# the last feeding
# we remove 'STOP'
head -n -1 "$TEMPFILE" > "$TEMPFILE2"
eval "./$1 < $TEMPFILE2"

rm "$TEMPFILE"
rm "$TEMPFILE2"
rmdir "$TEMPDIR"

exit 0
