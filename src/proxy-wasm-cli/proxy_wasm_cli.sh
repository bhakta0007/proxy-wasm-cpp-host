#!/bin/bash

PROXY_WASM_CLI_PATH=${PROXY_WASM_CLI_PATH:-$(dirname $0)}
VENV_FOLDER="venv"
RT_FILE="requirements.txt"
VENV_PATH=${VENV_PATH:-${PROXY_WASM_CLI_PATH}}
VENV=${VENV_PATH}/${VENV_FOLDER}
RT_PATH=${VENV_PATH}/${RT_FILE}

function setup_venv() {
    echo "Setting up venv"
    echo "Please wait while the python environment is being setup.."
    echo "This will take < a minute.."
    echo "Trying to create venv folder"
    RET=`python3 -m venv $VENV`
    if [ $? -ne 0 ]
    then
        echo "Error: Failed to create venv, error $RET"
        exit 2
    fi
    echo "Trying to setup venv: source $VENV/bin/activate && pip install --upgrade pip && pip install -r $RT_PATH $PROXY_OPTS"
    RET=`source $VENV/bin/activate && pip install --upgrade pip && pip install -r $RT_PATH $PROXY_OPTS`
    if [ $? -ne 0 ]
    then
        echo "Error: Failed to setup venv, error $RET"
        exit 3
    fi
}

INSTALL_VENV=0

if [ -d $VENV ]
then
    # Set venv, find the installed set of requirements
    # and compare with requirements.txt, if mismatch, install again
    RESP=`ls $VENV/bin/activate`
    RET=$?
    if [ $RET -ne 0 ]
    then
        setup_venv
    fi

    source ${VENV}/bin/activate
    RESP=`pip freeze | egrep -v "pkg-resources" > .pipcheck.txt`
    RET=$?
    if [ $RET -ne 0 ]
    then
        echo "Error checking environment: $RESP, retcode: $RET"
        rm -rf $VENV
        INSTALL_VENV=1
    fi

    RESP=`diff requirements.txt .pipcheck.txt`
    RET=$?
    if [ $RET -ne 0 ]
    then
        echo $RESP
        echo "Venv exists but not up to date - install again"
        rm -rf $VENV
        INSTALL_VENV=1
    else
        # echo "$VENV exists and is up-to-date"
        INSTALL_VENV=0
    fi
    RESP=`rm -f .pipcheck.txt`
elif [ "$#" -ge 1 ]
then
    INSTALL_VENV=$1
else
    INSTALL_VENV=1
fi

if [ $INSTALL_VENV -eq 1 ]
then
    setup_venv
fi

if [ -d $VENV ]
then
    source ${VENV}/bin/activate
fi

python proxy_wasm_cli.py
