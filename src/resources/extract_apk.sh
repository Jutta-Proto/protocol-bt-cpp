#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Invalid amount of arguments!" >&2
    echo "$0 JURA_JOE_APK_PATH.apk" >&2
    exit -1
fi

APK=$1

echo "Validating APK..."

unzip -l $APK | grep -q "assets/machinefiles/";
if [ "$?" != "0" ]
then
    echo "Invalid APK. Does not contain machinefiles." >&2
    exit -1
fi;

unzip -l $APK | grep -q "assets/JOE_MACHINES.TXT";
if [ "$?" != "0" ]
then
    echo "Invalid APK. Does not contain JOE_MACHINES.TXT." >&2
    exit -1
fi;

echo "APK valid."

echo "Extracting '$APK'..."
[ -e "machinefiles" ] && rm -rf "machinefiles"
mkdir "machinefiles"
unzip -q $APK "assets/machinefiles/*" -d "machinefiles/"
mv machinefiles/assets/machinefiles/*.xml machinefiles/
unzip -q $APK assets/JOE_MACHINES.TXT -d machinefiles/
mv machinefiles/assets/JOE_MACHINES.TXT machinefiles/
rm -rf machinefiles/assets
echo "done."