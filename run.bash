#!/bin/bash

argc=$#

#https://vaneyckt.io/posts/safer_bash_scripts_with_set_euxo_pipefail/
set -euo pipefail

#show commands (for debuging)
#set -x

#move to script dir 
cd "$(dirname "${0}")"

#save current dir as base dir
ROOT=$(pwd)

#where to store/get tar.gz / tar.bz2
DEPENDENCIES=../dependencies

#sudo apt install -f ninja-build

#avoid rm error if file doesn't exists
function safeRM {
  for file in ${@} ; do
    if [[ -f "${file}" ]]; then
      rm "${file}"
    fi
  done
}
#avoid rm error if directory doesn't exists
function safeRMDIR {
  for dir in ${@} ; do
    if [[ -d "${dir}" ]]; then
      rm -rf "${dir}"
    fi
  done
}
function safeMKDIR {
  if [ ! -d "$1" ]; then
    mkdir "$1"
  fi
}
function TAR {
  safeMKDIR "${DEPENDENCIES}"
  NAME=$1
  if [ ! -z "$2" ]; then
    NAME=$1-$2
  fi
  if [ -f "${DEPENDENCIES}/$NAME.tar.gz" ] && [ ! -f "${DEPENDENCIES}/$NAME.tar.bz2" ]; then
    echo "skipped"
  else
    tar -cjf $NAME.tar.bz2 $1
    mv $NAME.tar.bz2 ${DEPENDENCIES}/
    echo "stored $NAME.tar.bz2"
  fi
}
function UNTAR {
  NAME=$1
  if [ ! -z "$2" ]; then
    NAME=$1-$2
  fi
  if [ ! -d "$1" ]; then
    if [ ! -f "${DEPENDENCIES}/$NAME.tar.gz" ] && [ ! -f "${DEPENDENCIES}/$NAME.tar.bz2" ]; then
      echo "skipped"
      return
    else
      tar -xf ${DEPENDENCIES}/$NAME.tar.*
      echo "done $NAME.tar.*"
      return
    fi
  fi
}
#check for dependency
function CheckNeeded {
  NEEDED=$(which ${1})
  if [ ! -f "${NEEDED}" ]; then
    echo "${1} is a dependency, you must install it first!"
    exit 0
  fi
}
function get {
  BRANCH=${3-""}
  if [ ! -z "$BRANCH" ]; then
    BRANCH=$3
  fi
  untarResult=$(UNTAR "$1" "$BRANCH")
  if [ "${untarResult}"  == "skipped" ]; then
    if [ ! -d "$1" ]; then
      if [ ! -z "$BRANCH" ]; then
        git clone -b "$BRANCH" "$2"
      else
        git clone "$2"
      fi
      TAR "$1" "$BRANCH"
    fi
  fi
  # tarball may extract as name-branch/ instead of name/ — symlink if needed
  if [ ! -z "$BRANCH" ] && [ ! -d "$1" ] && [ -d "$1-$BRANCH" ]; then
    ln -sf "$1-$BRANCH" "$1"
  fi
}
function getAll {
  get fmt https://github.com/gwerners/fmt.git
  get stb https://github.com/gwerners/stb.git
  get imgui https://github.com/gwerners/imgui.git docking
  get json https://github.com/gwerners/json.git
  get raylib https://github.com/gwerners/raylib.git
  get rlImGui https://github.com/gwerners/rlImGui.git
  get ImNodes https://github.com/gwerners/ImNodes.git
  get agg https://github.com/gwerners/agg.git
}

function cleanOldBuild {
  ./clean.bash build
}


function build {

  pushd raylib
    safeMKDIR build
    pushd build
      cmake .. -G Ninja
      ninja
    popd
  popd
  safeMKDIR build
  pushd build
    cmake ..
    make all
  popd
}
function run {
  ./build/work/work
}

###############################################################################
#script main:
CheckNeeded g++ || exit 1
CheckNeeded cmake || exit 1
CheckNeeded ninja || exit 1
CheckNeeded git  || exit 1
getAll


build
run

###############################################################################
