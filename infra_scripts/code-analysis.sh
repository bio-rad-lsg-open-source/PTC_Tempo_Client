#!/bin/bash
#set -e

#cd application
#cd build-app
sudo snap install cmake --classic
mkdir build
cd build
/snap/bin/cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCLANG_TIDY_BUILD=ON ..
#sed -e 's/\("-I\)\(.*\/5\.15\.\)/"-isystem\2/' compile_commands.json > compile_commands-update.json
#jq 'map(select(any(contains("mongoose"))|not))' compile_commands-update.json > compile_commands.json
run-clang-tidy-10 -j 16 | tee clang-tidy-output
cat clang-tidy-output
if [ $JobType = "Codebuild" ]; then
    cat clang-tidy-output | python $CODEBUILD_SRC_DIR/build_scripts/PCR-Touch_bPARAM-Linux/clang-tidy-to-junit.py $CODEBUILD_SRC_DIR >junit.xml
    if [ $(cat clang-tidy-output | grep -c "warning:") -ne 0 ]; then exit 1; fi
fi



