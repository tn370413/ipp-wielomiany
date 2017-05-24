#!/bin/bash

if [ -n $3 ]; then
    echo "TOO MANY PARAMETERS"
    return 1
fi

if [ -z $2 ]; then
    echo "TOO FEW PARAMETERS"
    return 1
fi

if [ ! -x $1 ]; then
    echo "PROGRAM IS NOT AN EXECUTABLE"
    return 1
fi

if [ ! -d $2 ]; then
    echo "DATA DIRECTORY DOESN'T EXIST"
    return 1
fi

TEMPDIR=`mktemp -d`

FILE=`grep -l "START" $2`

TEMPFILE="$TEMPDIR/temp"
TEMPFILE2="$TEMPDIR/temp2"

# we remove 'START'
tail -n +2 "$FILE" > "$TEMPFILE"

LAST_LINE=`tail -1 $TEMPFILE`

while [ $LAST_LINE != "STOP" ]; do
    # last line wasn't stop, so we get the name of the next file from there
    NEXTFILE=${LAST_LINE:5}
    
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

# the last feeding
# we remove 'STOP'
head -n -1 "$TEMPFILE" > "$TEMPFILE2"
eval "$1 < $TEMPFILE > $TEMPFILE2"
tail -1 $TEMPFILE
return 0
