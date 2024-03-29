#!/bin/bash

ACTION='\033[1;90m'
FINISHED='\033[1;96m'
READY='\033[1;92m'
NOCOLOR='\033[0m' # No Color
ERROR='\033[0;31m'

TIMESTAMP=$(date)

echo
echo -e ${ACTION}Running build script at ${TIMESTAMP}${NOCOLOR}

# Check for git updates
echo
echo -e ${ACTION}Checking Git repo
echo -e =======================${NOCOLOR}

BRANCH=$(sudo -u guy git rev-parse --abbrev-ref HEAD)
if [ "$BRANCH" != "main" ] ; then
    echo -e ${ERROR}Not on main. Aborting. ${NOCOLOR}
    echo
    exit 0
fi

sudo -u guy git fetch
HEADHASH=$(sudo -u guy git rev-parse HEAD)
UPSTREAMHASH=$(sudo -u guy git rev-parse main@{upstream})

if [ "$HEADHASH" != "$UPSTREAMHASH" ] ; then
    echo -e ${ACTION}Not up to date with origin. Pull from remote${NOCOLOR}
else
    echo -e ${FINISHED}Current branch is up to date with origin/main.${NOCOLOR}
    exit 0
fi

PULLRESULT=$(sudo -u guy git pull)

if [ "$PULLRESULT" = "Already up to date." ] ; then
    echo -e ${FINISHED}Current branch is up to date with origin/main.${NOCOLOR}
    exit 0
fi

echo -e ${ACTION}Building code...${NOCOLOR}
MAKERESULT=$(sudo -u guy make)

if [ "$MAKERESULT" = "make: Nothing to be done for \`all\'." ] ; then
    echo -e ${FINISHED}make - Nothing to build.${NOCOLOR}
    exit 0
fi

if [ $? -ne 0 ] ; then
    echo -e ${ERROR}Build failed. Aborting. ${NOCOLOR}
    exit 0
fi    

echo -e ${ACTION}Build succeeded${NOCOLOR}

# Stop the process...
echo -e ${ACTION}Stopping wctl service...${NOCOLOR}
systemctl stop wctl

echo -e ${ACTION}Installing new version...${NOCOLOR}
make install

if [ $? -ne 0 ] ; then
    echo -e ${ERROR}Failed to install new version. Aborting. ${NOCOLOR}
    exit 0
fi    

# Restart the process...
echo -e ${ACTION}Restarting wctl service...${NOCOLOR}
systemctl start wctl
