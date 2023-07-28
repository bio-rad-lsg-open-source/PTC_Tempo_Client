#!/bin/bash
set -e
export SUDO="sudo"
if ! command -v sudo
then
	export SUDO=""
fi
sudo snap install cmake --classic
cmake --version
ls $WORKSPACE/*.cpp
export THREADS=8
export SONAR_SCANNER_VERSION=4.8.0.2856
export SONAR_SCANNER_HOME=$WORKSPACE/.sonar/sonar-scanner-$SONAR_SCANNER_VERSION-linux
curl --create-dirs -sSLo $WORKSPACE/.sonar/sonar-scanner.zip https://binaries.sonarsource.com/Distribution/sonar-scanner-cli/sonar-scanner-cli-$SONAR_SCANNER_VERSION-linux.zip
unzip -o $WORKSPACE/.sonar/sonar-scanner.zip -d $WORKSPACE/.sonar/
export PATH=$SONAR_SCANNER_HOME/bin:$PATH
export SONAR_SCANNER_OPTS="-server"

#curl --create-dirs -sSLo $WORKSPACE/.sonar/build-wrapper-linux-x86.zip https://sonarcloud.io/static/cpp/build-wrapper-linux-x86.zip
#unzip -o $WORKSPACE/.sonar/build-wrapper-linux-x86.zip -d $WORKSPACE/.sonar/
#export PATH=$WORKSPACE/.sonar/build-wrapper-linux-x86:/snap/bin:$PATH
echo $PATH

echo $SONARTOKEN

echo $GIT_COMMIT
echo $GIT_REVISION
GitShaShort="${GIT_COMMIT:0:8}"
echo "Short" $GitShaShort
ls $WORKSPACE
mkdir build
/snap/bin/cmake . -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && /snap/bin/cmake --build . --target all --config Release

echo "Build sonar scanner args..."
sonar_scanner_args=(
  "-X"
  "-Dsonar.projectName=PTC-Tempo-Client"
  "-Dsonar.projectVersion=1.0"
  "-Dsonar.sourceEncoding=UTF-8"
  "-Dsonar.organization=lsgsw"
  "-Dsonar.projectKey=lsgsw_PTC_Tempo_Client"
  "-Dsonar.sources=$WORKSPACE/"
  "-Dsonar.projectBaseDir=$WORKSPACE/"
  "-Dsonar.cfamily.threads=${THREADS}"
  "-Dsonar.cfamily.compile-commands=compile_commands.json"
  "-Dsonar.host.url=https://sonarcloud.io"
  "-Dsonar.scm.exclusions.disabled=false"
  "-Dsonar.log.level=TRACE"
  "-Dsonar.cpd.exclusions=**/mongoose.c,**/mongoose.h"
  "-Dsonar.exclusions=**/mongoose.c,**/mongoose.h"
  "-Dsonar.verbose=true"
)

if [ -z "$PullRequestKey" ]; then
  # Non-PR Sonar cloud command
  sonar_scanner_args+=(
    "-Dsonar.branch.name=$BranchName"
  )
else
  sonar_scanner_args+=(
    "-Dsonar.pullrequest.provider=github"
    "-Dsonar.pullrequest.branch=$BranchName"
    "-Dsonar.pullrequest.base=$BaseBranchName"
    "-Dsonar.pullrequest.key=$PullRequestKey"
    "-Dsonar.pullrequest.github.repository=$RepositoryName"
  )
fi
echo "start scan"

sonar-scanner "${sonar_scanner_args[@]}"


